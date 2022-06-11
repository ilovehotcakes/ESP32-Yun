/**
  motor.h - A wrapper class for TMCStepper and AccelStepper
  Author: Jason Chen

  This is a wrapper class so it's simpler for the driver program to manage the
  stepper motor. A motor object remember its last and maximum position so the
  user doesn't have to set it everytime after reboot.

  It also gives the user the option to set the maximum and minimum stepper motor
  positions via MQTT. (1) User doesn't have to pre-calculate the max/min travel
  distance (2) User can still re-adjust max/min positions if the stepper motor
  slips.

  The motor class communicates with the driver program via setting the msgAvail
  flag to true and the driver can retrieve the message via isMessageAvailable()
  but the user must set msgAvail flag back to false after reading the message.
**/
#define MOTOR_DISABLE  0
#define MOTOR_ENABLE   1

#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include "motor_settings.h"

TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
Preferences memory;
void sendMqtt(String);


int motor = MOTOR_DISABLE;
int stallVal = 10; // [0..255]
uint32_t maxPos;
uint32_t currPos;
uint32_t prevPos = 0;
bool isSetMax = false;
bool isSetMin = false;
bool isStalled = false;


// Returns current rounded position percentage. 0 is closed.
int motorCurrentPosition() {
  if (currPos == 0)
    return 0;
  else
    return (int) round((float) currPos / (float) maxPos * 100);
}


void motorStop() {
  motor = MOTOR_DISABLE;

  // Re-calcuate max/min positions
  if (isSetMax) {
    isSetMax = false;
    maxPos = stepper.currentPosition();
    memory.putInt("maxPos", maxPos);
    stepper.setMaxSpeed(velocity);  // Set stepper motor speed back to normal
  } else if (isSetMin) {
    isSetMin = false;
    int distanceTraveled = 2147483646 - stepper.currentPosition();
    maxPos = maxPos + distanceTraveled - prevPos;
    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(velocity);  // Set stepper motor speed back to normal
  }

  // Stop stepper motor and disable driver
  stepper.moveTo(stepper.currentPosition());
  stepper.disableOutputs();

  // Updated current position
  currPos = stepper.currentPosition();
  memory.putInt("currPos", currPos);

  // Callback
  // sendMqtt((String) motorCurrentPosition());
}


void IRAM_ATTR stallInterrupt() {
  // motor = MOTOR_DISABLE;
  stepper.setAcceleration(200000);
  stepper.moveTo(stepper.currentPosition());
  Serial.println("Motor stalled");
  // motorStop();
}


void loadPositions() {
  // memory.begin("local", false);
  maxPos = memory.getInt("maxPos", 100000);
  currPos = memory.getInt("currPos", 0);
}


void motorSetup() {
  pinMode(DIAG_PIN, INPUT);

  // Driver setup
  SERIAL_PORT.begin(115200);  // Initialize hardware serial for hardware UART driver
  driver.pdn_disable(true);   // Enable UART on TMC2209
  driver.begin();             // Begin sending data
  driver.toff(4);             // Enables driver in software
  driver.rms_current(600);    // Motor RMS current "rms_current will by default set ihold to 50% of irun but you can set your own ratio with additional second argument; rms_current(1000, 0.3)."
  driver.pwm_autoscale(true);    // Needed for stealthChop
  driver.en_spreadCycle(false);  // Toggle spreadCycle on TMC2208/2209/2224
  driver.blank_time(24);
  driver.microsteps(microsteps);

  driver.semin(5);
  driver.semax(2);
  driver.sedn(0b01);
  driver.TCOOLTHRS((3089838.00 * pow(float(velocity), -1.00161534)) * 1.5);
  driver.SGTHRS(60);
  // attachInterrupt(digitalPinToInterrupt(DIAG_PIN), stallInterrupt, RISING);

  // Motor setup
  stepper.setEnablePin(EN_PIN);
  stepper.setMaxSpeed(velocity);
  stepper.setAcceleration(acceleration);
  stepper.setPinsInverted(false, false, true);
  stepper.setCurrentPosition(currPos);
  stepper.disableOutputs();

  // Load previous position and maximum position
  loadPositions();
}


// Only enable the driver if the distance isn't 0, or else the drive will be
// enabled and won't be disable unless STOP is explicitly used.
void motorMoveTo(int steps) {
  stepper.moveTo(steps);
  if (stepper.distanceToGo() != 0) {
    motor = MOTOR_ENABLE;
    stepper.enableOutputs();
  }
}


void motorRun() {
  if (motor == MOTOR_ENABLE) {
    if (stepper.distanceToGo() != 0)
      stepper.run();
    else
      motorStop();
  }
}


void motorOpen() {
  motorMoveTo(0);
}


void motorClose() {
  motorMoveTo(maxPos);
}


void motorSetMax() {
  isSetMax = true;
  stepper.setMaxSpeed(velocity / 4);
  motorMoveTo(2147483646);
}


void motorSetMin() {
  isSetMin = true;
  stepper.setMaxSpeed(velocity / 4);
  prevPos = stepper.currentPosition();
  stepper.setCurrentPosition(2147483646);
  motorMoveTo(0);
}


int percentToSteps(int percent) {
  float result = (float) percent * (float) maxPos / 100.0;
  return (int) round(result);
}