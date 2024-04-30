#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, webserver(80), websocket("/ws") {}
WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    esp_task_wdt_init(10, true);  // Restart system if wd hasn't been fed in 10 seconds

    // connectWifi();  // For AP mode

    while (1) {
        if (WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        // If motor has changed position(%), broadcast it to all WS clients
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("WirelessTask received message: %s", inbox_.toString().c_str());
            motor_position_ = static_cast<String>(inbox_.parameter);
            websocket.textAll(motor_position_);
        }

        websocket.cleanupClients();  // Remove disconnected WS clients

        #if COMPILEOTA
            ArduinoOTA.handle();
        #endif

        vTaskDelay(1);  // Finished all task within loop, yielding control back to scheduler
    }
}


void WirelessTask::connectWifi() {
    esp_task_wdt_add(getTaskHandle());

    // Turn on LED to indicate not connected
    digitalWrite(LED_PIN, HIGH);

    // ESP32 in STA mode
    LOGI("Attempting to connect to WPA SSID: %s", ssid_.c_str());
    WiFi.begin(ssid_.c_str(), password_.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }

    // Setup ESPS32 as WiFi in access point (AP) mode; leave out password for no password
    // WiFi.softAP(ap_ssid_.c_str());
    // // Set ESP32's IP to 192.168.1.2
    // WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 2), IPAddress(255, 255, 255, 0));

    // Websocket server
    webserver.begin();

    websocket.onEvent(std::bind(&WirelessTask::wsEventHandler, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                      std::placeholders::_5, std::placeholders::_6));

    webserver.addHandler(&websocket);

    routing();

    #if COMPILEOTA
        ArduinoOTA.begin();  // TODO: check if OTA needs to be restarted after reconnection
    #endif

    digitalWrite(LED_PIN, LOW);

    // Remove wireless task from watchdog timer to avoid manually feeding WDT
    esp_task_wdt_delete(getTaskHandle());

    LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


void WirelessTask::routing() {
    // Root serves UI web page
    webserver.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    // HTTP RESTful API for managing system
    webserver.on("/system", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (request->params() > 1) {
            request->send(400, "text/plain", "failed: can only perform one system action at a time");
            return;
        }
        if (httpRequestHandler(request, hash(SYSTEM_SLEEP), [=](int val) -> bool { return false; }, "", system_task_)
            || httpRequestHandler(request, hash(SYSTEM_REBOOT), [=](int val) -> bool { return false; }, "", system_task_)
            || httpRequestHandler(request, hash(SYSTEM_RESET), [=](int val) -> bool { return false; }, "", system_task_)) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listSystemCommands());
    });

    // HTTP RESTful API for moving motor and changing motor settings
    webserver.on("/motor", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (request->params() > 1) {
            request->send(400, "text/plain", "failed: can only perform one motor action at a time");
            return;
        }
        if (httpRequestHandler(request, hash(MOTOR_MOVE), [=](int val) -> bool { return val > 100 || val < 0; },
                                  "position=0~100 (%); 0 to open; 100 to close", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_STOP), [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_SET_MIN), [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_SET_MAX), [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_STNDBY), [=](int val) -> bool { return val != 0 && val != 1; },
                                  "standby=0 | 1; 1 to standby motor driver; 0 to wake", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_OPENCLOSE), [=](int val) -> bool { return val != 0 && val != 1; },
                                  "open-close=0 | 1; 0 to keep opening/closing settings the same", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_SET_VELO), [=](float val) -> bool { return val <= 0.0; },
                                  "velocity (Hz) has to be greater than 0", motor_task_)  // float
            || httpRequestHandler(request, hash(MOTOR_SET_OPVELO), [=](float val) -> bool { return val <= 0.0; },
                                  "opening-velocity (Hz) has to be greater than 0", motor_task_)  // float
            || httpRequestHandler(request, hash(MOTOR_SET_CLVELO), [=](float val) -> bool { return val <= 0.0; },
                                  "closing-velocity (Hz) has to be greater than 0", motor_task_)  // float
            || httpRequestHandler(request, hash(MOTOR_SET_ACCL), [=](float val) -> bool { return val <= 0.0; },
                                  "acceleration has to be greater than 0.0", motor_task_)   // float
            || httpRequestHandler(request, hash(MOTOR_SET_OPACCL), [=](float val) -> bool { return val <= 0.0; },
                                  "opening-acceleration has to be greater than 0", motor_task_)   // float
            || httpRequestHandler(request, hash(MOTOR_SET_CLACCL), [=](float val) -> bool { return val <= 0.0; },
                                  "closing-acceleration has to be greater than 0", motor_task_)   // float
            || httpRequestHandler(request, hash(MOTOR_CURRENT), [=](int val) -> bool { return val <= 0 || val > 2000; },
                                  "current=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_OPCURRENT), [=](int val) -> bool { return val <= 0 || val > 2000; },
                                  "opening-current=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_CLCURRENT), [=](int val) -> bool { return val <= 0 || val > 2000; },
                                  "closing-current=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_DIRECTION), [=](int val) -> bool { return val != 0 && val != 1; },
                                  "direction=0 | 1", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_MICROSTEPS), [=](int val) -> bool { return val != 0 && val != 2 
                                                                                    && val != 4 && val != 8
                                                                                    && val != 16 && val != 32
                                                                                    && val != 64 && val != 128
                                                                                    && val != 256; },
                                  "microsteps=0 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_FULLSTEPS), [=](int val) -> bool { return val <= 0; },
                                  "full-step-per-rev has to be greater than 0", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_STALLGUARD), [=](int val) -> bool { return val != 0 && val != 1; },
                                  "stallguard=0 | 1; 0 to disable; 1 to enable", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_TCOOLTHRS), [=](int val) -> bool { return val < 0 || val > 1048575; },
                                  "coolstep-threshold=0~1048575; lower threshold velocity for switching on stallguard", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_SGTHRS), [=](int val) -> bool { return val < 0 || val > 255; },
                                  "stallguard-threshold=0~255; the greater, the easier to stall", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_SPREADCYCL), [=](int val) -> bool { return val != 0 && val != 1; },
                                  "fastmode=0 | 1; 0 to disable; 1 to enable", motor_task_)
            || httpRequestHandler(request, hash(MOTOR_TPWMTHRS), [=](int val) -> bool { return val < 0 || val > 1048575; },
                                  "fastmode-threshold=0~1048575; upper threshold to switch to fastmode", motor_task_)) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listMotorCommands());
    });
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, String param,
                                      bool (*eval)(int), String error_message, Task *task) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    if (request->hasParam(param)) {
        String value_str = request->getParam(param)->value();  // To check if param="0" is value=0
        int value = value_str.toInt();
        if (eval(value) || (error_message != "" && value == 0 && value_str != "0")) {
            request->send(400, "text/plain", "failed: " + error_message);
            return true;
        }
        LOGI("Parsed HTTP request: param=%s, value=%u", param.c_str(), value);
        request->send(200, "text/plain", "success");
        sendTo(task, Message(hash(param), value), 0);
        return true;
    }
    return false;
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, String param,
                                      bool (*eval)(float), String error_message, Task *task) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    if (request->hasParam(param)) {
        float value = request->getParam(param)->value().toFloat();
        if (eval(value)) {
            request->send(400, "text/plain", "failed: " + error_message);
            return true;
        }
        LOGI("Parsed HTTP request: param=%s, value=%.1f", param.c_str(), value);
        request->send(200, "text/plain", "success");
        sendTo(task, Message(hash(param), value), 0);
        return true;
    }
    return false;
}


void WirelessTask::wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    // Prevent the system task from sleeping before finishing processing WS events
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    switch (type) {
        case WS_EVT_CONNECT:
            client->printf(motor_position_.c_str());
            LOGI("WebSocket client #%u connected from %s", client->id(),
                                                           client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            LOGI("WebSocket client #%u disconnected", client->id());
            break;
        case WS_EVT_DATA:
            wsEventDataProcessor(arg, data, len);  // Used for reading slider position
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            LOGE("WebSocket client #%u error: %u", client->id(), *((uint16_t*) arg));
            break;
    }
}


void WirelessTask::wsEventDataProcessor(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = static_cast<AwsFrameInfo*>(arg);

    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        Message outgoing(MOTOR_MOVE, atoi((char*) data));

        LOGI("Received message from WebSocket: %s", outgoing.toString());

        if (outgoing.command > -1 && outgoing.command < 101) {  // Messages intended for motor task
            sendTo((Task*) motor_task_, outgoing, 0);
        } else {
            LOGE("Position (%) of motor cannot be great than 100 or less than 0");
        }
    }
}


void WirelessTask::addMotorTask(void *task) {
    motor_task_ = static_cast<Task*>(task);
}


void WirelessTask::addSystemTask(void *task) {
    system_task_ = static_cast<Task*>(task);
    system_sleep_timer_ = static_cast<SystemTask*>(task)->getSystemSleepTimer();
}