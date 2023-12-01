#include "wireless_task.h"


WirelessTask::WirelessTask(const uint8_t task_core) : 
        Task{"Wireless", 8192, 1, task_core},
        mqttClient_(wifi_client_)
{}


void WirelessTask::run() {
    disableCore0WDT();  // Disable watchdog timer

    while (1) {
        // Check wifi connection
        if (WiFi.status() != WL_CONNECTED) {
            connectWifi();
        }

        // Check mqtt connection
        if (!mqttClient_.connected()) {
            connectMqtt();
        }

        // Use non blocking method to check for messages
        mqttClient_.loop();

        // TODO
        // #if __has_include("ota.h")
        // ArduinoOTA.handle();
        // #endif
    }
}


// TODO: Add timeout and restart, watchdog timer
void WirelessTask::connectWifi() {
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    LOGI("Attempting to connect to WPA SSID: %s", ssid_.c_str());

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_.c_str(), password_.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(2000);
    }

    LOGI("Connected to the WiFi, IP: %s", WiFi.localIP().toString().c_str());
}


// TODO: add timeout and restart, watchdog timer
void WirelessTask::connectMqtt() {
    // Turn on LED to indicate disconnected
    digitalWrite(LED_PIN, HIGH);

    LOGI("Attempting to connect to MQTT broker: %s", broker_ip_.c_str());

    mqttClient_.setServer(broker_ip_.c_str(), broker_port_);
    
    while(!mqttClient_.connect(mqtt_id_.c_str(), mqtt_user_.c_str(), mqtt_password_.c_str()));

    mqttClient_.subscribe(in_topic_.c_str());
    // mqttClient_.setCallback(readMqtt);

    LOGI("Connected to the MQTT broker, topic: %s", in_topic_.c_str());

    digitalWrite(LED_PIN, LOW);
}


void WirelessTask::readMqtt(char* topic, byte* buf, unsigned int len) {
    String message = "";
    for (int i = 0; i < len; i++) message += (char) buf[i];
    // int command = message.toInt();

    // LOGI("Received message: %s", message);

    // if (command >= 0) motor_task.move(command);
    // else if (command == COVER_STOP) motor_task.stop();
    // else if (command == COVER_OPEN) motor_task.min();
    // else if (command == COVER_CLOSE) motor_task.max();
    // else if (command == COVER_SET_MAX) motor_task.setMax();
    // else if (command == COVER_SET_MIN) motor_task.setMin();
    // else if (command == SYS_RESET) motor_task.resetSettings();
    // else if (command == SYS_REBOOT) ESP.restart();
}


void WirelessTask::sendMqtt(String message) {
    mqttClient_.publish(secretOutTopic.c_str(), message.c_str());
    LOGI("Sent message: %s", message);
}