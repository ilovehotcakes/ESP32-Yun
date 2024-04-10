#pragma once
/**
    motor_task.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2022

    This class contains all stepper motor controls, which includes, initializing the stepper driver
    (TMCStepper), stepper motor control (FastAccelStepper), as well as recalling its previous
    position and maximum position on reboot. It also sends current position via MQTT after it
    stops.

    It also gives the user the option to set the maximum and minimum stepper motor positions via
    MQTT. (1) User doesn't have to pre-calculate the max/min travel distance (2) User can re-adjust
    max/min positions without reflashing firmware.
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


// Commands recieved from MQTT
enum MotorCommand {
    COVER_STOP    = -1,
    COVER_OPEN    = -2,
    COVER_CLOSE   = -3,
    COVER_SET_MIN = -4,
    COVER_SET_MAX = -5,
    STBY_ON       = -6,
    STBY_OFF      = -7
};


class MotorTask : public Task<MotorTask> {
    friend class Task<MotorTask>;

public:
    MotorTask(const uint8_t task_core);
    ~MotorTask();
    void addWirelessQueue(QueueHandle_t queue);
    QueueHandle_t getMotorCommandQueue();

protected:
    void run();

private:
    // TMCStepper library for interfacing MCU with stepper driver hardware
    TMC2209Stepper driver_ = TMC2209Stepper(&Serial1, R_SENSE, DRIVER_ADDR);

    // FastAccelStepper library for sending commands to the stepper driver to
    // move/accelerate and stop/deccelerate the stepper motor
    FastAccelStepperEngine engine_ = FastAccelStepperEngine();
    FastAccelStepper *motor_ = NULL;

    // Rotary encoder for keeping track of actual motor positions because motor could
    // slip and cause the position to be incorrect
    AS5600 encoder_;

    // Saving positions and other attributes
    Preferences motor_settings_;

    QueueHandle_t wireless_message_queue_;  // Used to receive message from wireless task
    QueueHandle_t motor_message_queue_;     // Used to send messages to wireless task
    int motor_command_ = -50;

    // TMC2209 settings
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

    int32_t encod_max_pos_        = 0;
    int8_t  last_updated_percent_ = -100;
    float motor_encoder_ratio_    = steps_per_revolution_ / 4096.0;
    float encoder_motor_ratio_    = 4096.0 / steps_per_revolution_;
    bool stallguard_enable_       = true;

    void stallguardInterrupt();
    void loadSettings(); // Load motor settings from flash
    void moveToPercent(int percent);
    void stop();
    bool setMin();
    bool setMax();
    inline int getPercent();
    inline int positionToSteps(int encoder_position);
    bool driverEnable(uint8_t enable_pin, uint8_t value);
    void driverStartup();
    void driverStandby();

    // TODO
    // void setMicrosteps()
    // void setVelocity() {}
    // void setAcceleration() {}
    // void setOpeningCurrent() {}
    // void setClosingCurrent() {}
    // void setDirection() {}
    // void disableStallguard() {}
    // void enableStallguard() {}
};