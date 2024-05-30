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
        request->send_P(200, "text/html", index_html, 
                        std::bind(&WirelessTask::htmlStringProcessor, this, std::placeholders::_1));
    });

    // HTTP RESTful API for moving motor and changing motor settings
    webserver.on("/motor", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        httpRequestHandler(request);
    });

    // HTTP RESTful API for managing system
    webserver.on("/system", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        httpRequestHandler(request);
    });

    // HTTP RESTful API for managing wireless settings
    webserver.on("/wireless", HTTP_GET, [=](AsyncWebServerRequest *request) {
        if (isPrefetch(request)) {
            return;
        }
        httpRequestHandler(request);
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


void WirelessTask::httpRequestHandler(AsyncWebServerRequest *request) {
    // Prevent the system task from sleeping before finishing processing HTTP requests
    xTimerStart(system_sleep_timer_, portMAX_DELAY);

    if (request->params() > 10) {
        request->send(400, "text/plain", "too many parameters");
        return;
    }

    for (int i = 0; i < request->params(); i++) {
        String param = request->getParam(i)->name();
        if (param == "") {
            continue;
        }
        Command command = hash(param);
        if (command == ERROR_COMMAND) {
            String list_of_commands;
            if (request->url() == "/motor") {
                list_of_commands =  listMotorCommands();
            } else if (request->url() == "/system") {
                list_of_commands =  listSystemCommands();
            } else {
                list_of_commands =  listWirelessCommands();
            }
            request->send(400, "text/plain", "failed: <param>=" + param 
                             + " not accepted\nuse of these <param>=" + list_of_commands);
            return;
        }
    }

    String response = "";
    bool success = true;
    Task *task = motor_task_;
    if (request->url() == "/system") {
        task = system_task_;
    } else if (request->url() == "/wireless") {
        task = this;
    }

    for (int i = 0; i < request->params(); i++) {
        String param = request->getParam(i)->name();
        if (param == "") {
            continue;
        }
        String value_str = request->getParam(param)->value();
        Command command = hash(param);
        if (command >= MOTOR_VLCTY && command <= MOTOR_CL_ACCEL) {
            std::pair<std::function<bool(float)>, String> eval = getCommandEvalFuncf(command);
            float value = value_str.toFloat();
            if (eval.first(value)) {
                response += "failed: " + param + eval.second + "\n";
                success = false;
                break;
            }
            LOGI("Parsed HTTP request: param=%s, value=%.1f", param.c_str(), value);
            response += "success: " + param + "\n";
            sendTo(task, Message(command, value), portMAX_DELAY);
        } else if (command == WIRELESS_SSID) {
            if (value_str == "") {
                response += "failed: " + param + " needs to be a non-empty string\n";
                success = false;
                break;
            }
            LOGI("Parsed HTTP request: param=%s, value=%s", param.c_str(), value_str);
            response += "success: " + param + "\n";
            setAndSave(sta_ssid_, value_str, "sta_ssid_");
        } else if (command == WIRELESS_PASS) {
            LOGI("Parsed HTTP request: param=%s, value=%s", param.c_str(), value_str);
            response += "success: " + param + "\n";
            setAndSave(sta_password_, value_str, "sta_password_");
        } else {
            std::pair<std::function<bool(int)>, String> eval = getCommandEvalFunc(command);
            int value = value_str.toInt();
            if (eval.first(value) || (eval.second != "" && value == 0 && value_str != "0")) {
                response += "failed: " + param + eval.second + "\n";
                success = false;
                break;
            }
            LOGI("Parsed HTTP request: param=%s, value=%u", param.c_str(), value);
            response += "success: " + param + "\n";
            sendTo(task, Message(command, value), portMAX_DELAY);
        }
    }

    if (success) {
        request->send(200, "text/plain", response);
        delay(100 / portTICK_PERIOD_MS);
        // WS text all;
    } else {
        request->send(400, "text/plain", response);
    }
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


String WirelessTask::htmlStringProcessor(const String& var) {
    JsonDocument settings = motor_task_->getSettings();
    if (var == "SLIDER") {
        return motor_position_;
    } else if (var == "SYNC_SETTINGS") {
        if (settings["sync_settings_"]) return "checked";
        return "";
    } else if (var == "OP_CURR") {
        return settings["open_current_"];
    } else if (var == "CL_CURR") {
        return settings["clos_current_"];
    } else if (var == "OP_VELO") {
        return settings["open_velocity_"];
    } else if (var == "CL_VELO") {
        return settings["clos_velocity_"];
    } else if (var == "OP_ACCEL") {
        return settings["open_accel_"];
    } else if (var == "CL_ACCEL") {
        return settings["clos_accel_"];
    } else if (var  == "DIRECTION") {
        if (settings["direction_"]) return "checked";
        return "";
    } else if (var  == "FULL_STEPS") {
        return settings["full_steps_"];
    } else if (var  == "MICROSTEPS") {
        return settings["microsteps_"];
    } else if (var  == "FASTMODE") {
        if (settings["spreadcycl_en_"]) return "checked";
        return "";
    } else if (var  == "FASTMODE_THRESH") {
        return settings["spreadcycl_th_"];
    } else if (var  == "STALLGUARD") {
        if (settings["stallguard_en_"]) return "checked";
    } else if (var  == "STALLGUARD_THRESH") {
        return settings["stallguard_th_"];
    }
    return "";
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