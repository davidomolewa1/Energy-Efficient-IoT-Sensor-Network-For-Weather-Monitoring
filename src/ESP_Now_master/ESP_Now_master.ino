#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include "esp_mac.h"


#define MAX_PEERS 3

#define SAMPLING_INTERVAL_UPDATE_ID 1
#define COMMUNICATION_INTERVAL_UPDATE_ID 2
#define MINIMUM_THRESHOLD_UPDATE_ID 3
#define MAXIMUM_THRESHOLD_UPDATE_ID 4
#define SENSOR_DATA_UPDATE 5
#define DATA_UPDATE_ID 6


esp_now_peer_info_t peerInfo[MAX_PEERS];


typedef struct UpdateData {
  uint8_t id = DATA_UPDATE_ID;
  unsigned long samplingInterval = 10000;
  unsigned long communicationInterval = 10000;
  float minThreshold = 0;
  float maxThreshold = 30;
} UpdateData;


UpdateData currentData;

// Structure example to send data
typedef struct SensorData {
  uint8_t id;
  float temperature;
  float humidity;
  unsigned char sensorID[20];
  bool lowerThresholdExceeded = false;
  bool upperThresholdExceeded = false;
} SensorData;



// Set this to the MAC address of your slave device
uint8_t slaveAddress[MAX_PEERS][6] = {
  { 0x08, 0xB6, 0x1F, 0x27, 0x71, 0x20 },  // Peer 1 MAC Address
  { 0x08, 0xB6, 0x1F, 0x28, 0x65, 0x10 },  // Peer 1 MAC Address
  { 0x08, 0xB6, 0x1F, 0x29, 0xD3, 0xA0 },  // Peer 2 MAC Address
  // { 0x24, 0x6F, 0x28, 0xAE, 0xB1, 0xC4 }   // Peer 3 MAC Address

  //08:B6:1F:28:65:10
};

// Function declarations
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len);

void initESPNow() {

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  addPeer();

  // Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataReceive);
}


void addPeer() {

  for (int i = 0; i < MAX_PEERS; i++) {
    esp_now_del_peer(slaveAddress[i]);
    memcpy(peerInfo[i].peer_addr, slaveAddress[i], 6);
    peerInfo[i].channel = 0;
    peerInfo[i].encrypt = false;
    if (esp_now_add_peer(&peerInfo[i]) != ESP_OK) {
      Serial.println("Failed to add peer - will retry in 5 seconds");
      continue;
    }
  }
  Serial.println("ESP-NOW initialized successfully");
}

// Callback function when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback for receiving data from the slave
void OnDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  uint8_t id;
  memcpy(&id, data, 8);

  if (id == SENSOR_DATA_UPDATE) {
    SensorData sensorData;
    memcpy(&sensorData, data, sizeof(sensorData));
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["id"] = SENSOR_DATA_UPDATE;
    jsonDoc["temperature"] = sensorData.temperature;
    jsonDoc["humidity"] = sensorData.humidity;
    jsonDoc["sensor_name"] = sensorData.sensorID;
    jsonDoc["min_threshold_exceeded"] = sensorData.lowerThresholdExceeded;
    jsonDoc["max_threshold_exceeded"] = sensorData.upperThresholdExceeded;

    String msg;
    serializeJson(jsonDoc, msg);
    Serial.println(msg);

    esp_now_send(info->src_addr, (const uint8_t *)&currentData, sizeof(currentData));

  }
}

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("\n\nMaster Device Starting...");

  // Set device as a Wi-Fi Station and disable power save mode
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  // Print MAC addresses
  Serial.print("Master MAC Address: ");
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  initESPNow();

  Serial.println("Setup completed");
}

void loop() {


  readSerial();
}


void readSerial() {
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    Serial.print("Received from Serial: ");
    Serial.println(message);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
      uint8_t id = doc["id"];

      if (id == SAMPLING_INTERVAL_UPDATE_ID) {''
        unsigned long interval = doc["interval"];
        currentData.samplingInterval = interval*1000;
      } else if (id == COMMUNICATION_INTERVAL_UPDATE_ID) {
        unsigned long interval = doc["interval"];
        currentData.communicationInterval = interval*1000;
      } else if (id == MINIMUM_THRESHOLD_UPDATE_ID) {
        float threshold = doc["threshold"];
        currentData.minThreshold = threshold;
      } else if (id == MAXIMUM_THRESHOLD_UPDATE_ID) {
        float threshold = doc["threshold"];
        currentData.maxThreshold = threshold;
      }

    } else {
      Serial.println("Failed to parse JSON");
    }
  }
}