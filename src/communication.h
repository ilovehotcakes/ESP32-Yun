#include <WiFi.h>
#include <PubSubClient.h>

class Connection {
private:
  enum ConnectionState {
    initializing,
    reconnectingWifi,
    connectingMqtt,
    readingMqttMessage
  };

  WiFiClient wifiClient;
  PubSubClient mqttClient;
  ConnectionState state;

  // For WiFi
  char ssid[];  // Network SSID (name)
  char pass[];  // Network password

  // For MQTT
  char mqttID[];
  char mqttUser[];
  char mqttPass[];
  char brokerIP[];  // Address of the MQTT server
  int  brokerPort;
  char inTopic[];
  char outTopic[];


public:
  Connection();
  void startWifi();
  void connectMqtt();
  int callback(char* topic, byte* buf, unsigned int len);
  int run();
};