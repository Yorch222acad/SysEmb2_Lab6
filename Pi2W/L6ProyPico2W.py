# Importaciones:
#-----------------------------{ Mínimas
from machine import Pin, PWM
#}----------------------------{ Uart
import sys # Uart vía USB
import select # UART no bloqueante
#}----------------------------{ Otros
import utime
#}--

#======================================================

# Variables globales:
tIter = 0
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
#}------------------------{ Ultrasonico
trig = Pin(18, Pin.OUT)
echo = Pin(19, Pin.IN)
#}--

# Configuración Uart no bloqueante
poll = select.poll()
poll.register(sys.stdin, select.POLLIN)

#======================================================

def main():

    # Variables locales:
    #----------------------{ Buzzer
    BuzzerState = False
    #}---------------------{ Motores
    mtr1Stt = False
    mtr2Stt = False
    DutyValue = 20
    duty = int((DutyValue/100) * 65535)
    #}---------------------{ Ultrasonico

    try:

        while True:

            #======================================================

            # Medición de distancia con ultrasonico con timeout
            trig.on()
            utime.sleep_ms(10)
            trig.off()

            timeout = 30000  # microsegundos (30 ms)
            inicio = None
            final = None

            # Espera que echo suba a 1
            start_tick = utime.ticks_us()
            while echo.value() == 0:
                if utime.ticks_diff(utime.ticks_us(), start_tick) > timeout:
                    print("Timeout esperando inicio del eco")
                    break
                inicio = utime.ticks_us()

            # Solo continuar si se detectó el inicio
            if inicio is not None:
                start_tick = utime.ticks_us()
                while echo.value() == 1:
                    if utime.ticks_diff(utime.ticks_us(), start_tick) > timeout:
                        break
                    final = utime.ticks_us()

            # Calcular distancia solo si se midió correctamente
            if inicio is not None and final is not None:
                tiempo = final - inicio
                distance = (tiempo * 0.0343) / 2  # velocidad del sonido en cm/us
                
            else:
                print("No se pudo medir la distancia")

            utime.sleep_ms(200)

            #======================================================
            led0.value(0)
            #UART
            BuzzerState, mtr1Stt, mtr2Stt = UartHandler(BuzzerState, mtr1Stt, mtr2Stt)
            #-----------------------
            if BuzzerState:
                if interactiveDelay(2.0):
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
            if distance < 10:
                led4.value(1)
            else:
                led4.value(0)

    except Exception as e:
        led0.toggle()
        utime.sleep_ms(200)
        led0.toggle()
        utime.sleep_ms(200)

#======================================================

def interactiveDelay(time_sec):
    global tIter
    if tIter == 0:
        # Guardar el instante de inicio en milisegundos
        tIter = utime.ticks_add(utime.ticks_ms(), int(time_sec * 1000))
    # Verificar si ya pasó el tiempo deseado
    if utime.ticks_diff(tIter, utime.ticks_ms()) <= 0:
        tIter = 0
        return True  # señal de que el tiempo terminó
    return False

#-----------------------------------------------------------------------

def UartHandler(BuzzerState, mtr1Stt, mtr2Stt):
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
    return BuzzerState, mtr1Stt, mtr2Stt

if __name__ == "__main__":
    main()
