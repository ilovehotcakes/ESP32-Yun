#pragma once
/**
    wireless_task.h - A class that contains all stepper motor attribute and controls.
    Author: Jason Chen, 2023

    WirelessTask establishes and maintains WiFi connection and MQTT connection.
**/
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>         // Mqtt
#include <FunctionalInterrupt.h>  // std:bind()
#include "task.h"
#include "logger.h"
#include "secrets.h"

#if COMPILEOTA
    #include <ArduinoOTA.h>
#endif


class WirelessTask : public Task<WirelessTask> {
    friend class Task<WirelessTask>;

public:
    WirelessTask(const uint8_t task_core);
    ~WirelessTask();
    void addSystemTaskQueue(QueueHandle_t queue);
    void addMotorTaskQueue(QueueHandle_t queue);
    void addMotorStandbySemaphore(SemaphoreHandle_t semaphore);

protected:
    void run();

private:
    void connectWifi();
    void connectMqtt();
    void readMqtt(char* topic, byte* buf, unsigned int len);
    void sendMqtt(String message);

    QueueHandle_t system_task_queue_;      // Used to send messages to system task
    QueueHandle_t motor_task_queue_;       // Used to send messages to motor task
    SemaphoreHandle_t motor_standby_sem_;  // Used to signal to motor driver to startup

    WiFiClient  wifi_client_;
    PubSubClient mqtt_client_;
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