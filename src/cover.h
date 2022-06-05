#ifndef __COVER_H__
#define __COVER_H__

#define MOTOR_DISABLE  0
#define MOTOR_ENABLE   1

#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include "cover_settings.h"

static TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
static Preferences preferences_local;

class Cover {
private:
  int motor = MOTOR_DISABLE;

  uint32_t max_steps;
  uint32_t current_position;
  uint32_t previous_position = 0;
  bool set_max = false;
  bool set_min = false;

  void moveTo(int);
  void updatePosition();
  void load_preference();
  int percentToSteps(int);
  int stepsToPercent(int);

public:
  Cover();
  void run();
  void stop();
  void open();
  void close();
  void percent(int);
  void setMax();
  void setMin();
};

#endif
