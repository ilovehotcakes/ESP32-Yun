#pragma once
#include "task.h"

class LedTask: public Task {
public:
    LedTask();
    ~LedTask();
    void turnOn();
    void turnOff();

protected:
    void run();

private:
};