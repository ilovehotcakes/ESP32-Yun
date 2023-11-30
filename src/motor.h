#pragma once
/**
    motor.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2022

    This class contains all stepper motor controls, which includes, initializing the stepper driver
    (TMCStepper), stepper motor control (FastAccelStepper), as well as recalling its previous
    position and maximum position on reboot. It also sends current position via MQTT after it
    stops.

    It also gives the user the option to set the maximum and minimum stepper motor positions via
    MQTT. (1) User doesn't have to pre-calculate the max/min travel distance (2) User can re-adjust
    max/min positions without reflashing firmware.
**/
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <Preferences.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include "motor_connections.h"


enum MotorState {
    MOTOR_IDLE,
    MOTOR_MIN,
    MOTOR_MAX,
    MOTOR_SET_MIN,
    MOTOR_SET_MAX
};


class Motor {
// TODO: add const
public:
    Motor();
    void init();
    void resetSettings();
    void moveTo(int newPos);
    void move(int percent);
    void min();
    void max();
    void setMin();
    void setMax();
    int  currentPercentage();
    void stop();
    void updatePosition();
    void run();
    void stallguardInterrupt();

private:
    #include "motor_settings.h"

    TMC2209Stepper driver = TMC2209Stepper(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
    FastAccelStepperEngine engine = FastAccelStepperEngine();
    FastAccelStepper *stepper = NULL;
    Preferences motorSettings;

    int maxPos;
    int currPos;
    int prevPos;
    MotorState currState;
    MotorState prevState;
    bool isMotorRunning;

    int percentToSteps(int percent) const; // Helper function to calculate position percentage to steps
    void loadSettings(); // Load motor settings from flash
    void setMotorState(MotorState newState);

    // TODO
    // void motorZero() {}
    // void motorSetCurrentPosition() {}
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