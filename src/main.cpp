/**
    ESP32 Motorcover
    Aurthor: Jason Chen, 2022

    A simple ESP32 program that let's user control powerful and quiet stepper motors via MQTT. You
    use it to open/close blinds, shades, etc. Can be used with Alexa or Home Assistant.

    This program utilizes TMC2209 drivers for the stepper motor and it drives NEMA stepper motors.

    You can copy the BasicOTA example from the Arduino example library and paste it into a file
    named "ota.h". Exlude the wifi setup and "loop()"; rename "setup()"" to "otaSetup()" and
    include it in the src folder to enable OTA updates. Useful for wireless updates.
**/
#include <Arduino.h>
#include "logger.h"
#include "motor_task.h"
#include "wireless_task.h"


static MotorTask motor_task(1);
static WirelessTask wireless_task(0);


void setup() {
    // Start logger
    LOG_INIT(9600, LogLevel::INFO);

    // Initialize LED & BUTTON
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    delay(2000);
    // Start the WiFi connection
    wireless_task.init();
    wireless_task.addListener(motor_task.getMotorCommandQueue());

    delay(2000);
    motor_task.init();
    motor_task.addListener(wireless_task.getWirelessMessageQueue());

    // Delete setup/loop task
    vTaskDelete(NULL);
}


void loop() {}