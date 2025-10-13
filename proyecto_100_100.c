#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "string.h"
#include "driverlib/pin_map.h"
// Uart libraries:
#include "driverlib/uart.h"
#include "utils/uartstdio.c"
//-------------------------------
// PWM libraries:
#include "driverlib/pwm.h"

int freq = 120000000;
volatile uint32_t ui32Loop;

void LecSnsUlt(uint32_t *distance);
void checkUART(char rxBuffer[10], int *rxIndex);
void interactiveDelay(float time_sec, int *tIter);
void toogleUart(bool *ledUart);
int CnvStrToInt(char *rxBuffer);
void CnvIntToStr(int DtCy, char *strDtCy);
int Duttyfun(int value);

int main(void)
{
    uint32_t ui32Period;
    uint32_t distance = 0;
    char rxBuffer[10];
    bool ledUart = true;
    int rxIndex = 0;
    int tIter = 0;
    int Dutty=3500;
    bool BuzzerState = false;
    bool mtr1Stt = false;
    bool mtr2Stt = false;
    bool waitingDuty = false;

    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),freq); 

    // Habilitar perifericos:
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //-------------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //-------------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  // HC-SR04 Trig/Echo
    //-------------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0); //Modulo PWM
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH); //Direccion del motor
    
    // Verificar perifericos:
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))  {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))  {}
    //-----------------------------------------------------
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))  {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))  {}
    //-----------------------------------------------------
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))  {}
    //-----------------------------------------------------
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0))  {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH))  {}


    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, 0x07);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0x11);
    GPIOPinWrite(GPIO_PORTN_BASE, 0x03, 0);  // Apaga PN0 y PN1
    //--------------------------------------------------------------
    // Configurar Uart:
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, 0x03);
    //--------------------------------------------------------------
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0x20); // Trig
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, 0x10);  // Echo
    //--------------------------------------------------------------
    GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, 0x0f);

    UARTStdioConfig(0, 9600, freq);

    //--------------------------------------------------------------

    // Configurar PF2 como salida PWM2
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinTypePWM(GPIO_PORTF_BASE, 0x04);

    // Configurar el generador PWM1 en modo up/down
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    // Periodo del PWM: 240 kHz
    ui32Period = freq / 25000;//Dutty cycle de 4800
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ui32Period);

    // Habilitar la salida PWM en PF2
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);

    // Habilitar el generador PWM
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    //--------------------------------------------------------------
    // Configurar PF1 como salida PWM1
    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, 0x02);

    // Configurar el generador PWM1 en modo up/down
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ui32Period);

    // Habilitar la salida PWM en PF1
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);

    // Habilitar el generador PWM
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

    // F0 actividad uart
    // F4 Accion UART
    // N0 UltraS
    // N1 Motores

    GPIOPinWrite(GPIO_PORTH_BASE, 0x0f, 0x00);

    while(1)
    {
        LecSnsUlt(&distance);
        checkUART(rxBuffer, &rxIndex);
        GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0);  // Apaga PN0 y PN1
        GPIOPinWrite(GPIO_PORTF_BASE, 0x01, 0);
        if (strncmp(rxBuffer, "buzzer", 6) == 0) {
            BuzzerState = true;
            memset(rxBuffer, 0, sizeof(rxBuffer));
            GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0x04);
            toogleUart(&ledUart);
        }
        if (strncmp(rxBuffer, "motor1", 6) == 0) {
            if (mtr1Stt == true) {
                mtr1Stt = false;
            } else {
                mtr1Stt = true;
            }
            memset(rxBuffer, 0, sizeof(rxBuffer));
            toogleUart(&ledUart);
        }
        if (strncmp(rxBuffer, "motor2", 6) == 0) {
            if (mtr2Stt == true) {
                mtr2Stt = false;
            } else {
                mtr2Stt = true;
            }
            memset(rxBuffer, 0, sizeof(rxBuffer));
            toogleUart(&ledUart);
        }
        if (strncmp(rxBuffer, "DutyCycle", 9) == 0) {
            waitingDuty = true;
            memset(rxBuffer, 0, sizeof(rxBuffer));
        }
        while (waitingDuty == true) {
            checkUART(rxBuffer, &rxIndex);
            SysCtlDelay(1200000);
            int value = CnvStrToInt(rxBuffer);
            if (value >0){
                UARTprintf("El valor del DutyCycle es: %d\n",value);
                memset(rxBuffer, 0, sizeof(rxBuffer));
                waitingDuty = false;
                toogleUart(&ledUart);
                Dutty=Duttyfun(value);
            }
        }
        if (BuzzerState == true) {
            interactiveDelay(2.0, &tIter);
            if (tIter == 0) {
                BuzzerState = false;
                GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0);
            }
        }
        if (mtr1Stt == true) {
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x02);
        }
        else if (mtr1Stt == false) {
            GPIOPinWrite(GPIO_PORTH_BASE, 0x01, 0x00);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x02, 0x00);
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x00);
        }
        if (mtr2Stt == true) {
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x02);
        }
        else if (mtr2Stt == false) {
            GPIOPinWrite(GPIO_PORTH_BASE, 0x08, 0x00);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x04, 0x00);
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x00);
        }
        if(distance < 10){ // menos de 10 cm
            if (mtr1Stt == true) {
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,0);
                GPIOPinWrite(GPIO_PORTH_BASE, 0x01, 0x01);
                GPIOPinWrite(GPIO_PORTH_BASE, 0x02, 0x00);
            }
            if (mtr2Stt == true) {
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,0);
                GPIOPinWrite(GPIO_PORTH_BASE, 0x08, 0x08);
                GPIOPinWrite(GPIO_PORTH_BASE, 0x04, 0x00);
            }
        }
        else{
            if (mtr1Stt == true) {
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, Dutty);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x01, 0x00);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x02, 0x02);
            }
            if (mtr2Stt == true) {  
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,Dutty);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x08, 0x00);
            GPIOPinWrite(GPIO_PORTH_BASE, 0x04, 0x04);
            }
        }
    }
}

//============================================================================

void checkUART(char rxBuffer[10], int *rxIndex) {
  int c;
  while (UARTCharsAvail(UART0_BASE)) {
    c = UARTCharGetNonBlocking(UART0_BASE);

    if (c == -1) return;

    if (c == '\r' || c == '\n') {
      rxBuffer[*rxIndex] = '\0';
      *rxIndex = 0;
      GPIOPinWrite(GPIO_PORTF_BASE, 0x01, 0x01);
      for (ui32Loop = 0; ui32Loop < (freq/60); ui32Loop++) {}
    } else {
      if (*rxIndex < 9) {
        rxBuffer[*rxIndex] = (char)c;
        (*rxIndex)++;
      }
    }
  }
}
//------------------------------------------------------------------
void interactiveDelay(float time_sec, int *tIter){
  int TotalTimeIter = (int)(time_sec*10);
  if (*tIter == 0) {
    *tIter = TotalTimeIter;
  }
  for (ui32Loop = 0; ui32Loop < (freq/100); ui32Loop++) {}
  *tIter -= 1;
}
//------------------------------------------------------------------
void LecSnsUlt(uint32_t *distance) {
    uint32_t sum = 0;
    int validCount = 0;
    uint32_t pulseTime;
    int attempts = 0;

    while (validCount < 5 && attempts < 10) { // Intenta máximo 10 veces
        // Generar pulso de 10 us en Trig
        GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x20);
        SysCtlDelay(freq / 3000000);  // ~10 us
        GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x00);

        // Esperar a que Echo se ponga en alto con timeout
        uint32_t timeout = 100000;
        while(!GPIOPinRead(GPIO_PORTB_BASE, 0x10) && timeout > 0) {
            timeout--;
        }
        if (timeout == 0) {
            attempts++;
            continue; // descarta esta medición
        }

        // Medir cuánto tiempo está en alto
        pulseTime = 0;
        timeout = 100000;
        while(GPIOPinRead(GPIO_PORTB_BASE, 0x10) && timeout > 0) {
            pulseTime++;
            SysCtlDelay(freq / 3000000); // ~1 us
            timeout--;
        }

        if (timeout == 0) {
            attempts++;
            continue; // descarta esta medición
        }

        uint32_t dist = pulseTime / 58; // convertir a cm

        if (dist <= 400) { // máximo rango del HC-SR04
            sum += dist;
            validCount++;
        }

        attempts++;
    }

    if (validCount > 0) {
        *distance = sum / validCount; // promedio de lecturas válidas
    } else {
        *distance = 999; // no se pudo medir
    }

    //UARTprintf("%d\n", *distance);

    // LED de advertencia
    if (*distance > 10) {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
    } else {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0x01);
    }
}



//------------------------------------------------------------------
void toogleUart(bool *ledUart){
    if (*ledUart == true) {
        *ledUart = false;
        GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0);
    } else {
        *ledUart = true;
        GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0x10);
    }
}
//------------------------------------------------------------------
int CnvStrToInt(char *rxBuffer) {
    int x = 0;
    int i = 0;

    while (rxBuffer[i] != '\0') {
        if (rxBuffer[i] >= '0' && rxBuffer[i] <= '9') {
            x = x * 10 + (rxBuffer[i] - '0');
        }
        i++;
    }

    return x;
}
void CnvIntToStr(int DtCy, char *strDtCy) {
    int i = 0;
    int isNegative = 0;

    if (DtCy < 0) {
        isNegative = 1;
        DtCy = -DtCy;
    }

    do {
        strDtCy[i++] = (DtCy % 10) + '0';
        DtCy = DtCy / 10;
    } while (DtCy != 0);

    if (isNegative) {
        strDtCy[i++] = '-';
    }

    strDtCy[i] = '\0';

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = strDtCy[start];
        strDtCy[start] = strDtCy[end];
        strDtCy[end] = temp;
        start++;
        end--;
    }
}
int Duttyfun(int value){
    int x=(value)*18+3000;
    return x;
}
