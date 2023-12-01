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
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <Preferences.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include "task.h"
#include "motor_connections.h"


enum MotorState {
    MOTOR_IDLE,
    MOTOR_MIN,
    MOTOR_MAX,
    MOTOR_SET_MIN,
    MOTOR_SET_MAX
};


class MotorTask : public Task<MotorTask> {
    friend class Task<MotorTask>;

public:
    MotorTask(const uint8_t task_core);
    void move(int percent);
    void stop();
    void min();
    void max();
    void setMin();
    void setMax();
    void resetSettings();

protected:
    void run();

private:
    #include "motor_settings.h"

    // TMCStepper library used for interfacing between MCU and stepper driver hardware
    TMC2209Stepper driver = TMC2209Stepper(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);

    // FastAccelStepper library used for sending commands to the stepper driver to
    // move/accelerate and stop/deccelerate the stepper motor
    FastAccelStepperEngine engine = FastAccelStepperEngine();
    FastAccelStepper *stepper = NULL;
    Preferences motor_setting_;

    bool is_motor_running_;
    uint32_t max_position_;
    uint32_t current_position_;
    uint32_t previous_position_;
    MotorState current_state_;
    MotorState previous_state_;

    int percentToSteps(int percent) const; // Helper function to calculate position percentage to steps
    void loadSettings(); // Load motor settings from flash
    void setMotorState(MotorState newState);
    void moveTo(int newPos);
    void stallguardInterrupt();
    int  currentPercentage();
    void updatePosition();

    // TODO
    // void motorSetSpeed() {}
    // void motorSetQuietModeSpeed() {}
    // void motorSetOpeningRMS() {}
    // void motorSetclosingRMS() {}
    // void motorSetDirection() {}
    // void motorEnableQuietmode() {}
    // void motorDisableQuietmode() {}
    // void motorEnableSG() {}
    // void motorDisableSG() {}
};