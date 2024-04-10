#pragma once
#include "task.h"

class LedTask: public Task<LedTask> {
    friend class Task<LedTask>;

public:
    LedTask();
    ~LedTask();
    void turnOn();
    void turnOff();

protected:
    void run();

private:
};