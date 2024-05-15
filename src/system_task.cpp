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
    pinMode(LED_PIN, OUTPUT);

    if (factory_reset_) {
        factory_reset_ = false;
        uint8_t mac_address[6];
        esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
        for (int i = 0; i < 6; i++) {
            char temp[2];
            sprintf(temp, "%02X", mac_address[i]);
            serial_ += temp;
        }
        serial_.toLowerCase();
    }

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
                case SYSTEM_REBOOT:
                    ESP.restart();
                    break;
            }
        }

        // Check button presses for wireless setup mode and factory reset
        if (digitalRead(BUTTON_PIN) == LOW && !button_pressed_) {
            button_press_start_ = xTaskGetTickCount();
            button_pressed_ = true;
        } else if (digitalRead(BUTTON_PIN) == HIGH && button_pressed_) {
            button_press_duration_ = xTaskGetTickCount() - button_press_start_;
            if (button_press_duration_ > 5000 && button_press_duration_ < 10000) {
                LOGI("Triggered wireless setup, button pressed for %dms", button_press_duration_);
                setAndSave(setup_mode_, true, "setup_mode_");
                // ESP.restart();
            } else if (button_press_duration_ > 10000) {
                LOGI("Triggered factory reset, button pressed for %dms", button_press_duration_);
                systemReset();
            }
            button_pressed_ = false;
        }

        // if (xTimerIsTimerActive(system_sleep_timer_) == pdFALSE) {
        //     xTimerStart(system_sleep_timer_, portMAX_DELAY);
        // }

        vTaskDelay(1);  // Finished all task within loop, yielding control back to scheduler
    }
}


void SystemTask::loadSettings() {
    bool load = readFromDisk();

    serial_ = getOrDefault(serial_, "serial_");
    factory_reset_ = getOrDefault(factory_reset_, "factory_reset_");
    setup_mode_ = getOrDefault(setup_mode_, "setup_mode_");
    system_wake_time_ = getOrDefault(system_wake_time_, "system_wake_time_");
    system_sleep_time_ = getOrDefault(system_sleep_time_, "system_sleep_time_");

    if (!load) {
        writeToDisk();
    }

    LOGI("System settings loaded, serial#: %s", serial_.c_str());
}


void SystemTask::systemSleep(TimerHandle_t timer) {
    // Standby motor driver
    sendTo(motor_task_, Message(MOTOR_STDBY, 1), 10);
    // gpio_hold_en(STBY_PIN);
    // gpio_deep_sleep_hold_en();

    // ULP I2C
    // Sleep; TODO wait till driver is in sleep
    // ESP.deepSleep(system_sleep_time_);
}


void SystemTask::systemReset() {
    LOGI("System factory reset\n");
    factory_reset_ = true;
    LITTLEFS.format();
    ESP.restart();
}


void SystemTask::addMotorTask(void *task) {
    motor_task_ = static_cast<Task*>(task);
}


TimerHandle_t SystemTask::getSystemSleepTimer() {
    return system_sleep_timer_;
}