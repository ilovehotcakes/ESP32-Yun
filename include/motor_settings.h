/**
  motor_settings.h - Specific settings for the stepper driver and motor
  Author: Jason Chen

  A file that includes all the stepper driver and motor settings instead of going
  through main.

  To use this file:
  - Replaces the connections from ESP32 to TMC2209 (mainly, EN_PIN, STEP_PIN, DIR_PIN)
  - Check and modify stepper motor specifications. I use a NEMA11 with a 5.18 planetary
    gearbox because that gives me enough torque to move the shades while being small
    enough to fit inside the shades top compartment.
**/


// Define connections to BigTreeTech TMC2209 V1.2 UART
#define EN_PIN           23 // Enable
#define STEP_PIN         22 // Steps
#define DIR_PIN          21 // Direction
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Match to your driver, SilentStepStick series use 0.11
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers

// Stepper motor specifications
const float gearbox_ratio = 5.18;        // Use 1 if stepper motor doesn't have a gearbox
const uint16_t microsteps = 8;           // 8 is a good in-between, powerful enough but also quiet
const int steps_per_rev = 200 * gearbox_ratio * microsteps;  // NEMA motors typically have 200 full steps per rev
const int velocity = steps_per_rev * 1;  // For 5.18:1 planetary gearbox, x1 is the fastest it will go
const int acceleration = steps_per_rev * 5;
