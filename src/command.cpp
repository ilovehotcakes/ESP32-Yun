#include "command.h"


Command hash (String command) {
    if (command == "get-position") return GET_MOTOR_POS;
    else if (command == "error") return ERROR_COMMAND;

    else if (command == "percent") return MOTOR_PERECENT;
    else if (command == "stop") return MOTOR_STOP;
    else if (command == "set-min") return MOTOR_SET_MIN;
    else if (command == "set-max") return MOTOR_SET_MAX;
    else if (command == "standby") return MOTOR_STDBY;
    else if (command == "open-close") return MOTOR_OPENCLOSE;
    else if (command == "velocity") return MOTOR_VELOCITY;
    else if (command == "opening-velocity") return MOTOR_OPVELOCITY;
    else if (command == "closing-velocity") return MOTOR_CLVELOCITY;
    else if (command == "acceleration") return MOTOR_ACCEL;
    else if (command == "opening-acceleration") return MOTOR_OPACCEL;
    else if (command == "closing-acceleration") return MOTOR_CLACCEL;
    else if (command == "current") return MOTOR_CURRENT;
    else if (command == "opening-current") return MOTOR_OPCURRENT;
    else if (command == "closing-current") return MOTOR_CLCURRENT;
    else if (command == "direction") return MOTOR_DIRECTION;
    else if (command == "microsteps") return MOTOR_MICROSTEPS;
    else if (command == "full-steps-per-rev") return MOTOR_FULLSTEPS;
    else if (command == "stallguard") return MOTOR_STALLGUARD;
    else if (command == "coolstep-threshold") return MOTOR_TCOOLTHRS;
    else if (command == "stallguard-threshold") return MOTOR_SGTHRS;
    else if (command == "fastmode") return MOTOR_SPREADCYCL;
    else if (command == "fastmode-threshold") return MOTOR_TPWMTHRS;

    else if (command == "sleep") return SYSTEM_SLEEP;
    else if (command == "reboot") return SYSTEM_REBOOT;
    else if (command == "reset") return SYSTEM_RESET;

    return ERROR_COMMAND;
}


String hash (Command command) {
    if (command == GET_MOTOR_POS) return "get-position";
    else if (command == ERROR_COMMAND) return "error";

    else if (command == MOTOR_PERECENT) return "percent";
    else if (command == MOTOR_STOP) return "stop";
    else if (command == MOTOR_SET_MIN) return "set-min";
    else if (command == MOTOR_SET_MAX) return "set-max";
    else if (command == MOTOR_STDBY) return "standby";
    else if (command == MOTOR_OPENCLOSE) return "open-close";
    else if (command == MOTOR_VELOCITY) return "velocity";
    else if (command == MOTOR_OPVELOCITY) return "opening-velocity";
    else if (command == MOTOR_CLVELOCITY) return "closing-velocity";
    else if (command == MOTOR_ACCEL) return "acceleration";
    else if (command == MOTOR_OPACCEL) return "opening-acceleration";
    else if (command == MOTOR_CLACCEL) return "closing-acceleration";
    else if (command == MOTOR_CURRENT) return "current";
    else if (command == MOTOR_OPCURRENT) return "opening-current";
    else if (command == MOTOR_CLCURRENT) return "closing-current";
    else if (command == MOTOR_DIRECTION) return "direction";
    else if (command == MOTOR_MICROSTEPS) return "microsteps";
    else if (command == MOTOR_FULLSTEPS) return "full-steps-per-rev";
    else if (command == MOTOR_STALLGUARD) return "stallguard";
    else if (command == MOTOR_TCOOLTHRS) return "coolstep-threshold";
    else if (command == MOTOR_SGTHRS) return "stallguard-threshold";
    else if (command == MOTOR_SPREADCYCL) return "fastmode";
    else if (command == MOTOR_TPWMTHRS) return "fastmode-threshold";

    else if (command == SYSTEM_SLEEP) return "sleep";
    else if (command == SYSTEM_REBOOT) return "reboot";
    else if (command == SYSTEM_RESET) return "reset";

    return "error";
}


String listMotorCommands() {
    String list = "";
    for (int command = MOTOR_PERECENT; command <= MOTOR_TPWMTHRS; command++) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}


String listSystemCommands() {
    String list = "";
    for (int command = SYSTEM_SLEEP; command >= SYSTEM_RESET; command--) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}