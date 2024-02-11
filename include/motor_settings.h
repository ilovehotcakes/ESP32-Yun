/**
    motor_settings.h - Specifying stepper driver settings and motor specifications
    Author: Jason Chen, 2022
    Modified: 10/30/2023

    A file that includes settings for stepper motor and driver.

    To use this file:
      - Check and modify stepper motor specifications. Current setting is for a NEMA11 with a
        5.18:1 planetary gearbox.
      - Check and modify stepper motor driver settings. 
**/
// TMC2209 driver settings
int microsteps = 8;                 // 8 microsteps per full step
int stepsPerRev = 200 * microsteps;  // NEMA motors have 200 full steps/rev
int maxSpeed = (int) stepsPerRev * 4;   // Max speed in Hz; Needs to be large enough to not trip SG
int acceleration = (int) maxSpeed * 0.5;  // Use lower value if using SG

bool flipDir = false;
bool enableSG = true;  // Default false
// bool quietMode = false;
int sgThreshold = 10;
int openingRMS = 550;
int closingRMS = 200;  // 1, 3: 200; 2: 400; 4: 300