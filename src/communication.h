#include <WiFi.h>
#include <PubSubClient.h>

class Commuication {
private:
  enum Control_State {
    initializing,
    connectingWifi,
    connectingMqtt,
    readingMqttMessage
  };

  WiFiClient wifiClient;
  PubSubClient mqttClient();
  Control_State state;

  // For WiFi
  char ssid[];  // Network SSID (name)
  char pass[];  // Network password

  // For MQTT
  char* mqttID;
  char* mqttUser;
  char* mqttPass;
  char* brokerIP;  // Address of the MQTT server
  int   brokerPort;
  char* inTopic;
  char* outTopic;


public:
  Commuication();
  void startWifi();
  void connectMqtt();
  int callback(char* topic, byte* buf, unsigned int len);
  int run();
};