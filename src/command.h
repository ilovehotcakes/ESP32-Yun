#pragma once
#include <Arduino.h>


enum Command {
    UPDATE_POSITION  = 0,
    ERROR_COMMAND  = INT_MIN,

    // Motor commands > 0
    MOTOR_STOP       = 1,
    MOTOR_PERECENT   = 2,
    MOTOR_STEP       = 3,
    MOTOR_FORWARD    = 4,
    MOTOR_BACKWARD   = 5,
    MOTOR_SET_MIN    = 6,
    MOTOR_SET_MAX    = 7,
    MOTOR_STANDBY    = 8,
    MOTOR_SYNC_STTNG = 9,
    MOTOR_VLCTY      = 10,
    MOTOR_OP_VLCTY   = 11,
    MOTOR_CL_VLCTY   = 12,
    MOTOR_ACCEL      = 13,
    MOTOR_OP_ACCEL   = 14,
    MOTOR_CL_ACCEL   = 15,
    MOTOR_CURRENT    = 16,
    MOTOR_OP_CURRENT = 17,
    MOTOR_CL_CURRENT = 18,
    MOTOR_DIRECTION  = 19,
    MOTOR_FULL_STEPS = 20,
    MOTOR_MICROSTEPS = 21,
    MOTOR_STALLGUARD = 22,
    MOTOR_TCOOLTHRS  = 23,
    MOTOR_SGTHRS     = 24,
    MOTOR_SPREADCYCL = 25,
    MOTOR_TPWMTHRS   = 26,

    // System commands < 0
    SYSTEM_SLEEP     = -1,
    SYSTEM_RESTART   = -2,
    SYSTEM_RESET     = -3,

    WIRELESS_SETUP   = -51,
    WIRELESS_SSID    = -52,
    WIRELESS_PASS    = -53
};


Command hash (String command);
String hash (Command command);
String listMotorCommands();
String listSystemCommands();
String listWirelessCommands();