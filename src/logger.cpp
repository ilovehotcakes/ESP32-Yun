#if COMPILELOGS  // if defined, exclude Serial.println statements when compiling

#include "logger.h"


LogLevel log_level = LogLevel::DEBUG;


void Logger(int baud_rate, LogLevel log_level) {
    Serial.begin(baud_rate);
    while(!Serial);  // Wait for serial port to connect
    log_level = log_level;
}


void print(LogLevel log_level, String s) {
    if (log_level <= log_level) Serial.println(s);
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

#endif