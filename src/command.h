#pragma once
#include <Arduino.h>


enum Command {
    GET_MOTOR_POS  = 0,
    ERROR_COMMAND  = INT_MIN,

    // Motor commands > 0
    MOTOR_MOVE       = 1,
    MOTOR_STOP       = 2,
    MOTOR_SET_MIN    = 3,
    MOTOR_SET_MAX    = 4,
    MOTOR_STNDBY     = 5,
    MOTOR_OPENCLOSE  = 6,
    MOTOR_SET_VELO   = 7,
    MOTOR_SET_OPVELO = 8,
    MOTOR_SET_CLVELO = 9,
    MOTOR_SET_ACCL   = 10,
    MOTOR_SET_OPACCL = 11,
    MOTOR_SET_CLACCL = 12,
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
    SYSTEM_STNDBY    = -1,
    SYSTEM_REBOOT    = -2,
    SYSTEM_RESET     = -3
};


Command hash (String command);
String hash (Command command);
String listMotorCommands();