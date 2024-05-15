#pragma once
/**
    wireless_task.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2023

    WirelessTask establishes and maintains WiFi connection and MQTT connection.
**/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include <esp_task_wdt.h>
#include "task.h"
#include "system_task.h"
#include "index.h"
// #include "secrets.h"

#if COMPILEOTA
    #include <ArduinoOTA.h>
#endif


class WirelessTask : public Task {
public:
    WirelessTask(const uint8_t task_core);
    ~WirelessTask();
    void addMotorTask(Task *task);
    void addSystemTask(Task *task);

protected:
    void run();

private:
    // Create AsyncWebServer object on port 80
    AsyncWebServer webserver;
    AsyncWebSocket websocket;
    String ap_ssid_      = "";          // SSID (hostname) for AP
    String sta_ssid_     = "secretSSID";  // SSID (hostname) for WiFi
    String sta_password_ = "secretPass";  // Network password for WiFi

    Task *motor_task_;    // To send messages to motor task
    Task *system_task_;   // To send messages to system task
    TimerHandle_t system_sleep_timer_;  // Prevent system sleep before processing incoming messages
    String motor_position_;

    void loadSettings();
    void connectWifi();
    void routing();
    bool isPrefetch(AsyncWebServerRequest *request);
    bool hasOneParam(AsyncWebServerRequest *request);
    bool httpRequestHandler(AsyncWebServerRequest *request, Command command,
                            bool (*eval)(int), String error_message, Task *task);
    bool httpRequestHandler(AsyncWebServerRequest *request, Command command,
                            bool (*eval)(float), String error_message, Task *task);
    void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len);
};