#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include "connection.h"
#include "motor.h"

bool VERBOSE = true;
TaskHandle_t C0;  // Dual core setup
Connection hermes;
Motor      zeus;


void core0Task(void * parameter) {
  for (;;) hermes.run();
}


void setup() {
  // Initialize hardware serial for debugging
  if (VERBOSE) Serial.begin(9600);

  // Core 0 setup for dual core operation
  disableCore0WDT();  // Disable watchdog timer
  xTaskCreatePinnedToCore(core0Task, "Core_0", 8192, NULL, 1, &C0, 0);

  hermes = Connection();
  hermes.startWifi();

  zeus = Motor();
}


void loop() {
  zeus.run();
}
