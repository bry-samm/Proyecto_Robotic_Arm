# Import standard python modules.
import sys
import time

# This example uses the MQTTClient instead of the REST client
from Adafruit_IO import MQTTClient
from Adafruit_IO import Client, Feed

# holds the count for the feed
run_count = 0

# Set to your Adafruit IO username and key.
# Remember, your key is a secret,
# so make sure not to publish it when you publish this code!
ADAFRUIT_IO_USERNAME = "bry_samm"
ADAFRUIT_IO_KEY = "aio_ZBWz33jGLDmpEcHrmSnbaetLnC5Q"

# Lists of feeds to subscribe to and send data to
FEEDS_RECEIVE = ['Servo_TX', 'Servo2_TX', 'Servo3_TX', 'Servo4_TX', 'Save_data']
FEEDS_SEND = ['Servo_RX', 'Servo2_RX', 'Servo3_RX', 'Servo4_RX']

# Mapping between RX and TX
FEED_MAP = {
    'Servo_TX': 'Servo_RX',
    'Servo2_TX': 'Servo2_RX',
    'Servo3_TX': 'Servo3_RX',
    'Servo4_TX': 'Servo4_RX'
}

# Define "callback" functions which will be called when certain events 
# happen (connected, disconnected, message arrived).
def connected(client):
    """Connected function will be called when the client is connected to
    Adafruit IO. This is a good place to subscribe to feed changes.
    """
    # Subscribe to all RX feeds
    for feed in FEEDS_RECEIVE:
        print('Subscribing to Feed {0}'.format(feed))
        client.subscribe(feed)
    print('Waiting for feed data...')

def disconnected(client):
    """Disconnected function will be called when the client disconnects."""
    sys.exit(1)

def message(client, feed_id, payload):
    """Message function will be called when a subscribed feed has a new value.
    The feed_id parameter identifies the feed, and the payload parameter has
    the new value.
    """
    print('Feed {0} received new value: {1}'.format(feed_id, payload))
    # Publish or "send" message to corresponding feed
    if feed_id in FEED_MAP:
        print('Sending data back: {0} to feed {1}'.format(payload, FEED_MAP[feed_id]))
        client.publish(FEED_MAP[feed_id], payload)

# Create an MQTT client instance.
client = MQTTClient(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)

# Setup the callback functions defined above.
client.on_connect = connected
client.on_disconnect = disconnected
client.on_message = message

# Connect to the Adafruit IO server.
client.connect()

# The first option is to run a thread in the background so you can continue
# doing things in your program.
client.loop_background()

while True:
    """
    # Uncomment the next 3 lines if you want to constantly send data
    # Adafruit IO is rate-limited for publishing
    # so we'll need a delay for calls to aio.send_data()
    run_count += 1
    print('sending count: ', run_count)
    client.publish(FEED_ID_Send, run_count)
    """
    print('Running "main loop" ')
    time.sleep(3)

