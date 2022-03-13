// MQTT example from https://github.com/garyexplains/examples/blob/master/MKR1000/mqtt_button_and_led.ino
// ESP32 dual core example from https://www.youtube.com/watch?v=k_D_Qu0cgu8
#include <Arduino.h>
#include <AccelStepper.h>
#include <TMCStepper.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include "variables.h"
#include "my_preferences.h"


TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
TaskHandle_t C0;  // Dual core setup
void core0Task(void * parameter);
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);


void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);


  Serial.begin(9600);         // Initialize hardware serial for debugging
  preferences_local.begin("local", false);
  load_preference();


  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);


  // Driver setup
  SERIAL_PORT.begin(115200);  // Initialize hardware serial for hardware UART driver
  driver.pdn_disable(true);   // Enable UART on TMC2209
  driver.begin();             // Begin sending data
  driver.toff(4);             // Enables driver in software
  driver.rms_current(600);    // Set motor current to 600mA
  driver.pwm_autoscale(true);    // Needed for stealthChop
  driver.en_spreadCycle(false);  // Toggle spreadCycle on TMC2208/2209/2224
  // driver.TPWMTHRS(0);
  driver.semin(0);
  driver.semax(2);
  driver.sedn(0b00);
  driver.blank_time(24);
  driver.microsteps(microsteps);


  // Motor setup
  stepper.setEnablePin(EN_PIN);
  stepper.setMaxSpeed(8000);
  stepper.setAcceleration(50000);
  stepper.setPinsInverted(false, false, true);
  stepper.setCurrentPosition(current_position);
  stepper.disableOutputs();


  // Attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(5000);
  Serial.println("You're connected to the network");
  Serial.println();


  // Attempt to connect to MQTT broker
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  while (!mqttClient.connect(broker, port));
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  mqttClient.subscribe(topic);  // Subscribe to a topic


  // Indicate finished setup
  digitalWrite(LED_PIN, LOW);
}


void loop() {
  if (command == STOP) {
    stepper.moveTo(stepper.currentPosition());
    stepper.disableOutputs();
  } else if (command == MOVE) {
    stepper.run();
  }
}


void core0Task(void * parameter) {
  for (;;) {
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
        command = MOVE;
        stepper.moveTo(steps_per_rev * 5);
        stepper.enableOutputs();
        mqttClient.beginMessage(subtopic);
        mqttClient.print("Rotate cw");
        mqttClient.endMessage();
        Serial.println("Sent MQTT message.");
        subMessage = "";
      } else if(subMessage == "2") {
        command = STOP;
        mqttClient.beginMessage(subtopic);
        mqttClient.print("Stop");
        mqttClient.endMessage();
        Serial.println("Sent MQTT message.");
        subMessage = "";
      } else if(subMessage == "3") {
        command = MOVE;
        stepper.moveTo(0);
        stepper.enableOutputs();
        mqttClient.beginMessage(subtopic);
        mqttClient.print("Rotate ccw");
        mqttClient.endMessage();
        Serial.println("Sent MQTT message.");
        subMessage = "";
      }
    }
    delay(10);
  }
}
