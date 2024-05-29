#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : 
        Task{"SystemTask", 8192, 1, task_core, 2} {
    pinMode(BUTTON_PIN, INPUT);

    // FreeRTOS implemented in C so can't use std::bind
    auto on_timer = [](TimerHandle_t timer) {
        SystemTask *temp_system_ptr = static_cast<SystemTask*>(pvTimerGetTimerID(timer));
        assert(temp_system_ptr != NULL);
        temp_system_ptr->systemSleep(timer);
    };
    system_sleep_timer_ = xTimerCreate("System_sleep_timer_", system_wake_time_, pdFALSE, this, on_timer);
    assert(system_sleep_timer_ != NULL);
}


SystemTask::~SystemTask() {
    xTimerDelete(system_sleep_timer_, portMAX_DELAY);
}


void SystemTask::run() {
    loadSettings();

    while (1) {
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("SystemTask received message: %s", inbox_.toString().c_str());
            switch (inbox_.command) {
                case SYSTEM_SLEEP:
                    // systemSleep(system_sleep_timer_);
                    LOGI("System sleep");  // TODO
                    break;
                case SYSTEM_RESET:
                    systemReset();
                    break;
                case SYSTEM_RESTART:
                    ESP.restart();
                    break;
            }
        }

        checkButtonPress();

        // if (xTimerIsTimerActive(system_sleep_timer_) == pdFALSE) {
        //     xTimerStart(system_sleep_timer_, portMAX_DELAY);
        // }

        vTaskDelay(1);  // Finished all task within loop, yielding control back to scheduler
    }
}


void SystemTask::loadSettings() {
    bool load = readFromDisk();

    serial_ = getOrDefault("serial_", serial_);
    if (serial_ == "") {  // Factory reset
        serial_ = getSerialNumber();
        settings_["serial_"] = serial_;
    }
    system_wake_time_ = getOrDefault("system_wake_time_", system_wake_time_);
    system_sleep_time_ = getOrDefault("system_sleep_time_", system_sleep_time_);

    if (!load) {
        writeToDisk();
    }

    LOGI("System settings loaded, serial#: %s", serial_.c_str());
}


inline void SystemTask::checkButtonPress() {
    // Check button presses for wireless setup mode and factory reset
    if (digitalRead(BUTTON_PIN) == LOW && !button_pressed_) {
        button_pressed_ = true;
        button_press_start_ = esp_timer_get_time();
    } else if (digitalRead(BUTTON_PIN) == HIGH && button_pressed_) {
        button_pressed_ = false;
        button_press_duration_ = (esp_timer_get_time() - button_press_start_) / 1000;  // us to ms
        if (button_press_duration_ > SETUP_MODE_TIMER && button_press_duration_ < FACTORY_RESET_TIMER) {
            LOGI("Triggered wireless setup, button pressed for %dms", button_press_duration_);
            sendTo(wireless_task_, Message(WIRELESS_SETUP, 1), 10);
        } else if (button_press_duration_ > FACTORY_RESET_TIMER) {
            LOGI("Triggered factory reset, button pressed for %dms", button_press_duration_);
            systemReset();
        } else {
            LOGI("No action, button pressed for %dms", button_press_duration_);
        }
    } else if (digitalRead(BUTTON_PIN) == LOW && button_pressed_) {
        int button_press_elapsed_ = (esp_timer_get_time() - button_press_start_) / 1000;  // us to ms
        if (button_press_elapsed_ > SETUP_MODE_TIMER && button_press_elapsed_ < FACTORY_RESET_TIMER) {
            digitalWrite(LED_PIN, HIGH);
        } else if (button_press_elapsed_ > FACTORY_RESET_TIMER) {
            digitalWrite(LED_PIN, LOW);
        }
    }
}


void SystemTask::systemSleep(TimerHandle_t timer) {
    // Standby motor driver
    sendTo(motor_task_, Message(MOTOR_STANDBY, 1), 10);
    // gpio_hold_en(STBY_PIN);
    // gpio_deep_sleep_hold_en();

    // ULP I2C
    // Sleep; TODO wait till driver is in sleep
    // ESP.deepSleep(system_sleep_time_);
}


void SystemTask::systemReset() {
    LOGI("System factory reset\n");
    WiFi.disconnect();
    LITTLEFS.format();
    ESP.restart();
}


void SystemTask::addMotorTask(Task *task) {
    motor_task_ = task;
}


void SystemTask::addWirelessTask(Task *task) {
    wireless_task_ = task;
}


TimerHandle_t SystemTask::getSystemSleepTimer() {
    return system_sleep_timer_;
}