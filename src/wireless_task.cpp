#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"WirelessTask", 8192, 1, task_core, 99}, mqtt_client_(wifi_client_) {}


WirelessTask::~WirelessTask() {}


void WirelessTask::run() {
    disableCore0WDT();  // Disable watchdog timer
    connectWifi();      // Must connect to WiFi before setting up OTA

    #if COMPILEOTA
        ArduinoOTA.begin();
    #endif

    while (1) {
        // Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        // Check MQTT connection
        if (!mqtt_client_.connected()) {
            connectMqtt();
        }

        // Use non blocking method to check for messages
        mqtt_client_.loop();

        // Check if there is message sent from the motor
        if (xQueueReceive(queue_, (void*) &inbox_, 0) == pdTRUE) {
            // LOGD("WirelessTask received message: %i", message);
            sendMqtt(static_cast<String>(inbox_.command));
        }

        #if COMPILEOTA
            ArduinoOTA.handle();
        #endif
    }
}


// Restart on timeout
void WirelessTask::connectWifi() {
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    LOGI("Attempting to connect to WPA SSID: %s", ssid_.c_str());

    WiFi.disconnect();
    // WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_.c_str(), password_.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(2000);
    }

    LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


void WirelessTask::connectMqtt() {
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    LOGI("Attempting to connect to MQTT broker: %s", broker_ip_.c_str());

    mqtt_client_.setServer(broker_ip_.c_str(), broker_port_);
    
    while(!mqtt_client_.connect(mqtt_id_.c_str(), mqtt_user_.c_str(), mqtt_password_.c_str()));

    mqtt_client_.subscribe(in_topic_.c_str());
    mqtt_client_.setCallback(std::bind(&WirelessTask::readMqtt, this,
                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    LOGI("Connected to the MQTT broker, topic: %s", in_topic_.c_str());

    digitalWrite(LED_PIN, LOW);
}


void WirelessTask::readMqtt(char* topic, byte* buf, unsigned int len) {
    String buffer = "";
    for (int i = 0; i < len; i++) {
        buffer += (char) buf[i];
    }
    Message outbox(buffer.toInt());

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
}


void WirelessTask::sendMqtt(String message) {
    mqtt_client_.publish(out_topic_.c_str(), message.c_str());
    LOGI("Sent MQTT message: %s", message);
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

void WirelessTask::addMotorStandbySemaphore(SemaphoreHandle_t semaphore) {
    motor_standby_sem_ = semaphore;
}