# Importaciones:
#-----------------------------{ Mínimas
from machine import Pin, PWM
#}----------------------------{ Uart
import sys # Uart vía USB
import select # UART no bloqueante
#}----------------------------{ Otros
import time
#}--

#======================================================

frecuencia = 1000

# Configuraciones de pines:
#-------------------------{ Leds
led0 = machine.Pin("LED", machine.Pin.OUT) # Configura el Led integrado como salida
led1 = Pin(10,Pin.OUT) # Actividad Uart
led2 = Pin(11,Pin.OUT)
led3 = Pin(12,Pin.OUT)
led4 = Pin(13,Pin.OUT)
#}------------------------{ Buzzer
Buzzer = Pin(16,Pin.OUT)
#}------------------------{ PWM
Pwm1 = PWM(Pin(14))
Pwm2 = PWM(Pin(15))
# ------------------
Pwm1.freq(frecuencia)
Pwm2.freq(frecuencia)
#}--

# Inicializar PWMs apagados
Pwm1.duty_u16(0)
Pwm2.duty_u16(0)

# Configuración Uart no bloqueante
poll = select.poll()
poll.register(sys.stdin, select.POLLIN)

tIter = 0

#======================================================

def main():
    BuzzerState = False
    mtr1Stt = False
    mtr2Stt = False
    DutyValue = 20
    duty = int((DutyValue/100) * 65535)
    try:
        while True:
            led0.value(0)
            if poll.poll(0):
                led0.value(1)
                linea = sys.stdin.readline().strip()
                if linea == "buzzer":
                    led1.toggle()
                    Buzzer.value(1)
                    BuzzerState = True
                if linea == "motor1":
                    led1.toggle()
                    if mtr1Stt:
                        mtr1Stt = False
                        led2.value(0)
                    else:
                        mtr1Stt = True
                        led2.value(1)
                if linea == "motor2":
                    led1.toggle()
                    if mtr2Stt:
                        mtr2Stt = False
                        led3.value(0)
                    else:
                        mtr2Stt = True
                        led3.value(1)
            #-----------------------
            if BuzzerState:
                interactiveDelay(2.0)
                if tIter==0:
                    BuzzerState = False
                    Buzzer.value(0)
            #-----------------------
            if mtr1Stt:
                Pwm1.duty_u16(duty)
            else:
                Pwm1.duty_u16(0)
            if mtr2Stt:
                Pwm2.duty_u16(duty)
            else: 
                Pwm2.duty_u16(0)
            #-----------------------
            time.sleep(0.1)

    except Exception as e:
        led0.toggle()
        time.sleep(0.2)
        led0.toggle()
        time.sleep(0.2)

#======================================================

def interactiveDelay(time_sec):
    global tIter
    TotalTimeIter = int(time_sec*10)
    if tIter==0:
        tIter=TotalTimeIter
    time.sleep(0.1)
    tIter-=1

if __name__ == "__main__":
    main()