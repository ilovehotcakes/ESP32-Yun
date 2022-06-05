#ifndef __MOTOR_H__
#define __MOTOR_H__

#define MOTOR_DISABLE  0
#define MOTOR_ENABLE   1

#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include "motor_settings.h"

static TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
static Preferences memory;

class Motor {
private:
  int motor = MOTOR_DISABLE;
  bool msgAvail = false;

  uint32_t maxPos;
  uint32_t currPos;
  uint32_t prevPos = 0;
  bool set_max = false;
  bool set_min = false;

  void loadPositions();
  void moveTo(int);
  void updatePosition();
  int percentToSteps(int);

public:
  Motor();
  int currentPosition();
  int isMessageAvailable();
  void markMessageRead();
  void run();
  void percent(int);
  void stop();
  void open();
  void close();
  void setMax();
  void setMin();
};

#endif