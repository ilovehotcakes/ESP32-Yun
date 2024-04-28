#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, webserver(80), websocket("/ws") {
}


WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    // connectWifi();  // For AP mode
    while (1) {
        if (WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        // If motor has changed position(%), broadcast it to all WS clients
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("Wireless task received message: %i", inbox_.command);
            motor_position_ = static_cast<String>(inbox_.command);
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
    // Root to get UI web page
    webserver.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    // RESTful api for turning on/off LED
    webserver.on("/led", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (!request->hasParam("state")) {
            request->send(400, "text/plain", "failed: <param>=state, i.e. /led?state=<value>");
            return;
        }
        int state = request->getParam("state")->value().toInt();
        if (state != 0 && state!= 1) {
            request->send(400, "text/plain", "failed: state=0 is off; state=1 is on");
            return;
        }
        digitalWrite(LED_PIN, state);
        request->send(200, "text/plain", "success");
    });

    // HTTP RESTful API for moving motor and changing motor settings
    webserver.on("/motor", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (request->params() > 1) {
            request->send(400, "text/plain", "failed: can only perform one motor action at a time");
            return;
        }
        if (httpRequestHandler(request, hash(MOTOR_MOVE), [=](int val) -> bool { return val > 100 || val < 0; },
                                           "position=0~100 (%); 0 to open; 100 to close")  // float
            || httpRequestHandler(request, hash(MOTOR_STOP), [=](int val) -> bool { return false; }, "")
            || httpRequestHandler(request, hash(MOTOR_SET_MIN), [=](int val) -> bool { return false; }, "")
            || httpRequestHandler(request, hash(MOTOR_SET_MAX), [=](int val) -> bool { return false; }, "")
            || httpRequestHandler(request, hash(MOTOR_STNDBY), [=](int val) -> bool { return val != 0 && val != 1; },
                                           "standby=0 | 1; 1 to standby motor driver; 0 to wake")
            || httpRequestHandler(request, hash(MOTOR_SET_DIR), [=](int val) -> bool { return val != 0 && val != 1; },
                                           "direction=0 | 1")
            || httpRequestHandler(request, hash(MOTOR_SET_MICSTP), [=](int val) -> bool { return val != 0 && val != 2 
                                                                                    && val != 4 && val != 8
                                                                                    && val != 16 && val != 32
                                                                                    && val != 64 && val != 128
                                                                                    && val != 256; },
                                           "microsteps=0 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256")
            || httpRequestHandler(request, hash(MOTOR_SET_FULSTP), [=](int val) -> bool { return val <= 0; },
                                           "full_step_per_rev has to be greater than 0")
            || httpRequestHandler(request, hash(MOTOR_SET_VELOC), [=](int val) -> bool { return val <= 0; },
                                           "velocity (Hz) has to be greater than 0")  // float
            || httpRequestHandler(request, hash(MOTOR_SET_ACCEL), [=](int val) -> bool { return val <= 0; },
                                           "acceleration has to be greater than 0")   // float
            || httpRequestHandler(request, hash(MOTOR_SET_OPCUR), [=](int val) -> bool { return val <= 0 || val > 2000; },
                                           "opening_current=1~2000 (mA); please refer to motor datasheet for max RMS")
            || httpRequestHandler(request, hash(MOTOR_SET_CLCUR), [=](int val) -> bool { return val <= 0 || val > 2000; },
                                           "closing_current=1~2000 (mA); please refer to motor datasheet for max RMS")
            || httpRequestHandler(request, hash(MOTOR_ENABLE_SG), [=](int val) -> bool { return val != 0 && val != 1; },
                                           "stallguard_enable=0 | 1; 0 to disable; 1 to enable")
            || httpRequestHandler(request, hash(MOTOR_SET_SGTHR), [=](int val) -> bool { return val < 0 || val > 255; },
                                           "stallguard_threshold=0~255; the greater, the easier to stall")) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listMotorCommands());
    });
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, String param, bool (*eval)(int), String error_message) {
    if (request->hasParam(param)) {
        String value_str = request->getParam(param)->value();  // To check if param="0" is value=0
        int value = value_str.toInt();
        if (eval(value) || ((param != "stop" && param != "min" && param != "max") && value == 0 && value_str != "0")) {
            request->send(400, "text/plain", "failed: " + error_message);
            return true;
        }
        Message(hash(param), value); // send to motor task
        request->send(200, "text/plain", "success");
        LOGI("Successfully parsing HTTP request: param=%s, value=%u", param.c_str(), value);
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
        Message outgoing(atoi((char*) data));

        LOGI("Received message from webserver: %s", outgoing.toString());

        if (outgoing.command > -1 && outgoing.command < 101) {  // Messages intended for motor task
            if (xQueueSend(motor_task_queue_, (void*) &outgoing, 0) != pdTRUE) {
                LOGE("Failed to send to motor_task queue_");
            }
        } else {
            LOGE("Position(%) of motor cannot be great than 100 or less than 0");
        }
    }
}


void WirelessTask::addSystemTaskQueue(QueueHandle_t queue) {
    system_task_queue_ = queue;
}


void WirelessTask::addSystemSleepTimer(TimerHandle_t timer) {
    system_sleep_timer_ = timer;
}


void WirelessTask::addMotorTaskQueue(QueueHandle_t queue) {
    motor_task_queue_ = queue;
}