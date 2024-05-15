#pragma once
#include "task.h"
#include "wireless_task.h"


class SystemTask: public Task {
public:
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    void addMotorTask(void *task);
    TimerHandle_t getSystemSleepTimer();

protected:
    void run();

private:
    String serial_ = "";
    bool factory_reset_ = true;
    int system_wake_time_ = 5000;      // mSec
    int system_sleep_time_ = 5000000;  // uSec

    int button_press_start_ = 0;
    int button_press_duration_ = 0;
    bool button_pressed_ = false;

    Task *motor_task_;
    TimerHandle_t system_sleep_timer_;

    void loadSettings();
    void systemSleep(TimerHandle_t timer);
    void systemReset();
};