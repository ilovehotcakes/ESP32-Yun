#pragma once
#include "task.h"

#define SYSTEM_WAKE_DURATION 5000   // ms
#define SYSTEM_SLEEP_DURTION 5000   // ms
#define SETUP_MODE_TIMER     5000   // ms
#define FACTORY_RESET_TIMER  15000  // ms


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
    int system_wake_time_  = SYSTEM_WAKE_DURATION;          // ms
    int system_sleep_time_ = SYSTEM_SLEEP_DURTION * 1000;  // us

    bool button_pressed_        = false;
    int  button_press_start_    = 0;
    int  button_press_duration_ = 0;

    Task *motor_task_;
    Task *wireless_task_;
    TimerHandle_t system_sleep_timer_;

    void loadSettings();
    inline void checkButtonPress();
    void systemSleep(TimerHandle_t timer);
    void systemReset();
};