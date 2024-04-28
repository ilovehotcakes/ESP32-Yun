#pragma once
/**
    Copyright 2020 Scott Bezek and the splitflap contributors

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    Modified by Jason Chen, 2024
**/
#include <Arduino.h>
#include "command.h"
#include "logger.h"


struct Message {
    Message(Command command, int parameter = INT_MIN) : command(command), parameter(parameter) {}
    String toString() { return "command=" + hash(command) 
                        + (parameter == INT_MIN ? "" : ", parameter=" + String(parameter)); }
    Command command;
    int parameter;
};


// Static polymorphic abstract base class for a FreeRTOS task using CRTP pattern. Concrete
// implementations should implement a run() method.
// Inspired by https://fjrg76.wordpress.com/2018/05/23/objectifying-task-creation-in-freertos-ii/
template<class T>
class Task {
public:
    Task(const char* const name, uint32_t stack_depth, UBaseType_t priority,
         const BaseType_t core_id = tskNO_AFFINITY, int queue_length = 1) :
            name_ {name},
            inbox_(ERROR_COMMAND, INT_MIN),
            stack_depth_ {stack_depth},
            priority_ {priority},
            core_id_ {core_id} {
        queue_ = xQueueCreate(queue_length, sizeof(Message));
        assert(queue_ != NULL && "Failed to create task_queue_");
    }

    ~Task() {
        vQueueDelete(queue_);
    }

    TaskHandle_t getTaskHandle() {
        return task_handle_;
    }

    QueueHandle_t getQueueHandle() {
        return queue_;
    }

    bool sendTo(Task *task, Message message, int timeout) {
        LOGI("Sending message from %s to %s", name_, task->name_);
        if (xQueueSend(task->getQueueHandle(), (void*) &message, timeout) != pdTRUE) {
            LOGE("Failed to send message from %s to %s", name_, task->name_);
            return false;
        }
        return true;
    }

    void init() {
        BaseType_t result = xTaskCreatePinnedToCore(taskFunction, name_, stack_depth_, this, priority_, &task_handle_, core_id_);
        assert(result == pdPASS && "Failed to create task");
    }

protected:
    const char* const name_;
    QueueHandle_t queue_;
    Message inbox_;

private:
    static void taskFunction(void* params) {
        T* t = static_cast<T*>(params);
        t->run();
    }

    uint32_t stack_depth_;
    UBaseType_t priority_;
    TaskHandle_t task_handle_;
    const BaseType_t core_id_;
};