#include <TMCStepper.h>
#include <AccelStepper.h>
#include <Preferences.h>
// Define connections to BTT TMC2209 V1.2 UART
#define EN_PIN           23 // Enable
#define STEP_PIN         22 // Steps
#define DIR_PIN          21 // Direction
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Match to your driver, SilentStepStick series use 0.11
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers

class Motor {
private:
  enum motorState {
    motorIdle = 0,
    motorUp   = 1,
    motorDown = 2,
  };

  enum command {
    close  = -1,
    stop   = -2,
    open   = -3,
    setMax = -4,
    setMin = -5
  };
  
  // TMC2209Stepper driver;
  // AccelStepper stepper;
  // Preferences preferences_local;

  // uint8_t open_percent;
  // uint32_t max_steps;
  // uint32_t current_position;

  // const float gear_ratio = 5.18;  // Use 1 if stepper motor doesn't have a gearbox
  // const uint16_t microsteps = 8;
  // const int steps_per_rev = 200 * gear_ratio * microsteps;
  // int velocity = steps_per_rev;
  // int acceleration = steps_per_rev * 10;
  // int motor_flag = motorIdle;
  // uint32_t previous_position = 0;
  // bool set_max = false;
  // bool set_min = false;

public:
  Motor();
  // void moveToPosition(int position);
  // void stopMotor();
  // void updatePosition();
  // void sendPercentage();
  // int percentToSteps(int percent);
  // int stepsToPercent(int steps);
  // void setMax();
  // void setMin();
};