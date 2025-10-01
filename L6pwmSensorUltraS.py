import RPi.GPIO as GPIO
import serial
from time import sleep

BtnBuzzer = 5
BtnMtr1 = 6
BtnMtr2 = 13

GPIO.setmode(GPIO.BCM)
GPIO.setup(BtnBuzzer, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(BtnMtr1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(BtnMtr2, GPIO.IN, pull_up_down=GPIO.PUD_UP)

ser = serial.Serial('/dev/ttyACM0', 9600)
ser.reset_input_buffer()

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
    except Exception as e:
        print(e)
