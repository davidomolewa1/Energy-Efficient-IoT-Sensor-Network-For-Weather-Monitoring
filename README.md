# Energy-Efficient-IoT-Sensor-Network-For-Weather-Monitoring
# ESP32 Master-Slave Remote Sensing System

This project implements a remote sensing system using ESP32 microcontrollers, where a master device communicates with up to three slave devices to collect temperature and humidity data. The slaves use DHT sensors for data acquisition, and communication is achieved via ESP-NOW and MQTT protocols.

## Features
- Master ESP32 collects data from up to three slaves.
- Slaves measure temperature and humidity using DHT11 sensors.
- Threshold-based alerts for temperature and humidity levels.
- ESP-NOW for communication between master and slaves.
- MQTT integration for publishing sensor data to a broker.
- Power-saving features for optimized operation.

## Components
### Hardware
- ESP32 microcontrollers (1 master, 3 slaves).
- DHT11 sensors (1 per slave).
- USB cables for power and programming.

### Software
- Arduino IDE (with required ESP32 board support).
- Python (for the MQTT and Serial interface).
- Libraries:
  - `WiFi.h`
  - `esp_now.h`
  - `ArduinoJson.h`
  - `DHT.h`
  - `paho-mqtt`
  - `pyserial`

## System Architecture
1. **Master ESP32**:
   - Acts as the central hub for receiving data from slaves.
   - Sends and receives commands to/from slaves via ESP-NOW.
   - Publishes sensor data to the MQTT broker.

2. **Slave ESP32**:
   - Collects temperature and humidity data from the DHT11 sensor.
   - Monitors thresholds and sends alerts to the master.
   - Operates in a power-saving mode between sampling intervals.

3. **Python Script**:
   - Interfaces with the MQTT broker for receiving configuration updates.
   - Listens to the USB port of the PC where the Master is connected to .
   - Publishes collected data back to the broker.

## Setup Instructions
### Hardware Setup
1. Connect each DHT11 sensor to its respective ESP32 slave:
   - Data pin to GPIO4 (configurable in code).
   - VCC to 3.3V.
   - GND to GND.
2. Note the MAC addresses of all ESP32 devices. Replace these in the code's `slaveAddress` array and `masterAddress` variable.
3. Power the ESP32 devices via USB or external power supply.

### Software Setup
#### ESP32 Master
1. Open the provided master code in the Arduino IDE.
2. Install required libraries (`WiFi`, `esp_now`, `ArduinoJson`).
3. Update the `masterAddress` and `slaveAddress` variables with the respective MAC addresses.
4. Upload the code to the master ESP32.

#### ESP32 Slaves
1. Open the provided slave code in the Arduino IDE.
2. Install required libraries (`WiFi`, `esp_now`, `DHT`, `ArduinoJson`).
3. Update the `masterAddress` variable with the master’s MAC address.
4. Upload the code to each slave ESP32.

#### Python Script
1. Install Python dependencies using:
   ```bash
   pip install paho-mqtt pyserial
   ```
2. Update the `SERIAL_PORT` and `MQTT_BROKER` in the script.
3. Run the script to start MQTT communication and Serial monitoring.

### MQTT Configuration
- Topics for interaction:
  - Sampling Interval: `d:/group5.iot/sampling_interval`
  - Communication Interval: `d:/group5.iot/communication_interval`
  - Minimum Threshold: `d:/group5.iot/minimum_threshold`
  - Maximum Threshold: `d:/group5.iot/maximum_threshold`
  - Sensor Data: `d:/group5.iot/status`

## Usage
1. Power on the ESP32 devices.
2. Start the Python script to establish Serial and MQTT communication.
3. Use the MQTT broker to configure sampling/communication intervals and thresholds.
4. Monitor sensor data and alerts via MQTT or Serial logs.

## Key Features in Code
### ESP32 Master
- Listens to slaves for sensor data.
- Publishes received data to the MQTT broker.
- Updates thresholds and intervals as received from MQTT.

### ESP32 Slave
- Measures temperature and humidity using the DHT11 sensor.
- Monitors against configured thresholds and sends alerts if exceeded.
- Sends regular updates to the master based on the sampling interval.

### Python Script
- Handles MQTT subscriptions and publishes configuration updates.
- Reads Serial data from the master and relays it to the MQTT broker.
- Provides fallback mechanisms for invalid or unexpected inputs.

## Debugging and Logs
- **Serial Logs**:
  - Use the Arduino Serial Monitor or any Serial interface to view logs from the ESP32 devices.
- **Python Script Logs**:
  - Displays received MQTT messages and Serial interactions.
- **MQTT Broker**:
  - Use an MQTT client to subscribe to topics and view published data.

## Future Enhancements
- Integration with cloud platforms for advanced analytics.
- Support for additional sensor types.
- Enhanced power optimization for battery-powered deployments.

---

### Note
This project assumes basic familiarity with ESP32 programming, Arduino IDE, and MQTT. For troubleshooting and detailed explanations, consult the [ESP32 documentation](https://docs.espressif.com/) and [Arduino reference](https://www.arduino.cc/reference/en/).

