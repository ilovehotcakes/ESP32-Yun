#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, server(80), ws("/ws") {}


WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    disableCore0WDT();  // Disable watchdog timer
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    LOGI("Attempting to connect to WPA SSID: %s", ssid_.c_str());

    // ESP32 as STA
    // if (!WiFi.config(IPAddress(192, 168, 20, 208), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0))) {
    //     Serial.println("STA Failed to configure");
    // }
    WiFi.begin(ssid_.c_str(), password_.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());


    // // Setup ESPS32 as WiFi in access point (AP) mode; leave out password for no password
    // WiFi.softAP(ssid);
    // // Set ESP32's IP to 192.168.1.2
    // WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 2), IPAddress(255, 255, 255, 0));

    // Websocket server
    server.begin();

    ws.onEvent(std::bind(&WirelessTask::eventHandler, this, std::placeholders::_1, std::placeholders::_2,
               std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    server.addHandler(&ws);

    // Route for root / web page
    server.on("/", HTTP_GET, [=](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, std::bind(&WirelessTask::processor, this, std::placeholders::_1));
    });

    // Start server
    server.begin();
    
    digitalWrite(LED_PIN, LOW);

    // #if COMPILEOTA
    //     ArduinoOTA.begin();
    // #endif

    while (1) {
        ws.cleanupClients();

        // // Check WiFi connection
        // if (WiFi.status() != WL_CONNECTED) {
        //     connectWifi();
        // }

        // // Check MQTT connection
        // if (!mqtt_client_.connected()) {
        //     connectMqtt();
        // }

        // // Use non blocking method to check for messages
        // mqtt_client_.loop();

        // // Check if there is message sent from the motor
        // if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
        //     // LOGD("WirelessTask received message: %i", message);
        //     sendMqtt(static_cast<String>(inbox_.command));
        // }

        // #if COMPILEOTA
        //     ArduinoOTA.handle();
        // #endif
    }
}


// Restart on timeout
void WirelessTask::connectWifi() {
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    // LOGI("Attempting to connect to WPA SSID: %s", ssid_.c_str());

    // Setup ESPS32 as WiFi in access point (AP) mode; leave out password for no password
    WiFi.softAP(ssid, password);
    // Set ESP32's IP to 192.168.1.2
    WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 2), IPAddress(255, 255, 255, 0));  

    // Websocket server
    server.begin();

    ws.onEvent(std::bind(&WirelessTask::eventHandler, this, std::placeholders::_1, std::placeholders::_2,
               std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    server.addHandler(&ws);

    // Route for root / web page
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //     request->send_P(200, "text/html", index_html, processor);
    // });

    // Start server
    server.begin();

    // LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


// void WirelessTask::connectMqtt() {
//     // Turn on LED to indicate disconnected
//     digitalWrite(LED_PIN, HIGH);

//     LOGI("Attempting to connect to MQTT broker: %s", broker_ip_.c_str());

//     mqtt_client_.setServer(broker_ip_.c_str(), broker_port_);
    
//     while(!mqtt_client_.connect(mqtt_id_.c_str(), mqtt_user_.c_str(), mqtt_password_.c_str()));

//     mqtt_client_.subscribe(in_topic_.c_str());
//     mqtt_client_.setCallback(std::bind(&WirelessTask::readMqtt, this,
//                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

//     LOGI("Connected to the MQTT broker, topic: %s", in_topic_.c_str());

//     digitalWrite(LED_PIN, LOW);
// }


// void WirelessTask::readMqtt(char* topic, byte* buf, unsigned int len) {
//     String buffer = "";
//     for (int i = 0; i < len; i++) {
//         buffer += (char) buf[i];
//     }
//     Message outbox(buffer.toInt());

//     LOGI("Received message from MQTT server: %i, %i", outbox.command, outbox.parameter);

//     if (outbox.command > -10) {  // Messages intended for motor task
//         xTimerStart(system_sleep_timer_, portMAX_DELAY);
//         // For moving commands, need to startup driver first if it's in standby
//         if (outbox.command > -1 && uxSemaphoreGetCount(motor_standby_sem_) == 1) {
//             Message startup(STBY_OFF);
//             if (xQueueSend(motor_task_queue_, (void*) &startup, 10) != pdTRUE) {
//                 LOGE("Failed to send to motor_task queue_");
//             }
//             vTaskDelay(5);  // Wait for driver to startup
//         }

//         if (xQueueSend(motor_task_queue_, (void*) &outbox, 0) != pdTRUE) {
//             LOGE("Failed to send to motor_task queue_");
//         }
//     } else {  // Messages inteded for system task
//         if (xQueueSend(system_task_queue_, (void*) &outbox, 0) != pdTRUE) {
//             LOGE("Failed to send to system_task queue_");
//         }
//     }
// }


// void WirelessTask::sendMqtt(String message) {
//     mqtt_client_.publish(out_topic_.c_str(), message.c_str());
//     LOGI("Sent MQTT message: %s", message);
// }


void WirelessTask::addSystemTaskQueue(QueueHandle_t queue) {
    system_task_queue_ = queue;
}


void WirelessTask::addSystemSleepTimer(TimerHandle_t timer) {
    system_sleep_timer_ = timer;
}


void WirelessTask::addMotorTaskQueue(QueueHandle_t queue) {
    motor_task_queue_ = queue;
}

void WirelessTask::addMotorStandbySemaphore(SemaphoreHandle_t semaphore) {
    motor_standby_sem_ = semaphore;
}


void WirelessTask::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*) arg;

    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        Message outbox(atoi((char*) data));

        LOGI("Received message from MQTT server: %i, %i", outbox.command, outbox.parameter);

        if (outbox.command > -10) {  // Messages intended for motor task
            xTimerStart(system_sleep_timer_, portMAX_DELAY);
            // For moving commands, need to startup driver first if it's in standby
            if (outbox.command > -1 && uxSemaphoreGetCount(motor_standby_sem_) == 1) {
                Message startup(STBY_OFF);
                if (xQueueSend(motor_task_queue_, (void*) &startup, 10) != pdTRUE) {
                    LOGE("Failed to send to motor_task queue_");
                }
                vTaskDelay(5);  // Wait for driver to startup
            }

            if (xQueueSend(motor_task_queue_, (void*) &outbox, 0) != pdTRUE) {
                LOGE("Failed to send to motor_task queue_");
            }
        } else {  // Messages inteded for system task
            if (xQueueSend(system_task_queue_, (void*) &outbox, 0) != pdTRUE) {
                LOGE("Failed to send to system_task queue_");
            }
        }

        // if (strcmp((char*) data, "stop") == 0) {
        //     ws.textAll(String(ledState));
        // }
    }
}


void WirelessTask::eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}


String WirelessTask::processor(const String& var) {
    Serial.println(var);
    if (var == "STATE") {
        Serial.println("2");
        return ledState ? "ON" : "OFF";
    }
    if (var == "CHECK") {
        Serial.println("3");
        return ledState ? "checked" : "";
    }
    return String();
}