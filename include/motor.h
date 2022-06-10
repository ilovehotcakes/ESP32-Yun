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

#ifndef __MOTOR_H__
#define __MOTOR_H__

#define MOTOR_DISABLE  0
#define MOTOR_ENABLE   1

#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include <functional>
#include "motor_settings.h"

static TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

class Motor {
private:
  int motor = MOTOR_DISABLE;
  uint32_t maxPos;
  uint32_t currPos;
  uint32_t prevPos = 0;
  bool isSetMax = false;
  bool isSetMin = false;
  std::function<void(String)> callback;
  Preferences memory;

  void loadPositions();
  void moveTo(int);
  int percentToSteps(int);

public:
  Motor();
  Motor(std::function<void(String)>);
  int currentPosition();
  void run();
  void percent(int);
  void stop();
  void open();
  void close();
  void setMax();
  void setMin();
};

#endif