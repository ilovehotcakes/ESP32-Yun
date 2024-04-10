#pragma once
#include "task.h"
#include "motor_task.h"


// Commands recieved from MQTT
enum Command {
    COVER_STOP    = -1,
    COVER_OPEN    = -2,
    COVER_CLOSE   = -3,
    COVER_SET_MIN = -4,
    COVER_SET_MAX = -5,
    STBY_ON       = -6,
    STBY_OFF      = -7,
    SYS_RESET     = -98,
    SYS_REBOOT    = -99
};


class SystemTask: public Task<SystemTask> {
    friend class Task<SystemTask>;

public:
    TaskHandle_t wireless_task_;
    MotorTask *motor_task_;
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    QueueHandle_t getSystemMessageQueue();

protected:
    void run();

private:
    QueueHandle_t system_message_queue_;

    int command_ = -50;
};