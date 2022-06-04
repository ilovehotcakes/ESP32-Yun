#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

class Connection {
  
private:
  enum ConnectionState {
    initializing,
    reconnectingWifi,
    connectingMqtt,
    readingMqttMessage
  };

  // Object state for the state machine
  ConnectionState connection = initializing;

  // For WiFi
  String ssid = secretSSID;  // Network SSID (name)
  String pass = secretPass;  // Network password

  // For MQTT
  String   mqttID     = secretMqttID;
  String   mqttUser   = secretMqttUser;
  String   mqttPass   = secretMqttPass;
  String   brokerIP   = secretBrokerIP;  // Address of the MQTT server
  uint16_t brokerPort = secretBrokerPort;
  String   inTopic    = secretInTopic;
  String   outTopic   = secretOutTopic;
  
  static void callback(char*, uint8_t*, unsigned int);
  void connectMqtt();


public:
  Connection();
  void startWifi();
  int run();
};