#include "Logger.h"

SystemLogger Logger;

void SystemLogger::addLog(const char* format, ...) {
    char buf[120];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.println(buf);

    portENTER_CRITICAL(&m_logMux);
    m_logs[m_logHead].timestamp_s = millis() / 1000;
    strncpy(m_logs[m_logHead].text, buf, sizeof(m_logs[m_logHead].text) - 1);
    m_logs[m_logHead].text[sizeof(m_logs[m_logHead].text) - 1] = '\0';
    m_logHead = (m_logHead + 1) % MAX_LOG_ENTRIES;
    if (m_logCount < MAX_LOG_ENTRIES) m_logCount++;
    portEXIT_CRITICAL(&m_logMux);
}

String SystemLogger::getLogsJson() {
    String json = "[";
    portENTER_CRITICAL(&m_logMux);
    int startIdx = (m_logCount < MAX_LOG_ENTRIES) ? 0 : m_logHead;
    for (int i = 0; i < m_logCount; i++) {
        int idx = (startIdx + i) % MAX_LOG_ENTRIES;
        json += "{\"t\":" + String(m_logs[idx].timestamp_s) + ",\"msg\":\"" + String(m_logs[idx].text) + "\"}";
        if (i < m_logCount - 1) json += ",";
    }
    portEXIT_CRITICAL(&m_logMux);
    json += "]";
    return json;
}

void SystemLogger::clearLogs() {
    portENTER_CRITICAL(&m_logMux);
    m_logCount = 0;
    m_logHead = 0;
    portEXIT_CRITICAL(&m_logMux);
}
