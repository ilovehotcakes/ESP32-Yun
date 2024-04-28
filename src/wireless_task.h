#pragma once
/**
    wireless_task.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2023

    WirelessTask establishes and maintains WiFi connection and MQTT connection.
**/
#include <Arduino.h>
#include <queue>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include <esp_task_wdt.h>
#include "task.h"
#include "index.h"
#include "secrets.h"

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
    void addMotorTask(void *task);

protected:
    void run();

private:
    void connectWifi();
    void routing();
    bool httpRequestHandler(AsyncWebServerRequest *request, String param, bool (*eval)(int), String error_message);
    void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len);
    void wsEventDataProcessor(void *arg, uint8_t *data, size_t len);

    TimerHandle_t system_sleep_timer_;  // Prevent system from sleeping before processing incoming messages
    QueueHandle_t system_task_queue_;   // To send messages to system task
    Task *motor_task_;    // To send messages to motor task
    String motor_position_;

    // Create AsyncWebServer object on port 80
    AsyncWebServer webserver;
    AsyncWebSocket websocket;
    String ap_ssid_  = "ESP32 Motorcover";  // SSID (hostname) for AP
    String ssid_     = secretSSID;          // SSID (hostname) for WiFi
    String password_ = secretPass;          // Network password for WiFi
};