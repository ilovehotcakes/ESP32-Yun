#include "commands.h"


Command hash (String command) {
    if (command == "position") return MOTOR_MOVE;
    else if (command == "stop") return MOTOR_STOP;
    else if (command == "min") return MOTOR_SET_MIN;
    else if (command == "max") return MOTOR_SET_MAX;
    else if (command == "standby") return MOTOR_STNDBY;
    else if (command == "direction") return MOTOR_SET_DIR;
    else if (command == "microsteps") return MOTOR_SET_MICSTP;
    else if (command == "full-steps-per-rev") return MOTOR_SET_FULSTP;
    else if (command == "velocity") return MOTOR_SET_VELOC;
    else if (command == "accerleration") return MOTOR_SET_ACCEL;
    else if (command == "opening-current") return MOTOR_SET_OPCUR;
    else if (command == "closing-current") return MOTOR_SET_CLCUR;
    else if (command == "stallguard-enable") return MOTOR_ENABLE_SG;
    else if (command == "stallguard-threshold") return MOTOR_SET_SGTHR;
    return ERROR_COMMAND;
}


String hash (Command command) {
    if (command == MOTOR_MOVE) return "position";
    else if (command == MOTOR_STOP) return "stop";
    else if (command == MOTOR_SET_MIN) return "min";
    else if (command == MOTOR_SET_MAX) return "max";
    else if (command == MOTOR_STNDBY) return "standby";
    else if (command == MOTOR_SET_DIR) return "direction";
    else if (command == MOTOR_SET_MICSTP) return "microsteps";
    else if (command == MOTOR_SET_FULSTP) return "full-steps-per-rev";
    else if (command == MOTOR_SET_VELOC) return "velocity";
    else if (command == MOTOR_SET_ACCEL) return "accerleration";
    else if (command == MOTOR_SET_OPCUR) return "opening-current";
    else if (command == MOTOR_SET_CLCUR) return "closing-current";
    else if (command == MOTOR_ENABLE_SG) return "stallguard-enable";
    else if (command == MOTOR_SET_SGTHR) return "stallguard-threshold";
    return "";
}


String listMotorCommands() {
    String list = "";
    for (int command = MOTOR_MOVE; command <= MOTOR_SET_SGTHR; command++) {
        list = list + hash(Command(command)) + " | ";
    }
    return list.substring(0, list.length() - 3);
}