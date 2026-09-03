#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define MAX_LOG_ENTRIES 40

struct LogEntry {
    uint32_t timestamp_s;
    char text[120];
};

class SystemLogger {
private:
    LogEntry m_logs[MAX_LOG_ENTRIES];
    int m_logHead = 0;
    int m_logCount = 0;
    portMUX_TYPE m_logMux = portMUX_INITIALIZER_UNLOCKED;

public:
    SystemLogger() = default;
    
    void addLog(const char* format, ...);
    String getLogsJson();
    void clearLogs();
};

extern SystemLogger Logger;

#endif // LOGGER_H
