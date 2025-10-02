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

int freq = 120000000;
volatile uint32_t ui32Loop;

void LecSnsUlt(uint32_t *distance);
void checkUART(char rxBuffer[10], int *rxIndex);
void interactiveDelay(float time_sec, int *tIter);

int main(void)
{
    uint32_t distance = 0;
    char rxBuffer[10];
    bool ledUart = true;
    int rxIndex = 0;
    int tIter = 0;
    bool BuzzerState = false;
    bool mtr1Stt = false;
    bool mtr2Stt = false;

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
    
    // Verificar perifericos:
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))  {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))  {}
    //-----------------------------------------------------
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))  {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))  {}
    //-----------------------------------------------------
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))  {}


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

    UARTStdioConfig(0, 9600, freq);

    // F0 actividad uart
    // F4 Accion UART
    // N0 UltraS
    // N1 Motores

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
            if (ledUart == true) {
                ledUart = false;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0);
            } else {
                ledUart = true;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0x10);
            }
        }
        if (strncmp(rxBuffer, "motor1", 6) == 0) {
            if (mtr1Stt == true) {
                mtr1Stt = false;
            } else {
                mtr1Stt = true;
            }
            memset(rxBuffer, 0, sizeof(rxBuffer));
            if (ledUart == true) {
                ledUart = false;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0);
            } else {
                ledUart = true;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0x10);
            }
        }
        if (strncmp(rxBuffer, "motor2", 6) == 0) {
            if (mtr2Stt == true) {
                mtr2Stt = false;
            } else {
                mtr2Stt = true;
            }
            memset(rxBuffer, 0, sizeof(rxBuffer));
            if (ledUart == true) {
                ledUart = false;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0);
            } else {
                ledUart = true;
                GPIOPinWrite(GPIO_PORTF_BASE, 0x10, 0x10);
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
        //Pwm
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x02);
            GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0x04);
            interactiveDelay(0.2, &tIter);
            if (tIter == 0) {
                mtr1Stt = false;
                GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0);
            }
        }
        if (mtr2Stt == true) {
        //Pwm
            GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0x02);
            GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0x04);
            interactiveDelay(0.2, &tIter);
            if (tIter == 0) {
                mtr2Stt = false;
                GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0);
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
    
    uint32_t pulseTime = 0;

    // Generar pulso de 10 us en Trig
    GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x20); 
    SysCtlDelay(freq / 3000000);  // ~10 us
    GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x00); 

    // Esperar a que Echo se ponga en alto con timeout
    uint32_t timeout = 30000;
    while(!GPIOPinRead(GPIO_PORTB_BASE, 0x10) && timeout > 0) {
        timeout--;
    }
void LecSnsUlt(uint32_t *distance);
    if (timeout == 0) {
        *distance = 999; // no hubo pulso
        return;
    }

    // Medir cuánto tiempo está en alto con timeout
    pulseTime = 0;
    timeout = 60000;
    while(GPIOPinRead(GPIO_PORTB_BASE, 0x10) && timeout > 0) {
        pulseTime++;
        SysCtlDelay(freq / 3000000); // ~1 us
        timeout--;
    }

    if (timeout == 0) {
        *distance = 999;
    } else {
        *distance = pulseTime / 58;
    }

    if (*distance > 10){
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
    } else {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0x01);
    }
}