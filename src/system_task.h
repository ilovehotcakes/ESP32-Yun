#pragma once
#include "task.h"


class SystemTask: public Task {
public:
    SystemTask(const uint8_t task_core);
    ~SystemTask();
    void addMotorTask(Task *task);
    void addWirelessTask(Task *task);
    TimerHandle_t getSystemSleepTimer();

protected:
    void run();

private:
    String serial_ = "";
    int system_wake_time_ = 5000;      // mSec
    int system_sleep_time_ = 5000000;  // uSec

    int button_press_start_ = 0;
    int button_press_duration_ = 0;
    bool button_pressed_ = false;

    Task *motor_task_;
    Task *wireless_task_;
    TimerHandle_t system_sleep_timer_;

    void loadSettings();
    inline void checkButtonPress();
    void systemSleep(TimerHandle_t timer);
    void systemReset();
};