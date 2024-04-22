#pragma once
/**
    motor_task.h - A class that contains all functions to control a stepper motor.
    Author: Jason Chen, 2022

    The way to control the stepper motor is sending PWM to the stepper motor driver, TMC2209. To
    operate the TMC2209: (1) need to start the driver (take it out of STANDBY), and (2) ENABLE the
    motor by energizing the motor coils. Both are done automatically: (1) is by the wireless task
    and (2) is by FastAccelStepper.
**/
#include <Arduino.h>
#include <HardwareSerial.h>       // Hardwareserial for uart
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <AS5600.h>
#include <Preferences.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include "task.h"
#include "logger.h"
#include "commands.h"


class MotorTask : public Task<MotorTask> {
    friend class Task<MotorTask>;

public:
    MotorTask(const uint8_t task_core);
    ~MotorTask();
    void addWirelessTaskQueue(QueueHandle_t queue);
    void addSystemSleepTimer(xTimerHandle timer);
    SemaphoreHandle_t getMotorStandbySemaphore();

protected:
    void run();

private:
    // TMCStepper library for interfacing with stepper motor driver hardware, mainly reading and
    // writing registers for setting speed, acceleration, current, etc.
    TMC2209Stepper driver_ = TMC2209Stepper(&Serial1, R_SENSE, DRIVER_ADDR);

    // FastAccelStepper library for generating PWM signal to the stepper driver to move/accelerate
    // and stop/deccelerate the stepper motor.
    FastAccelStepperEngine engine_ = FastAccelStepperEngine();
    FastAccelStepper *motor_ = NULL;

    // Rotary encoder for keeping track of the actual motor position because motor could slip and
    // cause the position to be incorrect, i.e. closed-loop system.
    AS5600 encoder_;

    // Saving motor settings, such as motor's max position and other attributes
    Preferences motor_settings_;

    QueueHandle_t wireless_task_queue_;   // To receive messages from wireless task
    xSemaphoreHandle motor_standby_sem_;  // To signal to wireless task that motor is in standby
    xTimerHandle system_sleep_timer_;     // To prevent system from sleeping before motor stops

    // User adjustable TMC2209 motor driver settings
    int microsteps_           = 16;
    int steps_per_revolution_ = 200 * microsteps_;  // NEMA motors have 200 full steps/rev
    int velocity_             = static_cast<int>(steps_per_revolution_ * 3);
    int acceleration_         = static_cast<int>(velocity_ * 0.5);
    bool direction_           = false;
    int opening_current_      = 200;
    int closing_current_      = 75;  // 1, 3: 200; 2: 400; 4: 300
    int stallguard_threshold_ = 10;

    volatile bool stalled_    = false;
    portMUX_TYPE stalled_mux_ = portMUX_INITIALIZER_UNLOCKED;
    bool stallguard_enabled_  = true;

    int32_t encod_pos_            = 0;
    int32_t encod_max_pos_        = 0;
    int8_t  last_updated_percent_ = -100;
    float motor_encoder_ratio_    = steps_per_revolution_ / 4096.0;
    float encoder_motor_ratio_    = 4096.0 / steps_per_revolution_;

    void stallguardInterrupt();
    void loadSettings(); // Load motor settings from flash
    void moveToPercent(int percent);
    void stop();
    bool setMin();
    bool setMax();
    bool driverEnable(uint8_t enable_pin, uint8_t value);
    // For quick configuration guide, please refer to p70-72 of TMC2209's datasheet rev1.09
    // TMC2209's UART interface automatically becomes enabled when correct UART data is sent. It
    // automatically adapts to uC's baud rate. Block until UART is finished initializing so ESP32
    // can send settings to the driver via UART.
    void driverStartup();
    void driverStandby();
    inline int getPercent();
    inline int positionToSteps(int encoder_position);

    // TODO set/get
    // void setMicrosteps()
    // void setVelocity() {}
    // void setAcceleration() {}
    // void setOpeningCurrent() {}
    // void setClosingCurrent() {}
    // void setDirection() {}
    // void disableStallguard() {}
    // void enableStallguard() {}
};