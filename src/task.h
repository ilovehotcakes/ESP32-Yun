#pragma once
/**
    task.h - A base wrapper class to instantiate FreeRTOS tasks with extra shared functions.
    Author: Jason Chen, 2024

    Inspired by SmartKnob by Scott Bezek and
    https://fjrg76.wordpress.com/2018/05/20/objectifying-task-creation-in-freertos/
**/
#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LITTLEFS.h>
#include "logger.h"
#include "command.h"


struct Message {
    Message(Command command, int parameter) : command(command), parameter(parameter) {}
    Message(Command command, float parameterf) : command(command), parameterf(parameterf) {}
    String toString() { return "command=" + hash(command) + ", parameter="
                               + (parameter != INT_MIN ? String(parameter) : String(parameterf)); }
    Command command;
    int parameter = INT_MIN;
    float parameterf = 0.0;
};


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
        assert(queue_ != NULL);
    }

    ~Task() {
        vQueueDelete(queue_);
    }

    void init() {
        BaseType_t result = xTaskCreatePinnedToCore(taskFunction, name_, stack_depth_,
                                                    this, priority_, &task_handle_, core_id_);
        assert(result == pdPASS);
    }

    TaskHandle_t getTaskHandle() {
        return task_handle_;
    }

    QueueHandle_t getQueueHandle() {
        return queue_;
    }

    JsonDocument getSettings() {
        return settings_;
    }

protected:
    const char* name_;
    Message inbox_;
    QueueHandle_t queue_;
    JsonDocument settings_;

    bool sendTo(Task *task, Message message, int timeout) {
        LOGI("Sending message from %s to %s", name_, task->name_);
        if (xQueueSend(task->getQueueHandle(), (void*) &message, timeout) != pdTRUE) {
            LOGE("Failed to send message from %s to %s", name_, task->name_);
            return false;
        }
        return true;
    }

    template<typename t>
    void setAndSave(t &setting, t value, const char *key) {
        setting = value;
        settings_[key] = value;
        writeToDisk();
    }

    template<typename t>
    t getOrDefault(const char *key, t default_value) {
        if (!settings_.containsKey(key)) {
            settings_[key] = default_value;
        }
        return settings_[key];
    }

    bool writeToDisk() {
        String path = String("/") + name_ + ".txt";
        LOGI("%s writing file to %s", name_, path.c_str());
        File file = LITTLEFS.open(path, FILE_WRITE);
        if (!file) {
            LOGI("%s failed to open file for writing", name_);
            return false;
        }
        if (serializeJson(settings_, file)) {
            LOGI("%s successfully written to file", name_);
        } else {
            LOGI("%s failed to write to file", name_);
        }
        file.close();
        return true;
    }

    bool readFromDisk() {
        String path = String("/") + name_ + ".txt";
        LOGI("%s reading file from %s", name_, path.c_str());
        File file = LITTLEFS.open(path, FILE_READ);
        if (!file) {
            LOGI("%s failed to open file for reading", name_);
            return false;
        }
        while (file.available()) {
            deserializeJson(settings_, file);
        }
        file.close();
        return true;
    }

    String getSerialNumber() {
        uint8_t mac_address[6];
        esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
        String serial = "";
        for (int i = 0; i < 6; i++) {
            char hexidecimal[2];
            sprintf(hexidecimal, "%02X", mac_address[i]);
            serial += hexidecimal;
        }
        serial.toLowerCase();
        return serial;
    }

    virtual void run() = 0;

private:
    uint32_t stack_depth_;
    UBaseType_t priority_;
    TaskHandle_t task_handle_;
    const BaseType_t core_id_;

    static void taskFunction(void* params) {
        Task *t = static_cast<Task*>(params);
        t->run();
    }
};