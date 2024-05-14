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
    else if (command == "standby") return MOTOR_STDBY;
    else if (command == "open-close") return MOTOR_OPEN_CLOSE;
    else if (command == "speed") return MOTOR_SPEED;
    else if (command == "opening-speed") return MOTOR_OP_SPEED;
    else if (command == "closing-speed") return MOTOR_CL_SPEED;
    else if (command == "acceleration") return MOTOR_ACCEL;
    else if (command == "opening-acceleration") return MOTOR_OP_ACCEL;
    else if (command == "closing-acceleration") return MOTOR_CL_ACCEL;
    else if (command == "current") return MOTOR_CURRENT;
    else if (command == "opening-current") return MOTOR_OP_CURRENT;
    else if (command == "closing-current") return MOTOR_CL_CURRENT;
    else if (command == "direction") return MOTOR_DIRECTION;
    else if (command == "microsteps") return MOTOR_MICROSTEPS;
    else if (command == "full-steps") return MOTOR_FULL_STEPS;
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
    else if (command == MOTOR_STDBY) return "standby";
    else if (command == MOTOR_OPEN_CLOSE) return "open-close";
    else if (command == MOTOR_SPEED) return "speed";
    else if (command == MOTOR_OP_SPEED) return "opening-speed";
    else if (command == MOTOR_CL_SPEED) return "closing-speed";
    else if (command == MOTOR_ACCEL) return "acceleration";
    else if (command == MOTOR_OP_ACCEL) return "opening-acceleration";
    else if (command == MOTOR_CL_ACCEL) return "closing-acceleration";
    else if (command == MOTOR_CURRENT) return "current";
    else if (command == MOTOR_OP_CURRENT) return "opening-current";
    else if (command == MOTOR_CL_CURRENT) return "closing-current";
    else if (command == MOTOR_DIRECTION) return "direction";
    else if (command == MOTOR_MICROSTEPS) return "microsteps";
    else if (command == MOTOR_FULL_STEPS) return "full-steps";
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
    for (int command = MOTOR_STOP; command <= MOTOR_TPWMTHRS; command++) {
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