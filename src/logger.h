/**
  logger.h - A simple logger wrapper
  Aurthor: Jason Chen

  A simple wrapper file for Serial.println statements that imitates the output
  format of Espressif ESP32 logger library. Main features includes:
  - Output Serial.println statements based on LogLevel(ERROR, INFO, DEBUG).
    Logs that are higher level than set level will not be outputted.
  - Serial.println statements easily can be omitted during compiling.

  Usage:
  - #include "logger.h"
  - #define DONTCOMPILELOGS before #include "logger.h" if logs are to be 
    excluded when compiling.
  - LOG_INIT(baudRate, LogLevel) in setup()
  - Use LOGE(msg), LOGI(msg), LOGD(msg) in-place of Serial.println() for
    different level of logs.
**/
#ifndef DONTCOMPILELOGS  // if defined, exclude Serial.println statements when compiling

#include "Arduino.h"
#include <stdarg.h>

enum LogLevel {
  ERROR=0,
  INFO=1,
  DEBUG=2
};


LogLevel logLevel = DEBUG;


void Logger(int, LogLevel);
void printErr(String, String, String, const char*, ...);
void printInf(String, String, String, const char*, ...);
void printDbg(String, String, String, const char*, ...);


#define LOG_INIT(baudRate, ll) Logger(baudRate, ll)
#define LOGE(msg, ...) printErr(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);
#define LOGI(msg, ...) printInf(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);
#define LOGD(msg, ...) printDbg(String(__FILE__), String(__LINE__), String(__func__), msg, ##__VA_ARGS__);


void Logger(int baudRate, LogLevel ll) {
  Serial.begin(baudRate);
  logLevel = ll;
}

void print(LogLevel ll, String s) {
  if (ll <= logLevel) Serial.println(s);
}

void printErr(String file, String line, String func, const char* s, ...) {
  String prefix = "[E][" + file + ":" + line + "] " + func + "(): ";

  char buf[128];
  va_list args;
	va_start(args, s);
	vsnprintf(buf, sizeof(buf), s, args);
	va_end(args);

	print(LogLevel::ERROR, prefix + String(buf));
}

void printInf(String file, String line, String func, const char* s, ...) {
  String prefix = "[I][" + file + ":" + line + "] " + func + "(): ";

  char buf[128];
  va_list args;
	va_start(args, s);
	vsnprintf(buf, sizeof(buf), s, args);
	va_end(args);

	print(LogLevel::INFO, prefix + String(buf));
}

void printDbg(String file, String line, String func, const char* s, ...) {
  String prefix = "[D][" + file + ":" + line + "] " + func + "(): ";

  char buf[128];
  va_list args;
	va_start(args, s);
	vsnprintf(buf, sizeof(buf), s, args);
	va_end(args);

	print(LogLevel::DEBUG, prefix + String(buf));
}

#else

#define LOG_INIT(baudRate, ll)
#define LOGE(msg, ...)
#define LOGI(msg, ...)
#define LOGD(msg, ...)

#endif