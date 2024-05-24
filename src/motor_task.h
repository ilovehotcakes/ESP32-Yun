#pragma once
/**
    motor_task.h - A class that contains all functions to control a two-phase bipolar stepper motor.
    Author: Jason Chen, 2022

    The MCU sends PWM signals to the stepper motor driver, TMC2209, to control the current to the
    motor and thereby moving the motor. Higher voltages and current generate more torque and higher
    RPMs. To operate the driver: (1) start the driver by taking it out of STANDBY, and (2) ENABLE
    the motor coils. Both are done automatically.

    The position of the system is absolute and it is translated between 3 different methods:
        (1) "Position" refers to the encoder's position, used for internal position tracking
        (2) "Steps" refers to the motor's position, used for moving the motor
        (3) "Percentage" refers to the overall percentage, used by UI
**/
#include <HardwareSerial.h>       // Hardwareserial for uart
#include <FunctionalInterrupt.h>  // std:bind()
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <AS5600.h>
#include "task.h"


#define DEFAULT_MOTOR_FULLSTEPS   200      // NEMA motors have 200 full steps/rev
#define DEFAULT_ENCODER_POSITIONS 4096.0f  // AS5600 absolute position is 12-bit


class MotorTask : public Task {
public:
    MotorTask(const uint8_t task_core);
    ~MotorTask();
    void addWirelessTask(Task *task);
    void addSystemSleepTimer(xTimerHandle timer);

protected:
    void run();

private:
    // TMCStepper library for interfacing with the stepper motor driver hardware, to read/write
    // registers for setting speed, acceleration, current, etc.
    TMC2209Stepper driver_ = TMC2209Stepper(&Serial1, R_SENSE, DRIVER_ADDR);

    // User adjustable TMC2209 motor driver settings, updated to driver registers via UART
    int   driver_stdby_  = false;
    int   sync_settings_ = true;  // If true, opeining/closing settings should be different
    float open_velocity_ = 3.0;
    float clos_velocity_ = 3.0;
    float open_accel_    = 0.5;
    float clos_accel_    = 0.5;
    int   open_current_  = 200;
    int   clos_current_  = 75;
    int   direction_     = false;
    int   microsteps_    = 16;
    int   full_steps_    = DEFAULT_MOTOR_FULLSTEPS;
    int   stallguard_en_ = true;
    int   coolstep_thrs_ = 0;
    int   stallguard_th_ = 10;
    int   spreadcycl_en_ = false;
    int   spreadcycl_th_ = 33;

    // FastAccelStepper library for generating PWM signal to the stepper driver to move/accelerate
    // and stop/deccelerate the stepper motor.
    FastAccelStepperEngine engine_ = FastAccelStepperEngine();
    FastAccelStepper *motor_       = NULL;

    // None user adjustable motor states. Managed by MotorTask.
    int8_t  last_updated_percent_ = -100;
    volatile bool stalled_        = false;
    portMUX_TYPE stalled_mux_     = portMUX_INITIALIZER_UNLOCKED;
    // bool motor_opening = false;
    // bool motor_closing = false;
    // bool motor_move_completed = false;

    // Rotary encoder for keeping track of motor's actual position because motor could slip and
    // cause the position to be incorrect. A closed-loop system.
    AS5600 encoder_;

    // Keeping track of the overall position via encoder's position and then  convert it into
    // motor's position and percentage.
    int32_t encod_pos_         = 0;
    int32_t encod_max_pos_     = static_cast<int32_t>(DEFAULT_ENCODER_POSITIONS) * 10;
    int   total_steps_         = full_steps_ * microsteps_;
    float motor_encoder_ratio_ = total_steps_ / DEFAULT_ENCODER_POSITIONS;
    float encoder_motor_ratio_ = DEFAULT_ENCODER_POSITIONS / total_steps_;

    Task *wireless_task_;              // To receive messages from wireless task
    xTimerHandle system_sleep_timer_;  // To prevent system from sleeping before motor stops

    void stallguardInterrupt();
    void loadSettings();  // Load motor settings from flash
    void prepareToMove(bool check, bool direction);
    void move(bool direction);
    void moveToStep(int target_step);
    void moveToPercent(int target_percent);
    void stop();
    bool setMin();
    bool setMax();
    bool motorEnable(uint8_t enable_pin, uint8_t value);
    void calculateTotalSteps();
    inline int getPercent();
    inline int positionToStep(int encoder_position);
    // For quick configuration guide, please refer to p70-72 of TMC2209's datasheet rev1.09
    // TMC2209's UART interface automatically becomes enabled when correct UART data is sent. It
    // automatically adapts to uC's baud rate. Block until UART is finished initializing so ESP32
    // can send settings to the driver via UART.
    void updateMotorSettings(float velocity, float acceleration, int current);
    void driverStartup();
    void driverStandby();
};