#pragma once
#include <Arduino.h>
#include <FunctionalInterrupt.h>


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
    MOTOR_ZERO       = 8,
    MOTOR_STANDBY    = 9,
    MOTOR_SYNC_STTNG = 10,
    MOTOR_VLCTY      = 11,
    MOTOR_OP_VLCTY   = 12,
    MOTOR_CL_VLCTY   = 13,
    MOTOR_ACCEL      = 14,
    MOTOR_OP_ACCEL   = 15,
    MOTOR_CL_ACCEL   = 16,
    MOTOR_CURRENT    = 17,
    MOTOR_OP_CURRENT = 18,
    MOTOR_CL_CURRENT = 19,
    MOTOR_DIRECTION  = 20,
    MOTOR_FULL_STEPS = 21,
    MOTOR_MICROSTEPS = 22,
    MOTOR_STALLGUARD = 23,
    MOTOR_TCOOLTHRS  = 24,
    MOTOR_SGTHRS     = 25,
    MOTOR_SPREADCYCL = 26,
    MOTOR_TPWMTHRS   = 27,

    // System commands < 0
    SYSTEM_SLEEP     = -1,
    SYSTEM_RESTART   = -2,
    SYSTEM_RESET     = -3,
    SYSTEM_RENAME    = -4,

    WIRELESS_SETUP   = -51,
    WIRELESS_SSID    = -52,
    WIRELESS_PASS    = -53
};


Command hash (String command);
String hash (Command command);
std::pair<std::function<bool(int)>, String> getCommandEvalFunc(Command command);
std::pair<std::function<bool(float)>, String> getCommandEvalFuncf(Command command);
String listMotorCommands();
String listSystemCommands();
String listWirelessCommands();