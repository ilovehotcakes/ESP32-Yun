#pragma once
#include "task.h"
#include "logger.h"
#include "commands.h"


class SystemTask: public Task<SystemTask> {
    friend class Task<SystemTask>;

public:
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    void addMotorTaskQueue(QueueHandle_t queue);
    void addMotorRunningSemaphore(SemaphoreHandle_t semaphore);

protected:
    void run();

private:
    QueueHandle_t motor_task_queue_;
    TimerHandle_t system_standby_timer_;
    SemaphoreHandle_t motor_running_semaphore_;

    void systemStandby(TimerHandle_t timer);

    int system_command_ = -50;
    int system_wake_time_ = 5000;      // mSec
    int system_sleep_time_ = 5000000;  // uSec
    bool sleep_enabled_ = false;
};