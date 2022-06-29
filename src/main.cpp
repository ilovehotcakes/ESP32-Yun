/**
  Motorized Cover - ESP32
  Aurthor: Jason Chen

  A simple ESP32 program that let's user control powerful and quiet stepper
  motors via MQTT. You use it to open and close blinds/shades, etc. Can be
  used with Alexa or Home Assistant.

  This program utilizes TMC2209 drivers for the stepper motor and it drives
  NEMA stepper motors.
**/
#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <PubSubClient.h>
#define DONTCOMPILELOGS
#include "logger.h"
#include "motor.h"
#include "ota.h"
#include "secrets.h"


// States for the state machine
enum CoverState { INITIALIZING, RECONNECTING_WIFI, CONNECTING_MQTT, READ_MQTT_MSG };
// Commands recieved from MQTT
enum Command { COVER_STOP=-1, COVER_OPEN=-2, COVER_CLOSE=-3, COVER_SET_MIN=-4, COVER_SET_MAX=-5, SYS_REBOOT=-99 };


void core0Task(void * parameter);
void startWifi();
void connectMqtt();
void sendMqtt(String);


TaskHandle_t C0;  // For dual core setup
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
CoverState state = INITIALIZING;       // State machine to manage WiFi/MQTT
String   ssid       = secretSSID;      // SSID (name) for WiFi
String   pass       = secretPass;      // Network password for WiFi
String   mqttID     = secretMqttID;    // MQTT ID for PubSubClient
String   mqttUser   = secretMqttUser;  // MQTT server username (optional)
String   mqttPass   = secretMqttPass;  // MQTT server password (optional)
String   brokerIP   = secretBrokerIP;  // IP of MQTT server
uint16_t brokerPort = secretBrokerPort;
String   inTopic    = secretInTopic;   // MQTT inbound topic
String   outTopic   = secretOutTopic;  // MQTT outbound topic


void setup() {
  LOG_INIT(9600, LogLevel::INFO);

  // Initialized and turn on LED to indicate boot up
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  motorSetup();

  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);

  // Start the WiFi connection
  startWifi();
  state = CONNECTING_MQTT;

  #if __has_include("ota.h")
    otaSetup();
  #endif
}


/**
  Keeps running the motor and checks if there is any messages avaiable to
  publish to MQTT server. The motor must run in the loop. For some unknown
  reasons, the stepper doesn't run smoothly on core0.
**/
void loop() {
  motorRun();
}


/**
  The state machine that tracks communication to MQTT/WiFi. I have tried to
  start the WiFi in the state machine but it doesn't work. Also, the delay at
  the of the function is required, else WiFi can't connect for unknown reason.
**/
void core0Task(void * parameter) {
  for (;;) {
    switch (state) {
      case RECONNECTING_WIFI:
        // Turn on LED to indicate disconnected
        digitalWrite(LED_PIN, HIGH);
        
        // Wait for reconnection
        if (WiFi.status() == WL_CONNECTED)
          state = CONNECTING_MQTT;
      break;

      case CONNECTING_MQTT:
        // Turn on LED to indicate disconnected
        digitalWrite(LED_PIN, HIGH);

        connectMqtt();

        // Update MQTT server of current shade opening position
        sendMqtt((String) motorCurrentPercentage());

        // Turn off LED to indicate fully connected
        digitalWrite(LED_PIN, LOW);
        state = READ_MQTT_MSG;
      break;

      case READ_MQTT_MSG:
        // Check wifi connection
        if (WiFi.status() == WL_NO_SSID_AVAIL)
          state = RECONNECTING_WIFI;

        // Check mqtt connection
        if (!mqttClient.connected())
          state = CONNECTING_MQTT;
        
        // Use non blocking method to check for messages
        mqttClient.loop();
      break;
    }
    delay(100);

    #if __has_include("ota.h")
      ArduinoOTA.handle();
    #endif
  }
}


// TODO: Add timeout and restart
void startWifi() {
  LOGI("Attempting to connect to WPA SSID: %s", ssid.c_str());

  while (WiFi.begin(ssid.c_str(), pass.c_str()) != WL_CONNECTED)
    delay(5000);
  
  LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


void readMqtt(char* topic, byte* buf, unsigned int len) {
  String message = "";
  for (int i = 0; i < len; i++) message += (char) buf[i];
  int command = message.toInt();

  LOGI("Received message: %s", message);

  if (command >= 0) motorMove(command);
  else if (command == COVER_STOP) motorStop();
  else if (command == COVER_OPEN) motorMin();
  else if (command == COVER_CLOSE) motorMax();
  else if (command == COVER_SET_MAX) motorSetMax();
  else if (command == COVER_SET_MIN) motorSetMin();
  else if (command == SYS_REBOOT) ESP.restart();
}


// TODO: add timeout and restart
void connectMqtt() {
  LOGI("Attempting to connect to MQTT broker: %s", brokerIP.c_str());

  mqttClient.setServer(brokerIP.c_str(), brokerPort);
  
  while (!mqttClient.connect(mqttID.c_str(), mqttUser.c_str(), mqttPass.c_str()));

  mqttClient.subscribe(inTopic.c_str());
  mqttClient.setCallback(readMqtt);

  LOGI("Connected to the MQTT broker, topic: %s", inTopic.c_str());
}


void sendMqtt(String message) {
  mqttClient.publish(secretOutTopic.c_str(), message.c_str());
  LOGI("Sent message: %s", message);
}