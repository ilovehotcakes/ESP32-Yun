#pragma once
#include <Arduino.h>


enum Command {
    GET_MOTOR_POS  = 0,
    ERROR_COMMAND  = INT_MIN,

    // Motor commands > 0
    MOTOR_PERECENT   = 1,
    MOTOR_STOP       = 2,
    MOTOR_SET_MIN    = 3,
    MOTOR_SET_MAX    = 4,
    MOTOR_STDBY      = 5,
    MOTOR_OPENCLOSE  = 6,
    MOTOR_VELOCITY   = 7,
    MOTOR_OPVELOCITY = 8,
    MOTOR_CLVELOCITY = 9,
    MOTOR_ACCEL      = 10,
    MOTOR_OPACCEL    = 11,
    MOTOR_CLACCEL    = 12,
    MOTOR_CURRENT    = 13,
    MOTOR_OPCURRENT  = 14,
    MOTOR_CLCURRENT  = 15,
    MOTOR_DIRECTION  = 16,
    MOTOR_MICROSTEPS = 17,
    MOTOR_FULLSTEPS  = 18,
    MOTOR_STALLGUARD = 19,
    MOTOR_TCOOLTHRS  = 20,
    MOTOR_SGTHRS     = 21,
    MOTOR_SPREADCYCL = 22,
    MOTOR_TPWMTHRS   = 23,

    // System commands < 0
    SYSTEM_SLEEP     = -1,
    SYSTEM_REBOOT    = -2,
    SYSTEM_RESET     = -3
};


Command hash (String command);
String hash (Command command);
String listMotorCommands();
String listSystemCommands();