#include "command.h"


Command hash(String command) {
    if (command == "update-position") return UPDATE_POSITION;
    else if (command == "error") return ERROR_COMMAND;

    else if (command == "stop") return MOTOR_STOP;
    else if (command == "percent") return MOTOR_PERECENT;
    else if (command == "step") return MOTOR_STEP;
    else if (command == "forward") return MOTOR_FORWARD;
    else if (command == "backward") return MOTOR_BACKWARD;
    else if (command == "set-min") return MOTOR_SET_MIN;
    else if (command == "set-max") return MOTOR_SET_MAX;
    else if (command == "zero") return MOTOR_ZERO;
    else if (command == "standby") return MOTOR_STANDBY;
    else if (command == "sync-settings") return MOTOR_SYNC_STTNG;
    else if (command == "velocity") return MOTOR_VLCTY;
    else if (command == "opening-velocity") return MOTOR_OP_VLCTY;
    else if (command == "closing-velocity") return MOTOR_CL_VLCTY;
    else if (command == "acceleration") return MOTOR_ACCEL;
    else if (command == "opening-acceleration") return MOTOR_OP_ACCEL;
    else if (command == "closing-acceleration") return MOTOR_CL_ACCEL;
    else if (command == "current") return MOTOR_CURRENT;
    else if (command == "opening-current") return MOTOR_OP_CURRENT;
    else if (command == "closing-current") return MOTOR_CL_CURRENT;
    else if (command == "direction") return MOTOR_DIRECTION;
    else if (command == "full-steps") return MOTOR_FULL_STEPS;
    else if (command == "microsteps") return MOTOR_MICROSTEPS;
    else if (command == "stallguard") return MOTOR_STALLGUARD;
    else if (command == "coolstep-threshold") return MOTOR_TCOOLTHRS;
    else if (command == "stallguard-threshold") return MOTOR_SGTHRS;
    else if (command == "fastmode") return MOTOR_SPREADCYCL;
    else if (command == "fastmode-threshold") return MOTOR_TPWMTHRS;

    else if (command == "sleep") return SYSTEM_SLEEP;
    else if (command == "restart") return SYSTEM_RESTART;
    else if (command == "reset") return SYSTEM_RESET;
    else if (command == "name") return SYSTEM_RENAME;

    else if (command == "setup") return WIRELESS_SETUP;
    else if (command == "ssid") return WIRELESS_SSID;
    else if (command == "password") return WIRELESS_PASS;

    return ERROR_COMMAND;
}


String hash(Command command) {
    if (command == UPDATE_POSITION) return "update-position";
    else if (command == ERROR_COMMAND) return "error";

    else if (command == MOTOR_STOP) return "stop";
    else if (command == MOTOR_PERECENT) return "percent";
    else if (command == MOTOR_STEP) return "step";
    else if (command == MOTOR_FORWARD) return "forward";
    else if (command == MOTOR_BACKWARD) return "backward";
    else if (command == MOTOR_SET_MIN) return "set-min";
    else if (command == MOTOR_SET_MAX) return "set-max";
    else if (command == MOTOR_ZERO) return "zero";
    else if (command == MOTOR_STANDBY) return "standby";
    else if (command == MOTOR_SYNC_STTNG) return "sync-settings";
    else if (command == MOTOR_VLCTY) return "velocity";
    else if (command == MOTOR_OP_VLCTY) return "opening-velocity";
    else if (command == MOTOR_CL_VLCTY) return "closing-velocity";
    else if (command == MOTOR_ACCEL) return "acceleration";
    else if (command == MOTOR_OP_ACCEL) return "opening-acceleration";
    else if (command == MOTOR_CL_ACCEL) return "closing-acceleration";
    else if (command == MOTOR_CURRENT) return "current";
    else if (command == MOTOR_OP_CURRENT) return "opening-current";
    else if (command == MOTOR_CL_CURRENT) return "closing-current";
    else if (command == MOTOR_DIRECTION) return "direction";
    else if (command == MOTOR_FULL_STEPS) return "full-steps";
    else if (command == MOTOR_MICROSTEPS) return "microsteps";
    else if (command == MOTOR_STALLGUARD) return "stallguard";
    else if (command == MOTOR_TCOOLTHRS) return "coolstep-threshold";
    else if (command == MOTOR_SGTHRS) return "stallguard-threshold";
    else if (command == MOTOR_SPREADCYCL) return "fastmode";
    else if (command == MOTOR_TPWMTHRS) return "fastmode-threshold";

    else if (command == SYSTEM_SLEEP) return "sleep";
    else if (command == SYSTEM_RESTART) return "restart";
    else if (command == SYSTEM_RESET) return "reset";
    else if (command == SYSTEM_RENAME) return "name";

    else if (command == WIRELESS_SETUP) return "setup";
    else if (command == WIRELESS_SSID) return "ssid";
    else if (command == WIRELESS_PASS) return "password";

    return "error";
}


std::pair<std::function<bool(int)>, String> getCommandEvalFunc(Command command) {
    if (command == MOTOR_STOP) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_PERECENT) {
        return std::make_pair([=](int val) -> bool { return val > 100 || val < 0; }, "=0~100 (%); 0 to open; 100 to close");
    } else if (command == MOTOR_STEP) {
        return std::make_pair([=](int val) -> bool { return val < 0; }, ">=0");
    } else if (command == MOTOR_FORWARD) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_BACKWARD) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_SET_MIN) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_SET_MAX) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_ZERO) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == MOTOR_STANDBY) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1; 1 to standby motor driver; 0 to start");
    } else if (command == MOTOR_SYNC_STTNG) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1; 0 to keep opening/closing settings the same");
    } else if (command == MOTOR_CURRENT) {
        return std::make_pair([=](int val) -> bool { return val < 1 || val > 2000; }, "=1~2000 (mA); please refer to motor datasheet for max RMS");
    } else if (command == MOTOR_OP_CURRENT) {
        return std::make_pair([=](int val) -> bool { return val < 1 || val > 2000; }, "=1~2000 (mA); please refer to motor datasheet for max RMS");
    } else if (command == MOTOR_CL_CURRENT) {
        return std::make_pair([=](int val) -> bool { return val < 1 || val > 2000; }, "=1~2000 (mA); please refer to motor datasheet for max RMS");
    } else if (command == MOTOR_DIRECTION) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1");
    } else if (command == MOTOR_FULL_STEPS) {
        return std::make_pair([=](int val) -> bool { return val <= 0; }, ">0");
    } else if (command == MOTOR_MICROSTEPS) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 2 && val != 4 && val != 8 && val != 16 && val != 32 && val != 64
                                                                && val != 128 && val != 256; }, "=0 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256");
    } else if (command == MOTOR_STALLGUARD) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1; 0 to disable; 1 to enable");
    } else if (command == MOTOR_TCOOLTHRS) {
        return std::make_pair([=](int val) -> bool { return val < 0 || val > 1048575; }, "=0~1048575; lower threshold velocity for switching on stallguard");
    } else if (command == MOTOR_SGTHRS) {
        return std::make_pair([=](int val) -> bool { return val < 0 || val > 255; }, "=0~255; the greater, the easier to stall");
    } else if (command == MOTOR_SPREADCYCL) {
        return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1; 0 to disable; 1 to enable");
    } else if (command == MOTOR_TPWMTHRS) {
        return std::make_pair([=](int val) -> bool { return val < 0 || val > 1048575; }, "=0~1048575; upper threshold to switch to fastmode");
    }

    else if (command == SYSTEM_SLEEP) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == SYSTEM_RESTART) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    } else if (command == SYSTEM_RESET) {
        return std::make_pair([=](int val) -> bool { return false; }, "");
    }

    // else if (command == WIRELESS_SETUP) {
    return std::make_pair([=](int val) -> bool { return val != 0 && val != 1; }, "=0 | 1; 1 to enter setup mode");
}


std::pair<std::function<bool(float)>, String> getCommandEvalFuncf(Command command) {
    if (command == MOTOR_VLCTY) {
        return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0 (Hz)");  // float
    } else if (command == MOTOR_OP_VLCTY) {
        return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0 (Hz)");  // float
    } else if (command == MOTOR_CL_VLCTY) {
        return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0 (Hz)");  // float
    } else if (command == MOTOR_ACCEL) {
        return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0" );  // float
    } else if (command == MOTOR_OP_ACCEL) {
        return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0" );  // float
    } 
    // else if (command == MOTOR_CL_ACCEL) {
    return std::make_pair([=](float val) -> bool { return val <= 0.0; }, ">0.0" );  // float
}


String listMotorCommands() {
    String list = "";
    for (int command = MOTOR_STOP; command <= MOTOR_TPWMTHRS; command++) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}


String listSystemCommands() {
    String list = "";
    for (int command = SYSTEM_SLEEP; command >= SYSTEM_RENAME; command--) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}


String listWirelessCommands() {
    String list = "";
    for (int command = WIRELESS_SETUP; command >= WIRELESS_PASS; command--) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}