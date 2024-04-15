/**
    ESP32 Motorcover
    Aurthor: Jason Chen, 2022
    Licence: TODO

    An ESP32-based low-powered wireless motor controller that works with bipolar stepper motors.
    It is a closed-loop system so it is capable of keeping track of its position while the motor is
    off.

    You can use it to motorize and automate the opening & closing of blinds/shades/windows etc.
**/
#include <Arduino.h>
#include "logger.h"
#include "system_task.h"
#include "motor_task.h"
#include "wireless_task.h"


static WirelessTask wireless_task(0);  // Running on core0
static SystemTask system_task(0);      // Running on core0
static MotorTask motor_task(1);        // Running on core1


void setup() {
    // Initializing serial output if compiled
    LOG_INIT(9600, LogLevel::INFO);

    // Initializing LED
    pinMode(LED_PIN, OUTPUT);

    // The system task performs coordination between all tasks
    system_task.init();

    wireless_task.init();
    wireless_task.addSystemQueue(system_task.getSystemMessageQueue());
    wireless_task.addMotorQueue(motor_task.getMotorMessageQueue());
    wireless_task.addMotorStandbySemaphore(motor_task.getMotorStandbySemaphore());

    // The motor task runs the motor and checks the rotary encoder to keep track of the motor's
    // position.
    motor_task.init();
    motor_task.addWirelessQueue(wireless_task.getWirelessMessageQueue());

    // Delete setup/loop task
    vTaskDelete(NULL);
}


void loop() {}