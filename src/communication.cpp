#include "communication.h"

Connection::Connection() {
  state = initializing;
  strcpy(ssid, String("NalaSecretBase_2.4").c_str());  // WiFi SSID (name)
  strcpy(pass, String("2063832037s").c_str());  // WiFi password
  // pass = wifiPassSecret.c_str();  // Network password
  // mqttID     = mqttIDSecret;
  // mqttUser   = mqttUserSecret;
  // mqttPass   = mqttPassSecret;
  // brokerIP   = brokerIPSecret;
  // brokerPort = 1883;
  // inTopic    = "/server/shades/1";
  // outTopic   = "/client/blinds/1";
  // Serial.print("hi");
  // startWifi();
}


// Todo: add timeout and restart
void Connection::startWifi() {
  Serial.println((String) "[E] Attempting to connect to WPA SSID: " + ssid);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED)
    delay(5000);
  
  Serial.print("[E] You're connected to the WiFi! IP: ");
  Serial.println(WiFi.localIP());

  state = connectingMqtt;
}


int Connection::callback(char* topic, byte* buf, unsigned int len) {
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
  return 0;
}


// Todo: add timeout and restart
// Todo: add will message to send percentage
void Connection::connectMqtt() {
  // mqttClient.setClient(wifiClient);
  // mqttClient.setServer(brokerIP, brokerPort);

  Serial.println((String) "[E] Attempting to connect to MQTT broker: " + brokerIP);
  
  // while (!mqttClient.connect(mqttID, mqttUser, mqttPass));

  // mqttClient.subscribe(inTopic);

  // mqttClient.setCallback(callback);

  // Serial.println((String) "[E] You're connected to the MQTT broker! Topic: " + inTopic);
}


// int Connection::run() {
//   switch (state) {
//     case connectingWifi:
//       // Wait for reconnection
//       if (WiFi.status() == WL_CONNECTED)
//         state = connectingMqtt;
//     break;

//     case connectingMqtt:
//       connectMqtt();

//       state = readingMqttMessage;
//     break;

//     case readingMqttMessage:
//       // Check if connected to wifi
//       if (WiFi.status() == WL_NO_SSID_AVAIL)
//         state = connectingWifi;

//       // Check if connected to mqtt
//       if (!mqttClient.connected())
//         state = connectingMqtt;
      
//       // Use non blocking pubsub to read messages
//       mqttClient.loop();
//     break;
//   }

//   return 0;
// }