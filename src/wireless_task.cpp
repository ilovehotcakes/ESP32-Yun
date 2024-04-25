#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, webserver(80), websocket("/ws") {
}


WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    esp_task_wdt_add(getTaskHandle());
    // connectWifi();  // For AP mode
    while (1) {
        esp_task_wdt_reset();

        // Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        // Check if there are incoming messages from motor task
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            LOGD("Wireless task received message: %i", inbox_.command);
            motor_position_ = static_cast<String>(inbox_.command);
            sendWebsocket(motor_position_);
        }

        websocket.cleanupClients();

        #if COMPILEOTA
            ArduinoOTA.handle();
        #endif

        vTaskDelay(1);  // Finished all task within loop, handing control back to scheduler
    }
}


void WirelessTask::connectWifi() {
    // Turn on LED to indicate disconnected
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

    websocket.onEvent(std::bind(&WirelessTask::eventHandler, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                      std::placeholders::_5, std::placeholders::_6));

    webserver.addHandler(&websocket);

    // Route for root/web page
    webserver.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    #if COMPILEOTA
        ArduinoOTA.begin();
    #endif

    digitalWrite(LED_PIN, LOW);

    LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


void WirelessTask::sendWebsocket(String message) {
    websocket.textAll(message);
}


void WirelessTask::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = static_cast<AwsFrameInfo*>(arg);

    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        Message outbox(atoi((char*) data));

        LOGI("Received message from webserver: %i, %i", outbox.command, outbox.parameter);

        if (outbox.command > -10) {  // Messages intended for motor task
            xTimerStart(system_sleep_timer_, portMAX_DELAY);
            if (xQueueSend(motor_task_queue_, (void*) &outbox, 0) != pdTRUE) {
                LOGE("Failed to send to motor_task queue_");
            }
        } else if (outbox.command == GET_POSITION) {
            sendWebsocket(motor_position_);
        } else {  // Messages intended for system task
            if (xQueueSend(system_task_queue_, (void*) &outbox, 0) != pdTRUE) {
                LOGE("Failed to send to system_task queue_");
            }
        }
    }
}


void WirelessTask::eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            LOGI("WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            LOGI("WebSocket client #%u disconnected", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
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