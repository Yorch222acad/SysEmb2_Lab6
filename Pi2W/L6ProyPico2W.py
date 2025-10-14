# Importaciones:
#-----------------------------{ Mínimas
from machine import Pin
#}----------------------------{ Uart
import sys # Uart vía USB
import select # UART no bloqueante
#}----------------------------{ Otros
import time
#}--

#======================================================

# Configuraciones de pines:
#-------------------------{ Leds
led1 = Pin(10,Pin.OUT)
led2 = Pin(11,Pin.OUT)
led3 = Pin(12,Pin.OUT)
led4 = Pin(13,Pin.OUT)
#}------------------------{ Buzzer
Buzzer = Pin(16,Pin.OUT)
#}--

# Configuración Uart no bloqueante
poll = select.poll()
poll.register(sys.stdin, select.POLLIN)

#======================================================

if __name__ == "__main__":

    try:
        while True:
            led1.value(0)
            if poll.poll(0):
                led1.value(1)
                linea = sys.stdin.readline().strip()
                if linea == "buzzer":
                    Buzzer.toggle()
            time.sleep(0.1)

    except Exception as e:
        led1.value(0)
        led1.value(0)
        #--------------
        led1.toggle()
        led2.toggle()
        time.sleep(0.2)
        led1.toggle()
        led2.toggle()
        time.sleep(0.2)