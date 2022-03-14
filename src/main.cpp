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
void moveToPosition(int position);
void stopMotor();
int percentToSteps(int percent);
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);


void setup() {
  // Initialize hardware serial for debugging
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);


  // Load preferences from memory
  preferences_local.begin("local", false);
  load_preference();
  // Send current open_precent


  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);


  // Driver setup
  SERIAL_PORT.begin(115200);  // Initialize hardware serial for hardware UART driver
  driver.pdn_disable(true);   // Enable UART on TMC2209
  driver.begin();             // Begin sending data
  driver.toff(4);             // Enables driver in software
  driver.rms_current(600);    // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
  driver.pwm_autoscale(true);    // Needed for stealthChop
  driver.en_spreadCycle(false);  // Toggle spreadCycle on TMC2208/2209/2224
  driver.blank_time(24);
  driver.microsteps(microsteps);


  // Motor setup
  stepper.setEnablePin(EN_PIN);
  stepper.setMaxSpeed(velocity);
  stepper.setAcceleration(acceleration);
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


  // Indicate setup is complete
  digitalWrite(LED_PIN, LOW);
}


void loop() {
  if (motor_flag == MOTOR_RUNNING && stepper.distanceToGo() != 0) {
    stepper.run();
  } else if (motor_flag == MOTOR_RUNNING && stepper.distanceToGo() == 0) {
    stopMotor();
  }
}


void core0Task(void * parameter) {
  String subMessage = "";

  for (;;) {
    int messageSize = mqttClient.parseMessage();
    if (messageSize) {
      // we received a message, print out the topic and contents
      Serial.print("Received a message with topic '");
      Serial.print(mqttClient.messageTopic());
      Serial.print("', length ");
      Serial.print(messageSize);
      Serial.println(" bytes:");

      // Use the Stream interface to print the contents
      while (mqttClient.available()) subMessage = subMessage + (char) mqttClient.read();
      int command = subMessage.toInt();
      subMessage = "";
      Serial.println(command);

      // Interpret command
      if (command >= 0) {
        moveToPosition(percentToSteps(command));
      } else if (command == CLOSE) {
        moveToPosition(max_steps);
      } else if (command == STOP) {
        stopMotor();
      } else if (command == OPEN) {
        moveToPosition(0);
      } else if (command == DOWN) {
        moveToPosition(100000000);
      } else if (command == UP) {
        moveToPosition(-100000000);
      }
    }
    delay(100);
  }
}


void sendMqttMessage() {
  mqttClient.beginMessage(subtopic);
  mqttClient.print(open_percent);
  mqttClient.endMessage();
  Serial.println("Sent MQTT message");
}


void moveToPosition(int position) {
  stepper.moveTo(position);
  motor_flag = MOTOR_RUNNING;
  stepper.enableOutputs();
}


void stopMotor() {
  motor_flag = MOTOR_STOPPED;
  stepper.moveTo(stepper.currentPosition());
  stepper.disableOutputs();
  current_position = stepper.currentPosition();
  open_percent = (int) ((float) current_position / (float) max_steps * 100);
  // preferences_local.putInt("current_position", current_position);
  sendMqttMessage();
}

int percentToSteps(int percent) {
  return percent * max_steps / 100;
}