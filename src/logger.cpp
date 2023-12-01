#include "logger.h"


LogLevel logLevel = LogLevel::DEBUG;


void Logger(int baudRate, LogLevel ll) {
    Serial.begin(baudRate);
    while(!Serial);  // Wait for serial port to connect
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