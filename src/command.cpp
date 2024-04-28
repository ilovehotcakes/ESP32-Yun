#include "commands.h"


Command hash (String command) {
    if (command == "get position") return GET_MOTOR_POS;
    else if (command == "error") return ERROR_COMMAND;

    else if (command == "position") return MOTOR_MOVE;
    else if (command == "stop") return MOTOR_STOP;
    else if (command == "set-min") return MOTOR_SET_MIN;
    else if (command == "set-max") return MOTOR_SET_MAX;
    else if (command == "standby") return MOTOR_STNDBY;
    else if (command == "open-close") return MOTOR_OPENCLOSE;
    else if (command == "velocity") return MOTOR_SET_VELO;
    else if (command == "opening-velocity") return MOTOR_SET_OPVELO;
    else if (command == "closing-velocity") return MOTOR_SET_CLVELO;
    else if (command == "acceleration") return MOTOR_SET_ACCL;
    else if (command == "opening-acceleration") return MOTOR_SET_OPACCL;
    else if (command == "closing-acceleration") return MOTOR_SET_CLACCL;
    else if (command == "current") return MOTOR_CURRENT;
    else if (command == "opening-current") return MOTOR_OPCURRENT;
    else if (command == "closing-current") return MOTOR_CLCURRENT;
    else if (command == "direction") return MOTOR_DIRECTION;
    else if (command == "microsteps") return MOTOR_MICROSTEPS;
    else if (command == "full-steps-per-rev") return MOTOR_FULLSTEPS;
    else if (command == "stallguard-enable") return MOTOR_STALLGUARD;
    else if (command == "coolstep-threshold") return MOTOR_TCOOLTHRS;
    else if (command == "stallguard-threshold") return MOTOR_SGTHRS;
    else if (command == "fastmode-enable") return MOTOR_SPREADCYCL;
    else if (command == "fastmode-threshold") return MOTOR_TPWMTHRS;

    else if (command == "system-standby") return SYSTEM_STNDBY;
    else if (command == "system-reboot")  return SYSTEM_REBOOT;
    else if (command == "system-reset")   return SYSTEM_RESET;

    return ERROR_COMMAND;
}


String hash (Command command) {
    if (command == GET_MOTOR_POS) return "get position";
    else if (command == ERROR_COMMAND) return "error";

    else if (command == MOTOR_MOVE) return "position";
    else if (command == MOTOR_STOP) return "stop";
    else if (command == MOTOR_SET_MIN) return "set-min";
    else if (command == MOTOR_SET_MAX) return "set-max";
    else if (command == MOTOR_STNDBY) return "standby";
    else if (command == MOTOR_OPENCLOSE) return "open-close";
    else if (command == MOTOR_SET_VELO) return "velocity";
    else if (command == MOTOR_SET_OPVELO) return "opening-velocity";
    else if (command == MOTOR_SET_CLVELO) return "closing-velocity";
    else if (command == MOTOR_SET_ACCL) return "acceleration";
    else if (command == MOTOR_SET_OPACCL) return "opening-acceleration";
    else if (command == MOTOR_SET_CLACCL) return "closing-acceleration";
    else if (command == MOTOR_CURRENT) return "current";
    else if (command == MOTOR_OPCURRENT) return "opening-current";
    else if (command == MOTOR_CLCURRENT) return "closing-current";
    else if (command == MOTOR_DIRECTION) return "direction";
    else if (command == MOTOR_MICROSTEPS) return "microsteps";
    else if (command == MOTOR_FULLSTEPS) return "full-steps-per-rev";
    else if (command == MOTOR_STALLGUARD) return "stallguard-enable";
    else if (command == MOTOR_TCOOLTHRS) return "coolstep-threshold";
    else if (command == MOTOR_SGTHRS) return "stallguard-threshold";
    else if (command == MOTOR_SPREADCYCL) return "fastmode-enable";
    else if (command == MOTOR_TPWMTHRS) return "fastmode-threshold";

    else if (command == SYSTEM_STNDBY) return "system-standby";
    else if (command == SYSTEM_REBOOT)  return "system-reboot";
    else if (command == SYSTEM_RESET)   return "system-reset";

    return "error";
}


String listMotorCommands() {
    String list = "";
    for (int command = MOTOR_MOVE; command <= MOTOR_TPWMTHRS; command++) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}


String listSystemCommands() {
    String list = "";
    for (int command = SYSTEM_STNDBY; command >= SYSTEM_RESET; command--) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}