#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h> // Mqtt
#include "task.h"
#include "logger.h"
#include "peripheral_connections.h"
#include "secrets.h"


// Commands recieved from MQTT
enum Command {
    COVER_STOP    = -1,
    COVER_OPEN    = -2,
    COVER_CLOSE   = -3,
    COVER_SET_MIN = -4,
    COVER_SET_MAX = -5,
    SYS_RESET     = -98,
    SYS_REBOOT    = -99
};


class WirelessTask : public Task<WirelessTask> {
    friend class Task<WirelessTask>;

public:
    WirelessTask(const uint8_t task_core);

protected:
    void run();

private:
    void connectWifi();
    void connectMqtt();
    void readMqtt(char* topic, byte* buf, unsigned int len);
    void sendMqtt(String message);

    WiFiClient  wifi_client_;
    PubSubClient mqttClient_;
    String   ssid_          = secretSSID;      // SSID (name) for WiFi
    String   password_      = secretPass;      // Network password for WiFi
    String   mqtt_id_       = secretMqttID;    // MQTT ID for PubSubClient
    String   mqtt_user_     = secretMqttUser;  // MQTT server username (optional)
    String   mqtt_password_ = secretMqttPass;  // MQTT server password (optional)
    String   broker_ip_     = secretBrokerIP;  // IP of MQTT server
    uint16_t broker_port_   = secretBrokerPort;
    String   in_topic_      = secretInTopic;   // MQTT inbound topic
    String   out_topic_     = secretOutTopic;  // MQTT outbound topic
};