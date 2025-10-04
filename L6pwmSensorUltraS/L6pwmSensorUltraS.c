/* LEEME
	Laboratorio 6 - 2da Fase Proyecto

	Hardware - Pines usados:
	- PN0: LED indicador sensor ultrasónico
	- PN1: LED indicador motores
	- PN2: Buzzer
	- PF0: LED indicador actividad UART
	- PF4: LED indicador recepción comando UART
	- PA0: UART0 RX (USB)
	- PA1: UART0 TX (USB)
	- PB4: Echo UltraSónico
	- PB5: Trig UltraSónico
	- PH0: Motor1 PWM
	- PH1: Motor1 Dirección
	- PH2: Motor2 PWM
	- PH3: Motor2 Dirección

	Descripción:
	Programa para la placa EK-TM4C1294XL que realiza las siguientes funciones:
	- Escucha comandos por UART. Si recibe "buzzer", enciende el buzzer (PN2) durante 2 segundos.
	- El LED PF4 indica la actividad del UART (se enciende al recibir un comando).
	- El LED PF0 parpadea cada vez que se recibe un comando por UART.
	- Controla dos motores DC mediante PWM y dirección. Los motores se activan/desactivan con mensajes recibidos por UART.
	  Si la distancia medida por el sensor ultrasónico es menor a 10 cm, los motores funcionan a una velocidad reducida.
	  Si la distancia es mayor o igual a 10 cm, los motores funcionan a velocidad normal.
	- El sensor ultrasónico mide la distancia al obstáculo más cercano y enciende el LED PN1 si la distancia es menor a 10 cm.
*/

// Librerías:
//-----------------------------{ Mínimas
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
//}----------------------------{ Uart
#include "driverlib/uart.h"
#include "utils/uartstdio.c"
//}----------------------------{ Adicionales
#include "string.h"
#include "driverlib/pin_map.h"
//}----------------------------{ PWM libraries
#include "driverlib/pwm.h"
//}--

// ======================================================

// Variables globales:
int freq = 120000000;
volatile uint32_t ui32Loop;

// Prototipos de funciones:
void LecSnsUlt(uint32_t *distance);
void checkUART(char rxBuffer[10], int *rxIndex);
void interactiveDelay(float time_sec, int *tIter);
void toogleUart(bool *ledUart);
void CnvIntToStr(int DtCy, char *strDtCy);
int CnvStrToInt(char *rxBuffer);

// =======================================================

int main(void)
{
	// Variables main:
	//-------------------------{ Uart 
	char rxBuffer[10];
	int rxIndex = 0;
	bool ledUart = true;
	//}------------------------{ UART Str-Int
	bool waitingDuty = false;
	int DtCy;
	char strDtCy[4];
	//}------------------------{ InteractiveDelay 
	int tIter = 0;
	float time_sec = 2.0;
	//}------------------------{ Motores PWM
	uint32_t ui32Period;
	uint32_t distance = 0;
	bool mtr1Stt = false;
	bool mtr2Stt = false;
	//}------------------------{ Buzzer
	bool BuzzerState = false;
	//}--

	SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ  // Configuración del reloj de la placa
						| SYSCTL_OSC_MAIN
						| SYSCTL_USE_PLL 
						| SYSCTL_CFG_VCO_480),freq); 

	// Habilitar perifericos:
	//---------------------------------------------{ Uart 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	//}--------------------------------------------{ GPIO 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	//}--------------------------------------------{ UltraSónico
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  
	//}--------------------------------------------{ Motores PWM
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	//}--
    
	// Verificar perifericos:
	//-----------------------------------------------------{ Uart 
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))  {}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))  {}
	//-----------------------------------------------------{ GPIO 
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ))  {}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))  {}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))  {}
	//}----------------------------------------------------{ UltraSónico
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))  {}
	//}----------------------------------------------------{ Motores PWM
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0))  {}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH))  {}
	//}--

	// Configurar pines 
	//--------------------------------------------------------------{ GPIO Outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, 0x07); // PN0,PN1,PN2
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0x11);	// PF0,PF4
	//}-------------------------------------------------------------{ GPIO Inputs
	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, 0x03); // PJ0, PJ1
	GPIOPadConfigSet(GPIO_PORTJ_BASE, 0x03, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	//}-------------------------------------------------------------{ Uart
	GPIOPinConfigure(GPIO_PA0_U0RX); // USB
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, 0x03);
	//}-------------------------------------------------------------{ UltraSónico
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0x20); // Trig
	GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, 0x10);  // Echo
	//}-------------------------------------------------------------{ Motores PWM
	GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, 0x0f);
	//}--
    

	// Configurar PWM: --{=>
	// | _ PWM 1: --[=>
	GPIOPinConfigure(GPIO_PF2_M0PWM2); // F2 - PWM2
	GPIOPinTypePWM(GPIO_PORTF_BASE, 0x04);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC); //Generador PWM1 modo up/down

	ui32Period = freq / 25000;// Periodo del PWM: 240 kHz || Dutty cycle de 4800
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ui32Period);
	PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);

	// | _ <=]-- PWM 2: --[=>
	GPIOPinConfigure(GPIO_PF1_M0PWM1); // F1 - PWM1
	GPIOPinTypePWM(GPIO_PORTF_BASE, 0x02);

	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);	//Generador PWM2 modo up/down
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ui32Period);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	// | _ <=]--
	// <=}--
	
	UARTStdioConfig(0, 9600, freq); // Configuración UART

	// Inicialización
	GPIOPinWrite(GPIO_PORTH_BASE, 0x0f, 0x00);
	GPIOPinWrite(GPIO_PORTN_BASE, 0x03, 0);  // Apaga PN0 y PN1

	while(1)
	{
		LecSnsUlt(&distance);
		checkUART(rxBuffer, &rxIndex);
		// Apagarlos para que funcionen como toogle
		GPIOPinWrite(GPIO_PORTN_BASE, 0x02, 0);
		GPIOPinWrite(GPIO_PORTF_BASE, 0x01, 0);
		// Al llegar los mensajes por UART activar las flags: --{=>
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
		// <=}-- Recibir characteres para DutyCycle: --{=>
		if (strncmp(rxBuffer, "DutyCycle", 9) == 0) {
			waitingDuty = true;
			//memset(rxBuffer, 0, sizeof(rxBuffer));
		}
		if (waitingDuty == true) {
			waitingDuty = false;
			int DtCy = CnvStrToInt(rxBuffer);
			CnvIntToStr(DtCy, strDtCy);
			UARTprintf("%s\n", strDtCy);
			UARTprintf("motor1\n");
			toogleUart(&ledUart);
			memset(rxBuffer, 0, sizeof(rxBuffer));
		}
		// <=}-- Ejecución buzzer, motores y sensor: --{=>
		// | _ Buzzer: --[=>
		if (BuzzerState == true) {
			interactiveDelay(2.0, &tIter);
			if (tIter == 0) {
				BuzzerState = false;
				GPIOPinWrite(GPIO_PORTN_BASE, 0x04, 0);
			}
		}
		// | _ <=]-- Motores y sensor: --[=>
		// | _ | _ Activar - Desactivar pines de dirección: --(=>
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
		// | _ | _ <=)-- Activar - Desactivar PWM: --(=>
		if(distance < 10){ // menos de 10 cm
			if (mtr1Stt == true) {
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,4000);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x01, 0x01);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x02, 0x00);
			}
			if (mtr2Stt == true) {
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,4000);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x08, 0x08);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x04, 0x00);
			}
		}
		else{
			if (mtr1Stt == true) {
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 4000);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x01, 0x00);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x02, 0x02);
			}
			if (mtr2Stt == true) {  
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,4000);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x08, 0x00);
			GPIOPinWrite(GPIO_PORTH_BASE, 0x04, 0x04);
			}
		}
		// | _ | _ <=)--
		// | _ <=]--
		// <=}--
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

//------------------------------------------------------------------

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

//------------------------------------------------------------------

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