/**
    Aurthor: Jason Chen, 2022
    Licence: TODO

    An ESP32-based low-powered wireless motor controller that works with two-phase bipolar stepper
    motors. It is a closed-loop system so it is capable of keeping track of its position even when
    the motor is off.

    You can use it to motorize and automate the opening & closing of blinds/shades/windows etc.
**/
#include "system_task.h"
#include "motor_task.h"
#include "wireless_task.h"


static WirelessTask wireless_task(0);  // Running on core0
static SystemTask system_task(0);      // Running on core0
static MotorTask motor_task(1);        // Running on core1


void setup() {
    // Initializing serial output if compiled
    LOG_INIT(115200, LogLevel::INFO);

    if (!LITTLEFS.begin(true)) {
        LOGI("Failed to mount filesystem");
    }

    // setCpuFrequencyMhz(80);

    system_task.init();
    system_task.addMotorTask(&motor_task);
    system_task.addWirelessTask(&wireless_task);

    wireless_task.init();
    wireless_task.addMotorTask(&motor_task);
    wireless_task.addSystemTask(&system_task);
    wireless_task.addSystemSleepTimer(system_task.getSystemSleepTimer());

    motor_task.init();
    motor_task.addWirelessTask(&wireless_task);
    motor_task.addSystemSleepTimer(system_task.getSystemSleepTimer());

    // Delete setup/loop task
    vTaskDelete(NULL);
}


void loop() {}