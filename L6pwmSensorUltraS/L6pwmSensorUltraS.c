#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"

int freq = 120000000;

void LecSnsUlt(int *distance);
void CntMtr(int distance, uint32_t *ui32Load);

int main(void)
{
    int distance = 0;
    uint32_t ui32PWMClock, ui32Load;

    // Configurar reloj del sistema
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                        SYSCTL_OSC_MAIN |
                        SYSCTL_USE_PLL |
                        SYSCTL_CFG_VCO_240), 
                        freq);

    ////////////////////////////////////////
    // Periféricos
    ////////////////////////////////////////
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  // HC-SR04 Trig/Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);  // LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  // PWM pin
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);  // Dirección motor
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    ////////////////////////////////////////
    // Configuración LED
    ////////////////////////////////////////
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, 0x01);  // LED indicador

    ////////////////////////////////////////
    // Configuración HC-SR04
    ////////////////////////////////////////
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0x20); // PB5 -> Trig
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, 0x10);  // PB4 -> Echo

    ////////////////////////////////////////
    // Configuración Motor
    ////////////////////////////////////////
    // Dirección motor (PH2)
    GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_PIN_2); // Sentido fijo

    // PWM en PF1
    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);

    ui32PWMClock = freq / 64;  // Prescaler
    ui32Load = (ui32PWMClock / 1000) - 1; // Frecuencia ~1kHz

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ui32Load);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);

    ////////////////////////////////////////
    // Bucle principal
    ////////////////////////////////////////
    while(1)
    {
        LecSnsUlt(&distance);
        CntMtr(distance, &ui32Load);
    }
}

//============================================================================

void LecSnsUlt(int *distance){
    int pulseTime = 0;

    // Pulso de Trig (10 us)
    GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x20);
    SysCtlDelay(freq / 3000000);  // ~10us
    GPIOPinWrite(GPIO_PORTB_BASE, 0x20, 0x00);

    // Esperar a que Echo suba
    while(!GPIOPinRead(GPIO_PORTB_BASE, 0x10)) {}

    // Medir tiempo mientras Echo esté en alto
    while(GPIOPinRead(GPIO_PORTB_BASE, 0x10))
    {
        pulseTime++;
        SysCtlDelay(freq / 3000000); // ~1us
    }

    // Distancia en cm
    *distance = pulseTime / 58;    
}
//-----------------------------------------------------------------
void CntMtr(int distance, uint32_t *ui32Load){
    if(distance>6) {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, *ui32Load * 0.5);
    }
    else if(distance>5) {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, *ui32Load * 0.3);
    }
    else if(distance>4) {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, *ui32Load * 0.2);
    }
    else if(distance>3) {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, *ui32Load * 0.1);
    }
    else {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x01, 0x01);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    }
}