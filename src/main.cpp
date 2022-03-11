// MQTT example from https://github.com/garyexplains/examples/blob/master/MKR1000/mqtt_button_and_led.ino
// ESP32 dual core example from https://www.youtube.com/watch?v=k_D_Qu0cgu8
#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>

TaskHandle_t C0;

// Define connections to TMC2208
const int dirPin = 18;  // Direction
const int stepPin = 19; // Step
const int ms2Pin = 21;  // MS2 pin
const int ms1Pin = 22;  // MS1 pin
const int enPin = 23;   // Enable
const int ledPin = 2;   // LED tied to GPIO2

const int STEPS_PER_REV = 200*5.18*4;  // Motor steps per rotation, microsteps = 4
int currPos = 10000;
int moveOrNot = 2;

char ssid[] = "NalaSecretBase_2.4";  // your network SSID (name)
char pass[] = "2063832037s";         // your network password (use for WPA, or use as key for WEP)

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.1.26"; // Address of the MQTT server
int        port     = 1883;
const char topic[]  = "/arduino/simple";
const char subtopic[]  = "/arduino/receive";
String subMessage = "";


void codeForTask1(void * parameter){
  for (;;) {
    if (moveOrNot == 1 || moveOrNot == 3) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(300);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(300);
    } else {
      delay(100);
    }
  }
}


void setup() {
  //Initialize serial
  Serial.begin(9600);
  
  // Setup the pins as Outputs
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH);   // Disable driver
  pinMode(ms1Pin, OUTPUT);
  digitalWrite(ms1Pin, LOW);   // Microstep 4
  pinMode(ms2Pin, OUTPUT);
  digitalWrite(ms2Pin, HIGH);  // Microstep 4
  digitalWrite(2, HIGH);

  
  // Attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(5000);  // failed, retry
  }
  Serial.println("You're connected to the network");
  Serial.println();

  
  // You can provide a username and password for authentication
   mqttClient.setUsernamePassword("mqtt-user", "jnkjnk37");
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  while (!mqttClient.connect(broker, port)) {
    // Serial.println(mqttClient.connectError());
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  mqttClient.subscribe(topic);  // subscribe to a topic
  

  // Dual core
  disableCore0WDT();
  xTaskCreatePinnedToCore(codeForTask1, "Core_0", 8192, NULL, 1, &C0, 0);
  digitalWrite(2, LOW);
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  // mqttClient.poll();

  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    subMessage = "";
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient.available()) {
      subMessage = subMessage + (char)mqttClient.read();
    }
    Serial.println(subMessage);

    if(subMessage == "1") {
      moveOrNot = 1;
      mqttClient.beginMessage(subtopic);
      mqttClient.print("Rotate cw");
      digitalWrite(dirPin, LOW);  // Set motor direction clockwise
      digitalWrite(enPin, LOW);   // Enable driver
      mqttClient.endMessage();
      Serial.println("Sent MQTT message.");
      subMessage = "";
    } else if(subMessage == "2") {
      moveOrNot = 2;
      mqttClient.beginMessage(subtopic);
      mqttClient.print("Stop");
      digitalWrite(enPin, HIGH);  // Disable driver
      mqttClient.endMessage();
      Serial.println("Sent MQTT message.");
      subMessage = "";
    } else if(subMessage == "3") {
      moveOrNot = 3;
      mqttClient.beginMessage(subtopic);
      mqttClient.print("Rotate ccw");
      digitalWrite(enPin, LOW);    // Enable driver
      digitalWrite(dirPin, HIGH);  // Set motor direction
      mqttClient.endMessage();
      Serial.println("Sent MQTT message.");
      subMessage = "";
    }
  }
  delay(10);
}