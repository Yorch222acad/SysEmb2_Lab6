# Librerías:
import serial # Comunicación Serial
from time import sleep # Retardo

# Configuración UART:
try:
    # Ajusta el puerto COM según tu sistema
    # En Windows suele ser 'COM1', 'COM2', etc
    # En Linux/Mac suele ser '/dev/ttyUSB0' o '/dev/ttyACM0'
    ser = serial.Serial('/dev/ttyACM0', 9600)
    ser.reset_input_buffer()
except Exception as e:
    print(f"Error al abrir puerto serial: {e}")
    exit()

# Función para ingresar el Duty Cycle por terminal:
def ingDtCy():
    while True:
        try:
            DtCy = int(input("Ingrese el valor del Duty Cycle entre 0 - 100: "))
            if 0 <= DtCy <= 100:
                return DtCy
            print("Valor incorrecto")
        except ValueError:
            print("Por favor ingrese un número válido")

def mostrar_menu():
    print("\n--- Menú de Control ---")
    print("1. Activar Buzzer")
    print("2. Activar Motor 1")
    print("3. Activar Motor 2")
    print("4. Modificar Duty Cycle")
    print("5. Salir")

while True:
    try:
        mostrar_menu()
        opcion = input("\nSeleccione una opción (1-5): ")

        if opcion == '1':
            ser.write(b"buzzer\n")
            print("enviado: buzzer")
        elif opcion == '2':
            ser.write(b"motor1\n")
            print("enviado: motor1")
        elif opcion == '3':
            ser.write(b"motor2\n")
            print("enviado: motor2")
        elif opcion == '4':
            DtCy = ingDtCy()
            mensaje = f"DutyCycle {DtCy}\n"
            ser.write(mensaje.encode())
            print("enviado:", mensaje.strip())
        elif opcion == '5':
            print("Cerrando programa...")
            ser.close()
            break
        
        # Esperar y mostrar respuesta si hay
        sleep(0.2)
        if ser.in_waiting > 0:
            value = ser.readline().decode('utf-8').rstrip()
            print("DutyCycle recibido:", value)

    except Exception as e:
        print(f"Error: {e}")