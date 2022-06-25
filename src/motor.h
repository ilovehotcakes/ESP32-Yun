/**
  motor.h - A file that contains all stepper motor controls
  Author: Jason Chen

  This file contains all stepper motor controls, which includes, initializing
  the stepper driver (TMCStepper), stepper motor control (FastAccelStepper), as
  well as recalling its current position and maximum position on reboot. It also
  sends current position via MQTT after it stops.

  It also gives the user the option to set the maximum and minimum stepper motor
  positions via MQTT. (1) User doesn't have to pre-calculate the max/min travel
  distance (2) User can still re-adjust max/min positions if the stepper motor
  slips.
**/
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <Preferences.h>
#include "motor_settings.h"

#define MOTOR_STOPPED  -1
#define MOTOR_OPENING  -2
#define MOTOR_CLOSING  -3
#define MOTOR_SET_MIN  -4
#define MOTOR_SET_MAX  -5

TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper;
Preferences settings;
void sendMqtt(String);

int maxPos;
int currPos;
int prevPos = 0;
int currState = MOTOR_STOPPED;
int prevState = MOTOR_STOPPED;
bool isMotorRunning = false;
bool flipDir = false;


void IRAM_ATTR stallguardInterrupt() {
  stepper->forceStop();
  Serial.println("[I] Motor stalled");
}


void loadPositions() {
  settings.begin("local", false);
  maxPos = settings.getInt("maxPos", 30000);
  currPos = settings.getInt("currPos", 0);
  stepper->setCurrentPosition(currPos);
}


// TODO: add driver.shaft(bool) so user can reverse directions
void motorSetup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  // Stepper driver setup
  SERIAL_PORT.begin(115200);  // Initialize hardware serial for hardware UART driver
  driver.begin();             // Begin sending data
  driver.toff(4);             // Not used in StealthChop but required to enable the motor, 0=off
  driver.pdn_disable(true);   // PDN_UART input disabled; set this bit when using the UART interface
  driver.rms_current(475);    // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
  driver.pwm_autoscale(true);    // Needed for StealthChop
  driver.en_spreadCycle(false);  // Disable SpreadCycle; SC is faster but louder
  driver.blank_time(24);         // Comparator blank time. Needed to safely cover the switching event and the duration of the ringing on the sense resistor.
  driver.microsteps(microsteps);
  // driver.shaft(flipDir);  // Test

  // Use Stallguard if the DIAG_PIN and RXD2(UART2) are defined
  #if defined(DIAG_PIN) && defined(RXD2)
    pinMode(DIAG_PIN, INPUT);
    driver.semin(0);    // CoolStep/SmartEnergy 4-bit uint that sets lower threshold, 0 disable
    driver.TCOOLTHRS((3089838.00 * pow(float(max_speed), -1.00161534)) * 1.5);  // Lower threshold velocity for switching on smart energy CoolStep and StallGuard to DIAG output
    driver.SGTHRS(10);  // [0..255] the higher the more sensitive to stall
    attachInterrupt(DIAG_PIN, stallguardInterrupt, RISING);
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
    Serial.println("[E] Please use a different GPIO pin for the STEP_PIN. The current pin is incompatible..");
  }

  // Load current position and maximum position from settings
  loadPositions();
}


int percentToSteps(int percent) {
  float steps = (float) percent * (float) maxPos / 100.0;
  return (int) round(steps);
}


// Stepper must move first before isMotorRunning==true; else motorRun will excute first before stepper stops
void motorMoveTo(int newPos) {
  if ((prevState == MOTOR_CLOSING && currState == MOTOR_OPENING)
  || (prevState == MOTOR_OPENING && currState == MOTOR_CLOSING))
    stepper->forceStop();
  
  if (newPos != stepper->getCurrentPosition() && newPos <= maxPos) {
    stepper->moveTo(newPos);
    isMotorRunning = true;
  }
}


void motorMove(int percent) {
  motorMoveTo(percentToSteps(percent));
}


void motorMin() {
  prevState = currState;
  currState = MOTOR_OPENING;
  driver.rms_current(475);
  motorMoveTo(0);
}


void motorMax() {
  prevState = currState;
  currState = MOTOR_CLOSING;
  driver.rms_current(475);
  motorMoveTo(maxPos);
}


void motorSetMin() {
  prevState = currState;
  currState = MOTOR_SET_MIN;
  driver.rms_current(350);
  stepper->setSpeedInHz(max_speed / 4);
  prevPos = stepper->getCurrentPosition();
  stepper->setCurrentPosition(2147483646);
  motorMoveTo(0);
}


void motorSetMax() {
  prevState = currState;
  currState = MOTOR_SET_MAX;
  driver.rms_current(475);
  stepper->setSpeedInHz(max_speed / 4);
  maxPos = 2147483646;
  motorMoveTo(2147483646);
}


// Returns current rounded position percentage. 0 is closed.
int motorCurrentPercentage() {
  if (currPos == 0)
    return 0;
  else
    return (int) round((float) currPos / (float) maxPos * 100);
}


void motorStop() {
  stepper->forceStop();
  stepper->moveTo(stepper->getCurrentPosition());
  if (currState == MOTOR_OPENING || currState == MOTOR_CLOSING) {
    prevState = currState;
    currState = MOTOR_STOPPED;
  }
}


void motorRun() {
  if (isMotorRunning) {
    vTaskDelay(1);
    if (!stepper->isRunning()) {
      isMotorRunning = false;

      if (currState == MOTOR_SET_MAX) {
        maxPos = stepper->getCurrentPosition();
        settings.putInt("maxPos", maxPos);
        stepper->setSpeedInHz(max_speed);  // Set stepper motor speed back to normal
      } else if (currState == MOTOR_SET_MIN) {
        int distanceTraveled = 2147483646 - stepper->getCurrentPosition();
        maxPos = maxPos + distanceTraveled - prevPos;
        stepper->setCurrentPosition(0);
        stepper->setSpeedInHz(max_speed);  // Set stepper motor speed back to normal
      }
      prevState = currState;
      currState = MOTOR_STOPPED;
      currPos = stepper->getCurrentPosition();
      settings.putInt("currPos", currPos);
      sendMqtt((String) motorCurrentPercentage());
    }
  }
}