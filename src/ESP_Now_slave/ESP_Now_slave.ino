#include <WiFi.h>
#include <DHT.h>
#include <esp_now.h>
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_wifi.h"

// GPIO pin for the DHT sensor
#define DHTPIN 4
#define DHTTYPE DHT11


#define SENSOR_DATA_UPDATE 5
#define DATA_UPDATE_ID 6


// Structure to send data
typedef struct SensorData {
  uint8_t id;
  float temperature;
  float humidity;
  unsigned char sensorID[20];
  bool lowerThresholdExceeded = false;
  bool upperThresholdExceeded = false;
} SensorData;


// Structure to update data
typedef struct UpdateData {
  uint8_t id;
  unsigned long samplingInterval = 10000;
  unsigned long communicationInterval = 10000;
  float minThreshold = 0;
  float maxThreshold = 30;
} UpdateData;

// DHT sensor initialization
DHT dht(DHTPIN, DHTTYPE);

// Variables Declaration 
unsigned long samplingInterval = 10000;
unsigned long communicationInterval = 10000;
unsigned long minThreshold = 0;
unsigned long maxThreshold = 30;
unsigned long sendingToMaster = 0;

uint32_t startTime = 0;
uint32_t lastSampleTime = 0;
uint32_t lastCommunicationTime = 0;

float humidity = 0.0;
float temperature = 0.0;

bool canSleep = true;


esp_now_peer_info_t peerInfo = {};

// Master MAC Address
uint8_t masterAddress[] = { 0x08, 0xB6, 0x1F, 0x27, 0x63, 0x54 };

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status != ESP_NOW_SEND_SUCCESS) {
    canSleep = true;
  }
}

// Callback for data request
void OnDataReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  uint8_t id;
  memcpy(&id, data, 8);
  if (id == DATA_UPDATE_ID) {
    UpdateData updateData;
    memcpy(&updateData, data, sizeof(updateData));
    samplingInterval = updateData.samplingInterval;
    communicationInterval = updateData.communicationInterval;
    minThreshold = updateData.minThreshold;
    maxThreshold = updateData.maxThreshold;

    // //Checking Total time it took to recieve data
    // Serial.print("Total time taken for master to respond is: ");
    // Serial.print(millis() - sendingToMaster);
    // Serial.println("ms");
    // delay(5); //Extra Delay to ensure values print before going to sleep
    canSleep = true;
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.println("Slave Device Starting...");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT sensor initialized");

  // Set up WiFi in Station mode
  WiFi.mode(WIFI_STA);

  // Print this device's MAC address
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  // Print MAC addresses
  Serial.print("This device's MAC Address: ");
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized");

  // Register callbacks
  esp_now_register_recv_cb(OnDataReceive);
  esp_now_register_send_cb(OnDataSent);

  // Add master as a peer
  memcpy(peerInfo.peer_addr, masterAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Master added as peer successfully");
}

void loop() {

  startTime = millis();

  if (startTime - lastSampleTime >= samplingInterval) {
    // unsigned long sensorStart = millis();
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    // Serial.print("Sensing took: ");
    // Serial.print(millis() - sensorStart);
    // Serial.println("ms");
    lastSampleTime = startTime;
  }

  bool outOfThreshold = (temperature < minThreshold) || (temperature > maxThreshold);

  if (startTime - lastCommunicationTime >= communicationInterval || outOfThreshold) {
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Initial DHT sensor reading failed!");
    } else {
      Serial.print("Initial readings - Temperature: ");
      Serial.print(temperature);
      Serial.print("°C, Humidity: ");
      Serial.print(humidity);
      Serial.println("%");

      esp_wifi_start();
      SensorData sensorData;
      sensorData.humidity = humidity;
      sensorData.temperature = temperature;
      sensorData.id = SENSOR_DATA_UPDATE;
      sensorData.lowerThresholdExceeded = temperature < minThreshold;
      sensorData.upperThresholdExceeded = temperature > maxThreshold;
      String sensor_id = "sensor1";  // or sensor2 or sensor3
      sensor_id.toCharArray((char *)sensorData.sensorID, sizeof(sensorData.sensorID));


      // sendingToMaster = millis(); //check start time for sending to master
      esp_now_send(peerInfo.peer_addr, (const uint8_t *)&sensorData, sizeof(sensorData));
      canSleep = false;
      while (!canSleep) {
        delay(10);
      }
      esp_wifi_stop();
    }
    lastCommunicationTime = startTime;
  }
  // Mark End of cycle
  // unsigned long endCycleTime = millis();
  // Serial.print("Total end cycle time: ");
  // Serial.print(millis() - endCycleTime);
  // Serial.println("ms");

  Serial.println("Going to sleep");
  uint32_t timeToNextSample = samplingInterval - (startTime - lastSampleTime);
  uint32_t timeToNextCommunication = communicationInterval - (startTime - lastCommunicationTime);

  // Find the minimum time to sleep
  uint32_t timeToSleep = min(timeToNextSample, timeToNextCommunication);

  // Enter sleep if the time to sleep is greater than 10 ms
  if (timeToSleep > 10) {
    esp_sleep_enable_timer_wakeup((uint64_t)timeToSleep * 1000);
    esp_light_sleep_start();
  }
}