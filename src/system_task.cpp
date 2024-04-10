#include "system_task.h"


SystemTask::SystemTask(const uint8_t task_core) : Task{"System", 8196, 1, task_core} {
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
        if (xQueueReceive(system_message_queue_, (void*) &command_, 0) == pdTRUE) {
            LOGI("System task received command: %d", command_);
            switch (command_) {
                case COVER_STOP:
                    motor_task_->stop();
                    break;
                case COVER_OPEN:
                    motor_task_->moveToPercent(0);
                    break;
                case COVER_CLOSE:
                    motor_task_->moveToPercent(100);
                    break;
                case COVER_SET_MAX:
                    motor_task_->setMax();
                    break;
                case COVER_SET_MIN:
                    motor_task_->setMin();
                    break;
                case SYS_RESET:
                    // motor_settings_.clear();
                    ESP.restart();
                    break;
                case SYS_REBOOT:
                    ESP.restart();
                    break;
                case STBY_ON:
                    motor_task_->driverStartup();
                    break;
                case STBY_OFF:
                    digitalWrite(STBY_PIN, HIGH);
                    break;
                default:
                    motor_task_->moveToPercent(command_);
                    break;
            }
        }
    }
}


QueueHandle_t SystemTask::getSystemMessageQueue() {
    return system_message_queue_;
}