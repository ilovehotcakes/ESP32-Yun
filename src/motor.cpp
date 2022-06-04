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
  stepper.setCurrentPosition(current_position);
  stepper.disableOutputs();

  load_preference();
  setMax();
}


void Motor::load_preference() {
  max_steps = preferences_local.getInt("max_steps", 100000);
  current_position = preferences_local.getInt("current_position", 0);
}


void Motor::updatePosition() {
  current_position = stepper.currentPosition();
  preferences_local.putInt("current_position", current_position);
}


void Motor::sendPercentage() {
  // mqttClient.beginMessage(outTopic);
  // mqttClient.print(open_percent);
  // mqttClient.endMessage();
  // Todo: setup #ifdef
  // Serial.print("Sent MQTT message: ");
  // Serial.print(open_percent);
  // Serial.print("% = ");
  // Serial.print(stepper.currentPosition());
  // Serial.print(" / ");
  // Serial.println(max_steps);
}


// Only enable the driver if the distance isn't 0, or else the drive will be enabled and won't be disable unless STOP is explicitly used
void Motor::moveToPosition(int position) {
  stepper.moveTo(position);
  if (stepper.distanceToGo() != 0) {
    motor = MOTOR_ENABLE;
    stepper.enableOutputs();
    stepper.setMaxSpeed(velocity);
  }
}


void Motor::stopMotor() {
  motor = MOTOR_DISABLE;
  if (set_max == true) {
    set_max = false;
    max_steps = stepper.currentPosition();
    preferences_local.putInt("max_steps", max_steps);
  } else if (set_min == true) {
    set_min = false;
    int distance_traveled = 2147483646 - stepper.currentPosition();
    max_steps = max_steps + distance_traveled - previous_position;
    stepper.setCurrentPosition(0);
  }
  stepper.moveTo(stepper.currentPosition());
  stepper.disableOutputs();
  updatePosition();
  sendPercentage();
}


int Motor::percentToSteps(int percent) {
  float result = (float) percent * (float) max_steps / 100.0;
  return (int) round(result);
}


int Motor::stepsToPercent(int steps) {
  float result = (float) current_position / (float) max_steps * 100;
  return (int) round(result);
}


void Motor::setMax() {
  set_max = true;
  stepper.setMaxSpeed(velocity / 4);
  moveToPosition(2147483646);
}


void Motor::setMin() {
  set_min = true;
  stepper.setMaxSpeed(velocity / 4);
  previous_position = stepper.currentPosition();
  stepper.setCurrentPosition(2147483646);
  moveToPosition(0);
}


void Motor::run() {
  if (motor == MOTOR_ENABLE) {
    if (stepper.distanceToGo() != 0)
      stepper.run();
    else if (stepper.distanceToGo() == 0)
      stopMotor();
  }
}