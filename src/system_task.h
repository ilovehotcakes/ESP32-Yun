#pragma once
#include "task.h"
#include "logger.h"
#include "commands.h"


class SystemTask: public Task<SystemTask> {
    friend class Task<SystemTask>;

public:
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    void addMotorTask(void *task);
    TimerHandle_t getSystemSleepTimer();

protected:
    void run();

private:
    Task *motor_task_;
    TimerHandle_t system_sleep_timer_;

    void systemStandby(TimerHandle_t timer);

    int system_wake_time_ = 5000;      // mSec
    int system_sleep_time_ = 5000000;  // uSec
    bool sleep_enabled_ = false;
};