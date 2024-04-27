#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : 
        Task{"SystemTask", 8192, 1, task_core, 2} {
    // FreeRTOS implemented in C so can't use std::bind
    auto on_timer = [](TimerHandle_t timer) {
        SystemTask *temp_system_ptr = static_cast<SystemTask*>(pvTimerGetTimerID(timer));
        assert(temp_system_ptr != NULL);
        temp_system_ptr->systemStandby(timer);
    };
    system_sleep_timer_ = xTimerCreate("System_sleep_timer", system_wake_time_, pdFALSE, this, on_timer);
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
            LOGI("System task received message: %s", inbox_.toString());
            switch (inbox_.command) {
                case SYS_STANDBY:
                    systemStandby(system_sleep_timer_);
                    break;
                case SYS_RESET:
                    // motor_settings_.clear();
                    ESP.restart();
                    break;
                case SYS_REBOOT:
                    LOGD("");
                    ESP.restart();
                    break;
            }
        }

        // if (xTimerIsTimerActive(system_standby_timer_) == pdFALSE) {
        //     xTimerStart(system_sleep_timer_, portMAX_DELAY);
        // }

        vTaskDelay(1);  // Finished all task within loop, handing control back to scheduler
    }
}


void SystemTask::systemStandby(TimerHandle_t timer) {
    // Standby motor driver
    Message standby(STBY_ON);
    if (xQueueSend(motor_task_queue_, (void*) &standby, 10) != pdTRUE) {
        LOGE("Failed to send to motor_task queue_");
    }
    // gpio_hold_en(STBY_PIN);
    // gpio_deep_sleep_hold_en();

    // ULP I2C
    // Sleep; TODO wait till driver is in standby
    // ESP.deepSleep(system_sleep_time_);
}


void SystemTask::addMotorTaskQueue(QueueHandle_t queue) {
    motor_task_queue_ = queue;
}


TimerHandle_t SystemTask::getSystemSleepTimer() {
    return system_sleep_timer_;
}