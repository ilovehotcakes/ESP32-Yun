#pragma once
#include "task.h"
#include "motor_task.h"


// Commands recieved from MQTT
enum SystemCommand {
    SYS_RESET     = -98,
    SYS_REBOOT    = -99
};


class SystemTask: public Task<SystemTask> {
    friend class Task<SystemTask>;

public:
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    QueueHandle_t getSystemMessageQueue();

protected:
    void run();

private:
    QueueHandle_t system_message_queue_;

    int system_command_ = -50;
};