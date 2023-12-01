#pragma once
/**
    logger.h - A simple Serial.println wrapper
    Aurthor: Jason Chen, 2022

    A simple wrapper file for Serial.println statements that imitates the output
    format of Espressif ESP32 logger library. Main features includes:
    - Output Serial.println statements based on LogLevel(ERROR, INFO, DEBUG).
      Logs that are higher level than set level will not be outputted.
    - Serial.println statements can be easily omitted during compiling.

    Usage:
    - #include "logger.h"
    - #define DONTCOMPILELOGS before #include "logger.h" if logs are to be 
      excluded when compiling.
    - LOG_INIT(baudRate, LogLevel) in setup()
    - Use LOGE(msg), LOGI(msg), LOGD(msg) in-place of Serial.println() for
      different level of logs.
**/
#ifndef DONTCOMPILELOGS  // if defined, exclude Serial.println statements when compiling

#include <Arduino.h>
#include <stdarg.h>

enum LogLevel {
    ERROR=0,
    INFO=1,
    DEBUG=2
};

void Logger(int, LogLevel);
void printErr(String, String, String, const char*, ...);
void printInf(String, String, String, const char*, ...);
void printDbg(String, String, String, const char*, ...);

#define LOG_INIT(baudRate, ll) Logger(baudRate, ll)
#define LOGE(msg, ...) printErr(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);
#define LOGI(msg, ...) printInf(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);
#define LOGD(msg, ...) printDbg(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);

#else

#define LOG_INIT(baudRate, ll)
#define LOGE(msg, ...)
#define LOGI(msg, ...)
#define LOGD(msg, ...)

#endif