/**
  motor_settings.h - Specific hardware settings for the stepper driver and motor
  Author: Jason Chen, 2022

  A file that includes the hardware settings for stepper driver (BTT TMC2209 V1.2)
  and stepper motor (NEMA 11); and connections from the driver to ESP32.

  To use this file:
  - Check and replace the connections from ESP32 to TMC2209 if needed (mainly EN_PIN,
    STEP_PIN, DIR_PIN)
  - If you don't plan on using stallguard, comment out DIAG_PIN and RXD2
  - Check and modify stepper motor specifications. I use a NEMA11 with a 5.18 planetary
    gearbox because that gives me enough torque to move the shades while being small
    enough to fit inside the top compartment.
**/

// Define LED pin, it's tied to GPIO2 on the HiLetGo board
#define LED_PIN           2

// Define ESP32 connections to stepper motor driver (BigTreeTech TMC2209 V1.2 UART)
#define EN_PIN           23 // Enable pin
#define STEP_PIN         32 // Step pin
#define DIR_PIN          21 // Direction pin
#define DIAG_PIN         35 // Optional for StallGuard: diag pin
#define RXD2             17 // Optional for StallGuard: must be connected to a ESP32 UART TX port
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Sense resistor, TMC2209 uses 0.11, check your driver's sense resistor value
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers

// Stepper motor specifications (NEMA 11 with 5.18:1 planetary gearbox)
const float gearbox_ratio = 5.18;          // Use 1 if stepper motor doesn't have a gearbox
const int microsteps = 8;                  // 8 microsteps per full step
const int steps_per_rev = 200 * gearbox_ratio * microsteps;  // NEMA motors have 200 full steps per rev
const int max_speed = steps_per_rev * 1;   // Max speed in Hz
const int acceleration = max_speed * 0.75; // Use lower value if using SG