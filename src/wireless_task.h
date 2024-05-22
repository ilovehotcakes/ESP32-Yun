#pragma once
/**
    wireless_task.h - A class that handles all wireless communications.
    Author: Jason Chen, 2023

    WirelessTask establishes and maintains WiFi connections. It also serves a interactive webpage
    for users to control and change settings to the system and motor.
**/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FunctionalInterrupt.h>  // std:bind()
#include <esp_task_wdt.h>
#include "task.h"
#include "index.h"  // Index HTML webpage

#if COMPILEOTA
    #include <ArduinoOTA.h>
#endif

#define MAX_ATTEMPTS 9
#define WDT_DURATION 9  // Sec


class WirelessTask : public Task {
public:
    WirelessTask(const uint8_t task_core);
    ~WirelessTask();
    void addMotorTask(Task *task);
    void addSystemTask(Task *task);
    void addSystemSleepTimer(TimerHandle_t timer);

protected:
    void run();

private:
    AsyncWebServer webserver;   // Create AsyncWebServer object on port 80
    AsyncWebSocket websocket;
    String ap_ssid_      = "";  // SSID (hostname) for AP
    String sta_ssid_     = "";  // SSID (hostname) for WiFi
    String sta_password_ = "";  // Password for WiFi
    bool   setup_mode_   = false;
    bool   initialized_  = true;
    int    attempts_     = 1;

    Task *motor_task_;    // To send messages to motor task
    Task *system_task_;   // To send messages to system task
    TimerHandle_t system_sleep_timer_;  // Prevent system sleep before processing incoming messages
    String motor_position_ = "0";

    void loadSettings();
    void connectWifi();
    void routing();
    bool isPrefetch(AsyncWebServerRequest *request);
    bool hasOneParam(AsyncWebServerRequest *request);
    bool httpRequestHandler(AsyncWebServerRequest *request, Command command,
                            String &setting, const char *key);
    bool httpRequestHandler(AsyncWebServerRequest *request, Command command,
                            bool (*eval)(int), String error_message, Task *task);
    bool httpRequestHandler(AsyncWebServerRequest *request, Command command,
                            bool (*eval)(float), String error_message, Task *task);
    void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len);
};