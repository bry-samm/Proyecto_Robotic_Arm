import sys
import time
import serial

# Conectarse a Adafruit IO
from Adafruit_IO import MQTTClient
from Adafruit_IO import Client, Feed

# COntador
run_count = 0

# Usuario y clave para conectarse a la cuenta de Adafruit IO
ADAFRUIT_IO_USERNAME = 
ADAFRUIT_IO_KEY = 

# Abrimos la conexión con el Arduino usando el puerto COM10
com_arduino = serial.Serial(port='COM10', baudrate=9600, timeout=0.1)

# Feeds desde los que recibimos datos 
FEEDS_RECEIVE = ['Servo_TX', 'Servo2_TX', 'Servo3_TX', 'Servo4_TX', 'Save_data']

# Feeds a los que enviamos datos 
FEEDS_SEND = ['Servo_RX', 'Servo2_RX', 'Servo3_RX', 'Servo4_RX']

# Se guarda los últimos valores recibidos de cada servomotor, aqui hay unos valores iniciales
latest_values = {
    'Servo_TX': '90',
    'Servo2_TX': '90',
    'Servo3_TX': '90',
    'Servo4_TX': '90'
}

# Esta función se ejecuta cuando se conecta a Adafruit IO
def connected(client):
    for feed in FEEDS_RECEIVE:
        print('Subscribing to Feed {0}'.format(feed))  # Suscripción a cada feed
        client.subscribe(feed)
    print('Waiting for feed data...')  # Mensaje para indicar que ya está esperando datos

# Esta función se ejecuta si se pierde la conexión con Adafruit IO (cuando termino de correr el programa)
def disconnected(client):
    sys.exit(1)  # Cierra el programa

# Esta función se ejecuta cada vez que llega un nuevo dato desde algún feed
def message(client, feed_id, payload):
    print(f'Feed {feed_id} received new value: {payload}')  # Muestra el feed y el valor recibido

    # Solo se actualiza latest_values si el dato viene de un feed de control
    if feed_id in latest_values and feed_id != 'Save_data':
        latest_values[feed_id] = payload  # Guardamos el nuevo valor en lastes_values esto con el fin de tener una copia de los valores y trabajar con estos
                                        # así se evitan problemas de valores no deseados y se puede cargar los valores simultáneamente a los motores

    # Si llega una señal desde 'Save_data' con valor 1, se mandan los datos al Arduino
    if feed_id == 'Save_data' and payload == '1':
        print("Save_data activado. Enviando datos por serial...")

        # Se muestran los valores que se van a mandar
        print("Valores actuales de latest_values:", latest_values)

        # Se guarda cada valor por separado
        val1 = latest_values['Servo_TX']
        val2 = latest_values['Servo2_TX']
        val3 = latest_values['Servo3_TX']
        val4 = latest_values['Servo4_TX']

        # Se concatenan los valores separados por comas para analizarlo en el programa en C
        data_string = f"{val1},{val2},{val3},{val4}\n"

        print(f"Enviando al Arduino: {data_string.strip()}")  # Mostramos lo que se va a enviar
        com_arduino.write(data_string.encode())  # Enviamos la cadena al Arduino

        time.sleep(0.1)  # Se espera un poco para que el Arduino tenga tiempo de responder

        feedback = com_arduino.readline().decode().strip()  # Se lee lo que nos manda el Arduino
        print(f"Feedback recibido: {feedback}")  # Mostramos el mensaje recibido

        feedback_values = feedback.split(',')  # Dividir el mensaje en una lista usando la coma

        # Se publica la respuesta del microcontrolador en Adafruit IO (feedback)
        if len(feedback_values) == 4:
            client.publish('Servo_RX', feedback_values[0])
            client.publish('Servo2_RX', feedback_values[1])
            client.publish('Servo3_RX', feedback_values[2])
            client.publish('Servo4_RX', feedback_values[3])

            # También actualizamos latest_values con la respuesta real del Arduino
            latest_values['Servo_TX'] = feedback_values[0]
            latest_values['Servo2_TX'] = feedback_values[1]
            latest_values['Servo3_TX'] = feedback_values[2]
            latest_values['Servo4_TX'] = feedback_values[3]

            print("latest_values actualizado con datos del Arduino:", latest_values)
        else:
            print("⚠️ Feedback inválido recibido. No se actualizaron los valores.")  # Si algo falla, esto es como un try catch

# Creamos el cliente que se conecta a Adafruit IO usando MQTT
client = MQTTClient(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)
client.on_connect = connected
client.on_disconnect = disconnected
client.on_message = message
# Iniciamos la conexión con Adafruit IO
client.connect()
client.loop_background()

# Bucle principal que se ejecuta sin parar
while True:
    print('Running "main loop" ')  # se muestra un mensaje cada vez que el bucle corre
    time.sleep(3)  # Esperamos 3 segundos antes de volver a empezar el bucle

