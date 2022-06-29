/**
  motor.h - A file that contains all stepper motor controls
  Author: Jason Chen, 2022

  This file contains all stepper motor controls, which includes, initializing
  the stepper driver (TMCStepper), stepper motor control (FastAccelStepper), as
  well as recalling its previous position and maximum position on reboot. It also
  sends current position via MQTT after it stops.

  It also gives the user the option to set the maximum and minimum stepper motor
  positions via MQTT. (1) User doesn't have to pre-calculate the max/min travel
  distance (2) User can re-adjust max/min positions without reflashing firmware.
**/
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <Preferences.h>
#include "motor_settings.h"

TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper;
Preferences motorSettings;
void sendMqtt(String);
enum MotorState { MOTOR_IDLE, MOTOR_MIN, MOTOR_MAX, MOTOR_SET_MIN, MOTOR_SET_MAX };

int maxPos;
int currPos;
int prevPos = 0;
MotorState currState = MOTOR_IDLE;
MotorState prevState = MOTOR_IDLE;
bool isMotorRunning = false;
bool flipDir = false;
bool enableSG = true;  // Default false
int sgThreshold = 10;
int openingRMS = 475;
int closingRMS = 475;


// For StallGuard
#if defined(DIAG_PIN) && defined(RXD2)
void IRAM_ATTR stallguardInterrupt() {
  stepper->forceStop();
  LOGE("Motor stalled");
}
#endif


// Load motor settings from flash
void loadMotorSettings() {
  motorSettings.begin("local", false);
  maxPos = motorSettings.getInt("maxPos", 30000);
  currPos = motorSettings.getInt("currPos", 0);
  stepper->setCurrentPosition(currPos);
  LOGI("Motor settings loaded(curr/max): %d/%d", currPos, maxPos);
}


void motorSetup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  // Stepper driver setup
  SERIAL_PORT.begin(115200);  // Initialize hardware serial for hardware UART driver
  driver.begin();             // Begin sending data
  driver.toff(4);             // Not used in StealthChop but required to enable the motor, 0=off
  driver.pdn_disable(true);   // PDN_UART input disabled; set this bit when using the UART interface
  driver.rms_current(closingRMS); // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
  driver.pwm_autoscale(true);     // Needed for StealthChop
  driver.en_spreadCycle(false);   // Disable SpreadCycle; SpreadCycle is faster but louder
  driver.blank_time(24);          // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
  driver.microsteps(microsteps);
  driver.shaft(flipDir);

  // Use StallGuard if user specifies connection to DIAG_PIN && RXD2
  #if defined(DIAG_PIN) && defined(RXD2)
  if (enableSG) {
    pinMode(DIAG_PIN, INPUT);
    driver.semin(0);              // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0 is disable
    driver.TCOOLTHRS((3089838.00 * pow(float(max_speed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on CoolStep and StallGuard to DIAG
    driver.SGTHRS(sgThreshold);   // [0..255] the higher the more sensitive to stall
    attachInterrupt(DIAG_PIN, stallguardInterrupt, RISING);
  }
  #endif

  // Stepper motor setup
  engine.init();
  stepper = engine.stepperConnectToPin(STEP_PIN);
  if (stepper) {
    stepper->setEnablePin(EN_PIN);
    stepper->setDirectionPin(DIR_PIN);
    stepper->setSpeedInHz(max_speed);
    stepper->setAcceleration(acceleration);
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(200);
  } else {
    LOGE("Please use a different GPIO pin for STEP_PIN. The current pin is incompatible..");
  }

  // Load current position and maximum position from motorSettings
  loadMotorSettings();
  LOGI("Motor setup complete");
}


// Helper function to calculate position percentage to steps
int percentToSteps(int percent) {
  float x = (float) percent * (float) maxPos / 100.0;
  return (int) round(x);
}


// Motor must move first before isMotorRunning==true, else motorRun will excute first before stepper stops
void motorMoveTo(int newPos) {
  if ((prevState == MOTOR_MAX && currState == MOTOR_MIN)
  || (prevState == MOTOR_MIN && currState == MOTOR_MAX))
    stepper->forceStop();
  
  if (newPos != stepper->getCurrentPosition() && newPos <= maxPos) {
    stepper->moveTo(newPos);
    isMotorRunning = true;
    LOGD("Motor moving(tar/curr/max): %d/%d/%d", newPos, stepper->getCurrentPosition(), maxPos);
  }
}


void motorMove(int percent) {
  motorMoveTo(percentToSteps(percent));
}


void setMotorState(MotorState newState) {
  prevState = currState;
  currState = newState;
}


void motorMin() {
  setMotorState(MOTOR_MIN);
  driver.rms_current(openingRMS);
  stepper->setSpeedInHz(max_speed);
  motorMoveTo(0);
}


void motorMax() {
  setMotorState(MOTOR_MAX);
  driver.rms_current(closingRMS);
  stepper->setSpeedInHz(max_speed);
  motorMoveTo(maxPos);
}


void motorSetMin() {
  setMotorState(MOTOR_SET_MIN);
  driver.rms_current(openingRMS);
  stepper->setSpeedInHz(max_speed / 4);
  prevPos = stepper->getCurrentPosition();
  stepper->setCurrentPosition(INT_MAX);
  LOGD("Motor setting new min position");
  motorMoveTo(0);
}


void motorSetMax() {
  setMotorState(MOTOR_SET_MAX);
  driver.rms_current(closingRMS);
  stepper->setSpeedInHz(max_speed / 4);
  maxPos = INT_MAX;
  LOGD("Motor setting new max position");
  motorMoveTo(INT_MAX);
}


// Returns current rounded position percentage. 0 is closed.
// TODO: add inaccuracy mode
int motorCurrentPercentage() {
  if (currPos == 0)
    return 0;
  else
    return (int) round((float) currPos / (float) maxPos * 100);
}


void motorStop() {
  stepper->forceStop();
  stepper->moveTo(stepper->getCurrentPosition());
  setMotorState(MOTOR_IDLE);
}


void updatePosition() {
  if (prevState == MOTOR_SET_MAX) {
    maxPos = stepper->getCurrentPosition();
    motorSettings.putInt("maxPos", maxPos);
    LOGD("Set max position, new max position: %d", maxPos);
  } else if (prevState == MOTOR_SET_MIN) {
    int distanceTraveled = INT_MAX - stepper->getCurrentPosition();
    maxPos = maxPos + distanceTraveled - prevPos;
    motorSettings.putInt("maxPos", maxPos);
    stepper->setCurrentPosition(0);
    LOGD("Set min position, new max position: %d", maxPos);
  }

  setMotorState(MOTOR_IDLE);
  currPos = stepper->getCurrentPosition();
  motorSettings.putInt("currPos", currPos);
  LOGD("Motor stopped(curr/max): %d/%d", currPos, maxPos);
}


void motorRun() {
  if (isMotorRunning) {
    vTaskDelay(1);
    if (!stepper->isRunning()) {
      isMotorRunning = false;

      updatePosition();

      sendMqtt((String) motorCurrentPercentage());
    }
  }
}