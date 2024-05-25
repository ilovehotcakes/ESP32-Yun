#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, webserver(80), websocket("/ws") {
    pinMode(LED_PIN, OUTPUT);
    esp_task_wdt_init(WDT_DURATION, true);  // Restart system if watchdog hasn't been fed
}


WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    loadSettings();

    while (1) {
        if (initialized_ && WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGI("WirelessTask received message: %s", inbox_.toString().c_str());
            switch (inbox_.command) {
                case UPDATE_POSITION:
                    // If motor has changed position(%), broadcast it to all WS clients
                    motor_position_ = static_cast<String>(inbox_.parameter);
                    websocket.textAll(motor_position_);
                    break;
                case WIRELESS_SETUP:
                    setAndSave(setup_mode_, static_cast<bool>(inbox_.parameter), "setup_mode_");
                    break;
            }
        }

        websocket.cleanupClients();  // Remove disconnected WS clients

        #if COMPILEOTA
            ArduinoOTA.handle();
        #endif

        vTaskDelay(1);  // Finished all task within loop, yielding control back to scheduler
    }
}


void WirelessTask::loadSettings() {
    bool load = readFromDisk();

    ap_ssid_ = getOrDefault("ap_ssid_",ap_ssid_);
    if (ap_ssid_ == "") {  // Factory reset
        ap_ssid_ = "yun-" + getSerialNumber().substring(6, 12);
        settings_["ap_ssid_"] = ap_ssid_;
    }
    sta_ssid_ = getOrDefault("sta_ssid_", sta_ssid_);
    sta_password_ = getOrDefault("sta_password_", sta_password_);
    attempts_ = getOrDefault("attempts_", attempts_);
    if (sta_ssid_ == "" || attempts_ > MAX_ATTEMPTS) {
        setup_mode_ = true;
        setAndSave(setup_mode_, true, "setup_mode_");
    } else {
        setup_mode_ = getOrDefault("setup_mode_", setup_mode_);
    }

    if (!load) {
        writeToDisk();
    }

    LOGI("Wireless settings loaded, attempt #%u", attempts_);
}


void WirelessTask::connectWifi() {
    // Turn on LED to indicate not connected
    digitalWrite(LED_PIN, HIGH);

    if (!setup_mode_) {
        // ESP32 in STA mode
        LOGI("Attempting to connect to WiFi, SSID=%s, password=%s", sta_ssid_.c_str(), sta_password_.c_str());
        setAndSave(attempts_, attempts_ + 1, "attempts_");
        esp_task_wdt_add(getTaskHandle());
        WiFi.begin(sta_ssid_.c_str(), sta_password_.c_str());
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
        }
        // Remove wireless task from watchdog timer to avoid manually feeding WDT
        esp_task_wdt_delete(getTaskHandle());
        LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
    } else {
        // ESP32 in AP mode which acts as an router
        LOGI("Starting AP, SSID: %s", ap_ssid_.c_str());
        WiFi.softAP(ap_ssid_.c_str());
        initialized_ = false;
    }

    if (!MDNS.begin(ap_ssid_.c_str())) {
        LOGE("Failed to set mDNS responder");
    }
    MDNS.addService("_osc", "_tcp", 80);

    websocket.onEvent(std::bind(&WirelessTask::wsEventHandler, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                      std::placeholders::_5, std::placeholders::_6));

    webserver.addHandler(&websocket);

    routing();

    webserver.begin();

    #if COMPILEOTA
        ArduinoOTA.setHostname(ap_ssid_.c_str());
        ArduinoOTA.begin();
    #endif

    setAndSave(attempts_, 1, "attempts_");

    digitalWrite(LED_PIN, LOW);
}


void WirelessTask::routing() {
    // Root serves UI web page
    webserver.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, [=](const String& var) -> String {
            if (var == "SLIDER") {
                return motor_position_;
            }
            return String();
        });
    });

    // HTTP RESTful API for managing system
    webserver.on("/system", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        if (!hasOneParam(request)) {
            return;
        }
        if (httpRequestHandler(request, SYSTEM_SLEEP, [=](int val) -> bool { return false; }, "", system_task_)
            || httpRequestHandler(request, SYSTEM_RESTART, [=](int val) -> bool { return false; }, "", system_task_)
            || httpRequestHandler(request, SYSTEM_RESET, [=](int val) -> bool { return false; }, "", system_task_)) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listSystemCommands());
    });

    // HTTP RESTful API for managing wireless settings
    webserver.on("/wireless", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        if (!hasOneParam(request)) {
            return;
        }
        if (httpRequestHandler(request, WIRELESS_SETUP, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1; 1 to enter setup mode", this)
            || httpRequestHandler(request, WIRELESS_SSID, sta_ssid_, "sta_ssid_")
            || httpRequestHandler(request, WIRELESS_PASS, sta_password_, "sta_password_")) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listWirelessCommands());
    });

    // HTTP RESTful API for moving motor and changing motor settings
    webserver.on("/motor", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        if (!hasOneParam(request)) {
            return;
        }
        if (httpRequestHandler(request, MOTOR_PERECENT, [=](int val) -> bool { return val > 100 || val < 0; },
                                  "=0~100 (%); 0 to open; 100 to close", motor_task_)
            || httpRequestHandler(request, MOTOR_STEP, [=](int val) -> bool { return val < 0; }, ">=0", motor_task_)
            || httpRequestHandler(request, MOTOR_STOP, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_FORWARD, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_BACKWARD, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_SET_MIN, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_SET_MAX, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_ZERO, [=](int val) -> bool { return false; }, "", motor_task_)
            || httpRequestHandler(request, MOTOR_STANDBY, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1; 1 to standby motor driver; 0 to start", motor_task_)
            || httpRequestHandler(request, MOTOR_SYNC_STTNG, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1; 0 to keep opening/closing settings the same", motor_task_)
            || httpRequestHandler(request, MOTOR_VLCTY, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0 (Hz)", motor_task_)  // float
            || httpRequestHandler(request, MOTOR_OP_VLCTY, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0 (Hz)", motor_task_)  // float
            || httpRequestHandler(request, MOTOR_CL_VLCTY, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0 (Hz)", motor_task_)  // float
            || httpRequestHandler(request, MOTOR_ACCEL, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0", motor_task_)   // float
            || httpRequestHandler(request, MOTOR_OP_ACCEL, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0", motor_task_)   // float
            || httpRequestHandler(request, MOTOR_CL_ACCEL, [=](float val) -> bool { return val <= 0.0; },
                                  ">0.0", motor_task_)   // float
            || httpRequestHandler(request, MOTOR_CURRENT, [=](int val) -> bool { return val < 1 || val > 2000; },
                                  "=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, MOTOR_OP_CURRENT, [=](int val) -> bool { return val < 1 || val > 2000; },
                                  "=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, MOTOR_CL_CURRENT, [=](int val) -> bool { return val < 1 || val > 2000; },
                                  "=1~2000 (mA); please refer to motor datasheet for max RMS", motor_task_)
            || httpRequestHandler(request, MOTOR_DIRECTION, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1", motor_task_)
            || httpRequestHandler(request, MOTOR_FULL_STEPS, [=](int val) -> bool { return val <= 0; },
                                  ">0", motor_task_)
            || httpRequestHandler(request, MOTOR_MICROSTEPS, [=](int val) -> bool { return val != 0 && val != 2 
                                                                                    && val != 4 && val != 8
                                                                                    && val != 16 && val != 32
                                                                                    && val != 64 && val != 128
                                                                                    && val != 256; },
                                  "=0 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256", motor_task_)
            || httpRequestHandler(request, MOTOR_STALLGUARD, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1; 0 to disable; 1 to enable", motor_task_)
            || httpRequestHandler(request, MOTOR_TCOOLTHRS, [=](int val) -> bool { return val < 0 || val > 1048575; },
                                  "=0~1048575; lower threshold velocity for switching on stallguard", motor_task_)
            || httpRequestHandler(request, MOTOR_SGTHRS, [=](int val) -> bool { return val < 0 || val > 255; },
                                  "=0~255; the greater, the easier to stall", motor_task_)
            || httpRequestHandler(request, MOTOR_SPREADCYCL, [=](int val) -> bool { return val != 0 && val != 1; },
                                  "=0 | 1; 0 to disable; 1 to enable", motor_task_)
            || httpRequestHandler(request, MOTOR_TPWMTHRS, [=](int val) -> bool { return val < 0 || val > 1048575; },
                                  "=0~1048575; upper threshold to switch to fastmode", motor_task_)) {
            return;
        }
        request->send(400, "text/plain", "failed: param not accepted\nuse of these <param>=" + listMotorCommands());
    });

    webserver.on("/json", HTTP_GET, [=](AsyncWebServerRequest *request) {
        JsonDocument all_settings;
        all_settings["system"] = system_task_->getSettings();
        all_settings["wireless"] = getSettings();
        all_settings["motor"] = motor_task_->getSettings();
        String result;
        serializeJson(all_settings, result);
        request->send(200, "application/json", result);
    });

    webserver.onNotFound([=](AsyncWebServerRequest *request) {
        if(request->method() == HTTP_GET) {
            request->send(404, "text/plain", "failed: use /motor? or /system? or /wireless? or /json" );
        }
    });
}


bool WirelessTask::isPrefetch(AsyncWebServerRequest *request) {
    for(int i = 0; i < request->headers(); i++){
        AsyncWebHeader *header = request->getHeader(i);
        if ((header->name() == "Purpose" && header->value() == "prefetch")    // Chrome/Safari/Edge
         || (header->name() == "X-Purpose" && header->value() == "prefetch")  // Safari (old)
         || (header->name() == "X-moz" && header->value() == "prefetch")) {   // Firefox
            return true;
        }
    }
    return false;
}


bool WirelessTask::hasOneParam(AsyncWebServerRequest *request) {
    if (request->params() > 1) {
        request->send(400, "text/plain", "failed: can only perform one request at a time");
        return false;
    }
    return true;
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, Command command,
                                      String &setting, const char *key) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    String param = hash(command);
    if (request->hasParam(param)) {
        String value = request->getParam(param)->value();
        LOGI("Parsed HTTP request: param=%s, value=%s", param.c_str(), value);
        request->send(200, "text/plain", "success");
        setAndSave(setting, value, key);
        return true;
    }
    return false;
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, Command command,
                                      bool (*eval)(int), String error_message, Task *task) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    String param = hash(command);
    if (request->hasParam(param)) {
        String value_str = request->getParam(param)->value();  // To check if param="0" is value=0
        int value = value_str.toInt();
        if (eval(value) || (error_message != "" && value == 0 && value_str != "0")) {
            request->send(400, "text/plain", "failed: " + param + error_message);
            return true;
        }
        LOGI("Parsed HTTP request: param=%s, value=%u", param.c_str(), value);
        request->send(200, "text/plain", "success");
        sendTo(task, Message(command, value), 0);
        return true;
    }
    return false;
}


bool WirelessTask::httpRequestHandler(AsyncWebServerRequest *request, Command command,
                                      bool (*eval)(float), String error_message, Task *task) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    String param = hash(command);
    xTimerStart(system_sleep_timer_, portMAX_DELAY);
    if (request->hasParam(param)) {
        float value = request->getParam(param)->value().toFloat();
        if (eval(value)) {
            request->send(400, "text/plain", "failed: " + param + error_message);
            return true;
        }
        LOGI("Parsed HTTP request: param=%s, value=%.1f", param.c_str(), value);
        request->send(200, "text/plain", "success");
        sendTo(task, Message(command, value), 0);
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
            LOGI("WebSocket client #%u sent a message", client->id());
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            LOGE("WebSocket client #%u error: %u", client->id(), *(static_cast<uint16_t*>(arg)));
            break;
    }
}


void WirelessTask::addMotorTask(Task *task) {
    motor_task_ = task;
}


void WirelessTask::addSystemTask(Task *task) {
    system_task_ = task;
}


void WirelessTask::addSystemSleepTimer(TimerHandle_t timer) {
    system_sleep_timer_ = timer;
}