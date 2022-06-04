#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
#include "motor_settings.h"

static TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDR);
static AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
static Preferences preferences_local;

class Motor {
private:
  enum MotorState {
    MOTOR_DISABLE = 0,
    MOTOR_ENABLE  = 1,
  };

  enum CoverState {
    COVER_CLOSE   = -1,
    COVER_STOP    = -2,
    COVER_OPEN    = -3,
    COVER_SET_MAX = -4,
    COVER_SET_MIN = -5
  };
  
  MotorState motor = MOTOR_DISABLE;
  CoverState cove  = COVER_STOP;

  uint32_t max_steps;
  uint32_t current_position;
  uint32_t previous_position = 0;
  bool set_max = false;
  bool set_min = false;

  void load_preference();
  void updatePosition();
  void moveToPosition(int);
  void stopMotor();
  void setMax();
  void setMin();
  void sendPercentage();
  int percentToSteps(int);
  int stepsToPercent(int);

public:
  Motor();
  void run();
};