/**
    motor_connections.h - Specifying stepper driver GPIO connections
    Author: Jason Chen, 2023

    A file that specifies GPIO pins from ESP32 to stepper driver.

    To use this file:
      - Check and replace the GPIO connections from ESP32 to TMC2209 if needed (mainly EN_PIN,
        STEP_PIN, DIR_PIN).
      - If you don't plan on using StallGuard, comment out DIAG_PIN.

**/


// Define ESP32 connections to stepper motor driver (BigTreeTech TMC2209 V1.2 UART)
#define EN_PIN           23 // Enable pin
#define STEP_PIN         32 // Step pin
#define DIR_PIN          21 // Direction pin
#define DIAG_PIN         35 // Optional for StallGuard: diag pin
#define TXD2             22 // For Serial1
#define SERIAL_PORT Serial1 // TMC2209 HardwareSerial port
#define R_SENSE       0.11f // Sense resistor, TMC2209 uses 0.11, check your driver's sense resistor value
#define DRIVER_ADDR    0b00 // 0b00 is slave, since there're no other drivers