import serial

# Crear comunicación serial con el Arduino
com_arduino = serial.Serial(port='COM10', baudrate=9600, timeout=0.1)

while True:
    try:
        valor = int(input("Ingrese un valor entre 0 y 180: "))
        if 0 <= valor <= 180:
            com_arduino.write(f"{valor}\n".encode())  # Enviar como cadena terminada en '\n'
        else:
            print("⚠️ El valor debe estar entre 0 y 180.")
    except ValueError:
        print(" Entrada inválida. Ingrese un número.")