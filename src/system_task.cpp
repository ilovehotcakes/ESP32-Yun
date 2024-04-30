#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : 
        Task{"SystemTask", 8192, 1, task_core} {
    queue_ = xQueueCreate(2, sizeof(Message));
    assert(queue_ != NULL && "Failed to create SystemTask queue_");

    // FreeRTOS implemented in C so can't use std::bind
    auto on_timer = [](TimerHandle_t timer) {
        SystemTask *temp_system_ptr = static_cast<SystemTask*>(pvTimerGetTimerID(timer));
        assert(temp_system_ptr != NULL);
        temp_system_ptr->systemSleep(timer);
    };
    system_sleep_timer_ = xTimerCreate("System_sleep_timer_", system_wake_time_, pdFALSE, this, on_timer);
    assert(system_sleep_timer_ != NULL && "Failed to create system_sleep_timer_");
}


SystemTask::~SystemTask() {
    xTimerDelete(system_sleep_timer_, portMAX_DELAY);
}


void SystemTask::run() {
    pinMode(BUTTON_PIN, INPUT);

    long xStart = xTaskGetTickCount();

    // vTaskDelay(500);

    // If button is not pressed for within the first few seconds, don't enter setup mode
    // Todo: if not first boot, skip this part (for deep sleep)
    while (digitalRead(BUTTON_PIN) == LOW)

    if (xTaskGetTickCount() - xStart > 5000) {
        Serial.println("......triggered setup function.......");
    }

    if (xTaskGetTickCount() - xStart > 20000) {
        Serial.println("......triggered hard reset.......");
    }

    while (1) {
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("SystemTask received message: %s", inbox_.toString().c_str());
            switch (inbox_.command) {
                case SYSTEM_SLEEP:
                    // systemSleep(system_sleep_timer_);
                    Serial.println("System sleep");
                    break;
                case SYSTEM_RESET:
                    LOGI("System factory reset")
                    motor_task_->settings_.clear();
                    // wireless_task_->settings_.clear();
                    // settings_.clear();
                    ESP.restart();
                    break;
                case SYSTEM_REBOOT:
                    LOGD("");
                    ESP.restart();
                    break;
            }
        }

        // if (xTimerIsTimerActive(system_sleep_timer_) == pdFALSE) {
        //     xTimerStart(system_sleep_timer_, portMAX_DELAY);
        // }

        vTaskDelay(1);  // Finished all task within loop, yielding control back to scheduler
    }
}


void SystemTask::systemSleep(TimerHandle_t timer) {
    // Standby motor driver
    sendTo(motor_task_, Message(MOTOR_STNDBY, 1), 10);
    // gpio_hold_en(STBY_PIN);
    // gpio_deep_sleep_hold_en();

    // ULP I2C
    // Sleep; TODO wait till driver is in sleep
    // ESP.deepSleep(system_sleep_time_);
}


void SystemTask::addMotorTask(void *task) {
    motor_task_ = static_cast<Task*>(task);
}


TimerHandle_t SystemTask::getSystemSleepTimer() {
    return system_sleep_timer_;
}