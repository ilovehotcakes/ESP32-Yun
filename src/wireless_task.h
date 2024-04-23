#pragma once
/**
    wireless_task.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2023

    WirelessTask establishes and maintains WiFi connection and MQTT connection.
**/
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include <esp_task_wdt.h>
#include "task.h"
#include "logger.h"
#include "commands.h"
#include "secrets.h"
#include "index.h"

#if COMPILEOTA
    #include <ArduinoOTA.h>
#endif


class WirelessTask : public Task<WirelessTask> {
    friend class Task<WirelessTask>;

public:
    WirelessTask(const uint8_t task_core);
    ~WirelessTask();
    void addSystemTaskQueue(QueueHandle_t queue);
    void addSystemSleepTimer(TimerHandle_t timer);
    void addMotorTaskQueue(QueueHandle_t queue);

protected:
    void run();

private:
    void connectWifi();
    void sendWebsocket(String message);
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

    TimerHandle_t system_sleep_timer_;     // Keep system from sleeping between driver startup and motor running
    QueueHandle_t system_task_queue_;      // To send messages to system task
    QueueHandle_t motor_task_queue_;       // To send messages to motor task
    String motor_position_;

    // Create AsyncWebServer object on port 80
    AsyncWebServer webserver;
    AsyncWebSocket websocket;
    const char* ssid     = "ESP32 Motorcover";
    const char* password = "123456789";

    String   ssid_       = secretSSID;      // SSID (name) for WiFi
    String   password_   = secretPass;      // Network password for WiFi
};