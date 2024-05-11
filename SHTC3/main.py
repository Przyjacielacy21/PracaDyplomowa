import machine, utime, asyncio, json
from mqtt_as import MQTTClient, config
import shtc3
import secret

# MQTT connection details
config['ssid'] = secret.ssid
config['wifi_pw'] = secret.wifi_passwd
config['server'] = secret.server
publish_topic = "/home/shtc3"
config_topic = "/config/shtc3"

# Pins for SHTC3, address 112
pin_scl = machine.Pin(27) #Phisical pin 32
pin_sda = machine.Pin(26) #Physical pin 31
i2c = machine.I2C(1, scl = pin_scl, sda = pin_sda)
sensor = shtc3.SHTC3(i2c)

internal_sensor = machine.ADC(4) # Internal temperature sensor is connected to ADC's channel 4
onboard_led = machine.Pin("LED", machine.Pin.OUT) # On Pico W/WH onboard led is connedted to wireless chip instead of RP2040

delay_for = 5

def ReadInternalSensorTemperature():
    conversion_factor = 3.3 / (65535)
    reading = internal_sensor.read_u16() * conversion_factor
    temperature = 27 - (reading - 0.706)/0.001721
    return round(temperature, 2)

def ReadMeasurements():
    temperature, rel_humidity = sensor.measurements
    internal_temperature = ReadInternalSensorTemperature()
    
    data = {
        "t": temperature,
        "internal_t": internal_temperature,
        "h": rel_humidity
    }   
    return data

def Blink(n):
    for _ in range(0, n):
        onboard_led.toggle()
        utime.sleep_ms(500)
        onboard_led.toggle()
        utime.sleep_ms(500)

async def messages(client):  # Respond to incoming messages
    async for topic, msg, retained in client.queue: 
        global delay_for
        
        if topic.decode() == config_topic:
            new_config = json.loads(msg)
            delay_for = new_config["delay"]
            Blink(2)
        else:
            print((topic.decode(), json.loads(msg), retained))
            Blink(1)
        
async def up(client):  # Respond to connectivity being (re)established
    while True:
        await client.up.wait()  # Wait on an Event
        client.up.clear()
        await client.subscribe(config_topic, 1) # renew subscriptions
        Blink(3)

async def main_mqtt_method(client):
    await client.connect()
    for coroutine in (up, messages):
        asyncio.create_task(coroutine(client))
    onboard_led.on()
    while True:
        await asyncio.sleep(delay_for)
        # If WiFi is down the following will pause for the duration.
        await client.publish(publish_topic, json.dumps(ReadMeasurements()), qos = 1)

config["queue_len"] = 1  # Use event interface with default queue size
client = MQTTClient(config)
onboard_led.off()
try:
    asyncio.run(main_mqtt_method(client))
finally:
    client.close()  # Prevent LmacRxBlk:1 errors