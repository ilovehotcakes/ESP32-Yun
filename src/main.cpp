#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "variables.h"
#include "my_preferences.h"


bool VERBOSE = true;
TaskHandle_t C0;  // Dual core setup
void core0Task(void * parameter);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Control_State state = initializing;
void startWifi();
void connectMqtt();
int readState();


void setup() {
  // Initialize hardware serial for debugging
  if (VERBOSE) Serial.begin(9600);

  // For the blue LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);

  startWifi();
  state = connectingMqtt;

  // Load preferences from memory
  // preferences_local.begin("local", false);
  // load_preference();
  
  // sendPercentage();
}


void loop() {
  delay(1000);
}


void core0Task(void * parameter) {
  for (;;) readState();
}


// Todo: add timeout and restart
void startWifi() {
  Serial.println((String) "[E] Attempting to connect to WPA SSID: " + ssid);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED)
    delay(5000);
  
  Serial.print("[E] You're connected to the WiFi! IP: ");
  Serial.println(WiFi.localIP());

  state = connectingMqtt;
}


void callback(char* topic, byte* buf, unsigned int len) {
  int command = 0;
  for (int i = 0; i < len; i++) command += (buf[i]-'0') * pow(10, len-1-i);

  Serial.println((String) "Received a command: " + command);

  // if (command >= 0) {
  //   // moveToPosition(percentToSteps(command));
  // } else if (command == STOP) {
  //   // stopMotor();
  // } else if (command == CLOSE) {
  //   // moveToPosition(max_steps);
  // } else if (command == OPEN) {
  //   // moveToPosition(0);
  // } else if (command == SET_MAX) {
  //   // setMax();
  // } else if (command == SET_MIN) {
  //   // setMin();
  // }
}


// Todo: add timeout and restart
// Todo: add will message to send percentage
void connectMqtt() {
  mqttClient.setServer(brokerIP, brokerPort);

  Serial.println((String) "[E] Attempting to connect to MQTT broker: " + brokerIP);
  
  while (!mqttClient.connect(mqttID, mqttUser, mqttPass));

  mqttClient.subscribe(inTopic);

  mqttClient.setCallback(callback);

  Serial.println((String) "[E] You're connected to the MQTT broker! Topic: " + inTopic);
}


int readState() {
  switch (state) {
    case connectingWifi:
      // Wait for reconnection
      if (WiFi.status() == WL_CONNECTED)
        state = connectingMqtt;
    break;

    case connectingMqtt:
      digitalWrite(LED_PIN, HIGH);

      connectMqtt();

      state = readingMqttMessage;
      
      digitalWrite(LED_PIN, LOW);
    break;

    case readingMqttMessage:
      // Check if connected to wifi
      if (WiFi.status() == WL_NO_SSID_AVAIL)
        state = connectingWifi;

      // Check if connected to mqtt
      if (!mqttClient.connected())
        state = connectingMqtt;
      
      // Use non blocking pubsub to read messages
      mqttClient.loop();
    break;
  }

  return 0;
}