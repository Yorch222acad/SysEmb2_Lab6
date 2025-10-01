import RPi.GPIO as GPIO
import serial
from time import sleep

BtnBuzzer = 5
BtnMtr1 = 6
BtnMtr2 = 13
MdfDtCy = 19

GPIO.setmode(GPIO.BCM)
GPIO.setup(BtnBuzzer, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(BtnMtr1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(BtnMtr2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(MdfDtCy, GPIO.IN, pull_up_down=GPIO.PUD_UP)

ser = serial.Serial('/dev/ttyACM0', 9600)
ser.reset_input_buffer()

def ingDtCy():
    DtCy = int(input("Ingrese el valor del Duty Cycle entre 0 - 100: "))
    while DtCy < 0 or DtCy > 100:
        print("Valor incorrecto")
        DtCy = int(input("Ingrese el valor del Duty Cycle entre 0 - 100: "))
    return DtCy

while True:
    try:
        if GPIO.input(BtnBuzzer) == GPIO.LOW:
            ser.write(b"buzzer\n")
            print("enviado: buzzer")
            sleep(0.2)
        if GPIO.input(BtnMtr1) == GPIO.LOW:
            ser.write(b"motor1\n")
            print("enviado: motor1")
            sleep(0.2)
        if GPIO.input(BtnMtr2) == GPIO.LOW:
            ser.write(b"motor2\n")
            print("enviado: motor2")
            sleep(0.2)
        if GPIO.input(MdfDtCy) == GPIO.LOW:
            DtCy = ingDtCy()
            mensaje = "DutyCycle " + str(DtCy) + "\n"
            ser.write(mensaje.encode())
            print("enviado:", mensaje.strip())
            sleep(0.2)
        if ser.in_waiting > 0:
            value = ser.readline().decode('utf-8').rstrip()
            print("DutyCycle recibido ", value)

    except Exception as e:
        print(e)