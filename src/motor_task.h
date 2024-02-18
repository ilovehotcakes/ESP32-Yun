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
enum Command {
    COVER_STOP    = -1,
    COVER_OPEN    = -2,
    COVER_CLOSE   = -3,
    COVER_SET_MIN = -4,
    COVER_SET_MAX = -5,
    SYS_RESET     = -98,
    SYS_REBOOT    = -99
};


class MotorTask : public Task<MotorTask> {
    friend class Task<MotorTask>;

public:
    MotorTask(const uint8_t task_core);
    ~MotorTask();
    void addListener(QueueHandle_t queue);
    QueueHandle_t getMotorCommandQueue();

protected:
    void run();

private:
    #include "motor_settings.h"

    // TMCStepper library for interfacing MCU with stepper driver hardware
    TMC2209Stepper tmc2209 = TMC2209Stepper(&Serial1, R_SENSE, DRIVER_ADDR);

    // FastAccelStepper library for sending commands to the stepper driver to
    // move/accelerate and stop/deccelerate the stepper motor
    FastAccelStepperEngine engine = FastAccelStepperEngine();
    FastAccelStepper *motor = NULL;

    // Rotary encoder for keeping track of actual motor positions because motor could
    // slip and cause the position to be incorrect
    AS5600 encoder;

    // Saving positions and other attributes
    Preferences motor_settings_;

    QueueHandle_t wireless_message_queue_;  // Used to receive message from wireless task
    QueueHandle_t motor_command_queue_;     // Used to send messages to wireless task
    int command = -50;

    int32_t encod_max_pos_  = 0;
    int8_t  last_updated_percent_ = -100;
    float motor_encoder_ratio_ = stepsPerRev / 4096.0;
    float encoder_motor_ratio_ = 4096.0 / stepsPerRev;

    void stallguardInterrupt();
    void loadSettings(); // Load motor settings from flash
    void moveToPercent(int percent);
    void stop();
    void setMin();
    void setMax();
    inline int getPercent();
    inline int positionToSteps(int encoder_position);

    // TODO
    // void motorSetSpeed() {}
    // void motorSetQuietModeSpeed() {}
    // void motorSetDirection() {}
    // void motorEnableQuietmode() {}
    // void motorDisableQuietmode() {}
};