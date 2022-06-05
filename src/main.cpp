#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "motor.h"


#define INITIALIZING      0
#define RECONNECTING_WIFI 1
#define CONNECTING_MQTT   2
#define READ_MQTT_MSG     3
#define COVER_STOP       -1
#define COVER_OPEN       -2
#define COVER_CLOSE      -3
#define COVER_SET_MAX    -4
#define COVER_SET_MIN    -5
#define LED_PIN           2 // LED tied to GPIO2 on HiLetGo board


void core0Task(void * parameter);
void startWifi();
void connectMqtt();
void sendMessage();



bool VERBOSE = true;
TaskHandle_t C0;  // Dual core setup
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Motor    motor;
int connection = INITIALIZING;  // For WiFi/MQTT state machine
String   ssid = secretSSID;     // SSID (name) for WiFi
String   pass = secretPass;     // Network password for WiFi
String   mqttID     = secretMqttID;    // For MQTT
String   mqttUser   = secretMqttUser;
String   mqttPass   = secretMqttPass;
String   brokerIP   = secretBrokerIP;  // Address of the MQTT server
uint16_t brokerPort = secretBrokerPort;
String   inTopic    = secretInTopic;
String   outTopic   = secretOutTopic;




void setup() {
  // Initialize hardware serial for debugging
  if (VERBOSE) Serial.begin(9600);
  
  // Turn on 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);

  // Connect to the WiFi
  startWifi();
  connection = CONNECTING_MQTT;

  motor = Motor();
}


void loop() {
  motor.run();
}


// Motor tasks
void core0Task(void * parameter) {
  for (;;) {
    switch (connection) {
      case RECONNECTING_WIFI:
        // Turn on LED to indicate disconnected
        digitalWrite(LED_PIN, HIGH);
        
        // Wait for reconnection
        if (WiFi.status() == WL_CONNECTED)
          connection = CONNECTING_MQTT;
      break;

      case CONNECTING_MQTT:
        // Turn on LED to indicate disconnected
        digitalWrite(LED_PIN, HIGH);
        connectMqtt();
        digitalWrite(LED_PIN, LOW);
        connection = READ_MQTT_MSG;
      break;

      case READ_MQTT_MSG:
        // Check wifi connection
        if (WiFi.status() == WL_NO_SSID_AVAIL)
          connection = RECONNECTING_WIFI;

        // Check mqtt connection
        if (!mqttClient.connected())
          connection = CONNECTING_MQTT;
        
        // Use non blocking pubsub to read messages
        mqttClient.loop();
      break;
    }
    delay(100);  // Must have or else state won't change when initializing
  }
}



// Todo: add timeout and restart
void startWifi() {
  Serial.println("[E] Attempting to connect to WPA SSID: " + ssid);

  while (WiFi.begin(ssid.c_str(), pass.c_str()) != WL_CONNECTED)
    delay(5000);
  
  Serial.print("[E] You're connected to the WiFi! IP: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* buf, unsigned int len) {
  String message = "";
  for (int i = 0; i < len; i++) message += (char) buf[i];
  int command = message.toInt();

  Serial.println("Received message: " + message);

  if (command >= 0 && command <= 100) motor.percent(command);
  else if (command == COVER_STOP) motor.stop();
  else if (command == COVER_OPEN) motor.open();
  else if (command == COVER_CLOSE) motor.close();
  else if (command == COVER_SET_MAX) motor.setMax();
  else if (command == COVER_SET_MIN) motor.setMin();
}


// Todo: add timeout and restart
// Todo: add will message to send percentage
void connectMqtt() {
  Serial.println("[E] Attempting to connect to MQTT broker: " + brokerIP);

  mqttClient.setServer(brokerIP.c_str(), brokerPort);
  
  while (!mqttClient.connect(mqttID.c_str(), mqttUser.c_str(), mqttPass.c_str()));

  mqttClient.subscribe(inTopic.c_str());
  mqttClient.setCallback(callback);

  Serial.println("[E] You're connected to the MQTT broker! Topic: " + inTopic);
}


void sendMessage () {
  // int percent = (int) (currPos + " / " + maxPos);
  // mqttClient.beginPublish("client/shades/1", 1, false);
  // mqttClient.print(percent);
  // mqttClient.endPublish();
  // Serial.println((String) "Sent message: " + percent + "% = " + currPos + " / " + maxPos);
}