#include "motor.h"

Motor::Motor() {
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

  // Motor setup
  stepper.setEnablePin(EN_PIN);
  stepper.setMaxSpeed(velocity);
  stepper.setAcceleration(acceleration);
  stepper.setPinsInverted(false, false, true);
  stepper.setCurrentPosition(currPos);
  stepper.disableOutputs();

  loadPositions();
}


// Only enable the driver if the distance isn't 0, or else the drive will be enabled and won't be disable unless STOP is explicitly used
void Motor::moveTo(int steps) {
  stepper.moveTo(steps);
  if (stepper.distanceToGo() != 0) {
    motor = MOTOR_ENABLE;
    stepper.enableOutputs();
  }
}


int Motor::currentPosition() {
  if (currPos == 0)
    return 0;
  else
    return (int) round((float) currPos / (float) maxPos * 100);
}


int Motor::isMessageAvailable() {
  return msgAvail;
}


void Motor::markMessageRead() {
  msgAvail = false;
}


void Motor::run() {
  if (motor == MOTOR_ENABLE) {
    if (stepper.distanceToGo() != 0)
      stepper.run();
    else
      stop();
  }
}


void Motor::percent(int percent) {
  moveTo(percentToSteps(percent));
}


void Motor::stop() {
  motor = MOTOR_DISABLE;
  if (set_max) {
    set_max = false;
    maxPos = stepper.currentPosition();
    memory.putInt("maxPos", maxPos);
  } else if (set_min) {
    set_min = false;
    int distance_traveled = 2147483646 - stepper.currentPosition();
    maxPos = maxPos + distance_traveled - prevPos;
    stepper.setCurrentPosition(0);
  }
  stepper.moveTo(stepper.currentPosition());
  stepper.disableOutputs();
  stepper.setMaxSpeed(velocity);
  updatePosition();
  msgAvail = true;
}


void Motor::open() {
  moveTo(0);
}


void Motor::close() {
  moveTo(maxPos);
}


void Motor::setMax() {
  set_max = true;
  stepper.setMaxSpeed(velocity / 4);
  moveTo(2147483646);
}


void Motor::setMin() {
  set_min = true;
  stepper.setMaxSpeed(velocity / 4);
  prevPos = stepper.currentPosition();
  stepper.setCurrentPosition(2147483646);
  moveTo(0);
}


void Motor::loadPositions() {
  maxPos = memory.getInt("maxPos", 100000);
  currPos = memory.getInt("currPos", 0);
}


void Motor::updatePosition() {
  currPos = stepper.currentPosition();
  memory.putInt("currPos", currPos);
}


int Motor::percentToSteps(int percent) {
  float result = (float) percent * (float) maxPos / 100.0;
  return (int) round(result);
}

