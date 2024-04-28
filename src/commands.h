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
    MOTOR_SET_DIR    = 6,
    MOTOR_SET_MICSTP = 7,
    MOTOR_SET_FULSTP = 8,
    MOTOR_SET_VELOC  = 9,
    MOTOR_SET_ACCEL  = 10,
    MOTOR_SET_OPCUR  = 11,
    MOTOR_SET_CLCUR  = 12,
    MOTOR_ENABLE_SG  = 13,
    MOTOR_SET_SGTHR  = 14,

    // System commands < 0
    SYSTEM_STNDBY    = -1,
    SYSTEM_RESET     = -2,
    SYSTEM_REBOOT    = -3
};


Command hash (String command);
String hash (Command command);
String listMotorCommands();