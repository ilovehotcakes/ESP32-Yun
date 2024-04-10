#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : Task{"System", 8192, 1, task_core} {
    system_message_queue_ = xQueueCreate(10, sizeof(int));
    assert(system_message_queue_ != NULL);
}


SystemTask::~SystemTask() {
    vQueueDelete(system_message_queue_);
}


void SystemTask::run() {
    pinMode(BUTTON_PIN, INPUT);

    // If button is not pressed for within the first few seconds, don't enter setup mode
    // todo..

    while(1) {
        if (xQueueReceive(system_message_queue_, (void*) &system_command_, 0) == pdTRUE) {
            LOGI("System task received command: %d", system_command_);
            switch (system_command_) {
                case SYS_RESET:
                    // motor_settings_.clear();
                    ESP.restart();
                    break;
                case SYS_REBOOT:
                    ESP.restart();
                    break;
            }
        }
    }
}


QueueHandle_t SystemTask::getSystemMessageQueue() {
    return system_message_queue_;
}