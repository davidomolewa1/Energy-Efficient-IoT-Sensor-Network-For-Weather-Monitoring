import threading
import serial
import json
import paho.mqtt.client as mqtt
import time

# MQTT Configuration
MQTT_BROKER = "mqtt.eclipseprojects.io"  # Replace with your broker address
MQTT_PORT = 1883
MQTT_SAMPLING_INTERVAL_TOPIC = "d:/group5.iot/sampling_interval"
MQTT_COMMUNICATION_INTERVAL_TOPIC = "d:/group5.iot/communication_interval"
MQTT_MINIMUM_THRESHOLD_TOPIC = "d:/group5.iot/minimum_threshold"
MQTT_MAXIMUM_THRESHOLD_TOPIC = "d:/group5.iot/maximum_threshold"
MQTT_PUBLISH_TOPIC = "d:/group5.iot/status"

# Serial Configuration
SERIAL_PORT = "/dev/cu.usbserial-0001"  # Replace with your serial port
BAUD_RATE = 115200

# MQTT Client
mqtt_client = mqtt.Client()

# Serial Port (shared between threads)
serial_connection = None

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(MQTT_SAMPLING_INTERVAL_TOPIC)
        client.subscribe(MQTT_COMMUNICATION_INTERVAL_TOPIC)
        client.subscribe(MQTT_MINIMUM_THRESHOLD_TOPIC)
        client.subscribe(MQTT_MAXIMUM_THRESHOLD_TOPIC)
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    global serial_connection
    try:
        message = msg.payload.decode('utf-8')
        data = json.loads(message)
        print(f"Received MQTT message on topic {msg.topic}: {message}")

        if msg.topic == MQTT_SAMPLING_INTERVAL_TOPIC:
            interval = data["interval"]
            data["interval"] = 5 if interval < 5 else interval
            data["id"] = 1
        elif msg.topic == MQTT_COMMUNICATION_INTERVAL_TOPIC:
            interval = data["interval"]
            data["interval"] = 5 if interval < 5 else interval
            data["id"] = 2
        elif msg.topic == MQTT_MINIMUM_THRESHOLD_TOPIC:
            data["id"] = 3
        elif msg.topic == MQTT_MAXIMUM_THRESHOLD_TOPIC:
            data["id"] = 4
        else:
            data["id"] = -1

        modified_message = json.dumps(data)

        if serial_connection and serial_connection.is_open:
            serial_connection.write((modified_message + "\n").encode('utf-8'))
            print(f"Sent to Serial: {modified_message}")


    except Exception as e:
        print(f"Error processing MQTT message: {e}")


def read_serial():
    global serial_connection
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            serial_connection = ser
            print("Listening to serial port...")
            while True:
                if ser.in_waiting > 0:
                    raw_line = ser.readline()
                    try:
                        line = raw_line.decode('utf-8', errors='ignore').strip()
                        data = json.loads(line)

                        if (data.get("id") == 5): 
                            del data["id"]
                            data["timestamp"] = int(time.time())
                            mqtt_client.publish(MQTT_PUBLISH_TOPIC, json.dumps(data))
                            print(f"Published to MQTT: {data}")

                        else:
                            print(data)

                    except json.JSONDecodeError as e:
                        print(f"JSON Decode Error: {e}. Raw Data: {raw_line}")

    except serial.SerialException as e:
        print(f"Serial Communication Error: {e}")

# MQTT Listener
def start_mqtt():
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_forever()
    except Exception as e:
        print(f"MQTT Error: {e}")

# Main
if __name__ == "__main__":
    # Create threads
    serial_thread = threading.Thread(target=read_serial, daemon=True)
    mqtt_thread = threading.Thread(target=start_mqtt, daemon=True)

    # Start threads
    serial_thread.start()
    mqtt_thread.start()

    # Keep the main thread alive
    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("\nExiting program...")
