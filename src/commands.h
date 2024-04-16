#pragma once

enum MotorCommand {
    COVER_STOP    = -1,
    COVER_SET_MIN = -2,
    COVER_SET_MAX = -3,
    STBY_ON       = -4,
    STBY_OFF      = -5
};

enum SystemCommand {
    SYS_STANDBY = -97,
    SYS_RESET   = -98,
    SYS_REBOOT  = -99
};