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

# Pines del ultrasonico
trig = Pin(18, Pin.OUT)
echo = Pin(19, Pin.IN)


# Inicializar PWMs apagados
Pwm1.duty_u16(0)
Pwm2.duty_u16(0)

# Configuración Uart no bloqueante
poll = select.poll()
poll.register(sys.stdin, select.POLLIN)

tIter = 0

#======================================================

def main():
    distance = LectrUltrasonico()

    if distance == -1:
        print("Error: no se pudo medir distancia")
    BuzzerState = False
    mtr1Stt = False
    mtr2Stt = False
    DutyValue = 20
    duty = int((DutyValue/100) * 65535)

    # Tiempo máximo de espera en microsegundos
    timeout = 30000  # 30 ms

    try:
        while True:
            distance = LectrUltrasonico()
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
    TotalTimeIter = int(time_sec*100)
    if tIter==0:
        tIter=TotalTimeIter
    utime.sleep_ms(10)
    tIter-=1

if __name__ == "__main__":
    main()

#======================================================
def LectrUltrasonico():
    # Medición de distancia con ultrasonico con timeout
    trig.off()
    utime.sleep_us(2)
    trig.on()
    utime.sleep_us(10)
    trig.off()

    timeout = 30000  # microsegundos (30 ms)
    inicio = None
    final = None

    # Espera que echo suba a 1 (inicio del eco)
    start_tick = utime.ticks_us()
    while echo.value() == 0:
        if utime.ticks_diff(utime.ticks_us(), start_tick) > timeout:
            return -1  # No se detectó eco
        inicio = utime.ticks_us()

    # Espera que echo baje a 0 (fin del eco)
    start_tick = utime.ticks_us()
    while echo.value() == 1:
        if utime.ticks_diff(utime.ticks_us(), start_tick) > timeout:
            return -1  # El pulso duró demasiado
        final = utime.ticks_us()

    if inicio is None or final is None:
        return -1  # Error de sincronización

    tiempo = final - inicio
    distancia = (tiempo * 0.0343) / 2  # en cm
    utime.sleep_ms(200)

    return distancia
