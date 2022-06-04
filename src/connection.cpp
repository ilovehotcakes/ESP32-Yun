#include "connection.h"

Connection::Connection() {
  // For Blue LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // flag = -6;
}


// Todo: add timeout and restart
void Connection::startWifi() {
  Serial.println((String) "[E] Attempting to connect to WPA SSID: " + ssid);

  while (WiFi.begin(ssid.c_str(), pass.c_str()) != WL_CONNECTED)
    delay(5000);
  
  Serial.print("[E] You're connected to the WiFi! IP: ");
  Serial.println(WiFi.localIP());

  connection = connectingMqtt;
}


void Connection::callback(char* topic, uint8_t* buf, unsigned int len) {
  String message = "";
  for (int i = 0; i < len; i++) message += (char) buf[i];
  int command = message.toInt();

  Serial.println((String) "Received a command: " + command);

  // if (command >= 0) {
  //   moveToPosition(percentToSteps(command));
  // } else if (command == COVER_STOP) {
  //   cover = COVER_STOP;
  // } else if (command == COVER_CLOSE) {
  //   moveToPosition(0);
  //   cover = COVER_CLOSE;
  // } else if (command == COVER_OPEN) {
  //   moveToPosition(max_steps);
  //   cover = COVER_OPEN;
  // } else if (command == COVER_SET_MAX) {
  //   setMax();
  // } else if (command == COVER_SET_MIN) {
  //   setMin();
  // }
}


// Todo: add timeout and restart
// Todo: add will message to send percentage
void Connection::connectMqtt() {
  mqttClient.setServer(brokerIP.c_str(), brokerPort);

  Serial.println((String) "[E] Attempting to connect to MQTT broker: " + brokerIP);
  
  while (!mqttClient.connect(mqttID.c_str(), mqttUser.c_str(), mqttPass.c_str()));

  mqttClient.subscribe(inTopic.c_str());

  mqttClient.setCallback(callback);

  Serial.println((String) "[E] You're connected to the MQTT broker! Topic: " + inTopic);
}


int Connection::run() {
  switch (connection) {
    case reconnectingWifi:
      digitalWrite(LED_PIN, HIGH);
      // Wait for reconnection
      if (WiFi.status() == WL_CONNECTED)
        connection = connectingMqtt;
    break;

    case connectingMqtt:
      digitalWrite(LED_PIN, HIGH);
      connectMqtt();
      connection = readingMqttMessage;
      digitalWrite(LED_PIN, LOW);
    break;

    case readingMqttMessage:
      // Check wifi connection
      if (WiFi.status() == WL_NO_SSID_AVAIL)
        connection = reconnectingWifi;

      // Check mqtt connection
      if (!mqttClient.connected())
        connection = connectingMqtt;
      
      // Use non blocking pubsub to read messages
      mqttClient.loop();
    break;
  }
  return 0;
}