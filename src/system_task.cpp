#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : Task{"System", 8192, 1, task_core, 10} {
    // FreeRTOS implemented in C so can't use std::bind
    auto on_timer = [](TimerHandle_t timer) {
        SystemTask *temp_system_ptr = static_cast<SystemTask*>(pvTimerGetTimerID(timer));
        assert(temp_system_ptr != NULL);
        temp_system_ptr->systemStandby(timer);
    };
    system_standby_timer_ = xTimerCreate("Motor standby", system_wake_time_, pdFALSE, this, on_timer);
    assert(system_standby_timer_ != NULL && "Failed to create system_standby_timer_.");
}


SystemTask::~SystemTask() {
    xTimerDelete(system_standby_timer_, portMAX_DELAY);
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
        if (xQueueReceive(queue_, (void*) &system_command_, 0) == pdTRUE) {
            LOGI("System task received command: %d", system_command_);
            switch (system_command_) {
                case SYS_STANDBY:
                    systemStandby(system_standby_timer_);
                    break;
                case SYS_RESET:
                    // motor_settings_.clear();
                    ESP.restart();
                    break;
                case SYS_REBOOT:
                    ESP.restart();
                    LOGD("");
                    break;
            }
        }

        if (uxSemaphoreGetCount(motor_running_semaphore_) == 1) {
            // || xTimerIsTimerActive(system_standby_timer_) == pdFALSE) {
            xTimerStart(system_standby_timer_, portMAX_DELAY);
        }
    }
}


void SystemTask::systemStandby(TimerHandle_t timer) {
    // Standby motor driver
    int standby_driver = STBY_ON;
    if (xQueueSend(motor_task_queue_, (void*) &standby_driver, 10) != pdTRUE) {
        LOGE("Failed to send to motor_message_queue_");
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


void SystemTask::addMotorRunningSemaphore(SemaphoreHandle_t semaphore) {
    motor_running_semaphore_ = semaphore;
}