#ifndef WEB_SERVER_MANAGER_C3_H
#define WEB_SERVER_MANAGER_C3_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <TreelTPMS.h>
#include "Config.h"
#include "Logger.h"

class WebServerManager {
private:
#if ENABLE_WEBSERVER
    WebServer m_server;
    DNSServer m_dnsServer;
    bool m_apActive = false;
#endif
    String m_wifiModeStr = "Disconnected";
    String m_ipAddress = "0.0.0.0";

public:
    WebServerManager();

    void begin();
    void handleClient();

    String getWifiMode() const { return m_wifiModeStr; }
    String getIpAddress() const { return m_ipAddress; }

private:
    void handleRoot();
    void handleApiData();
    void handleApiLogs();
    void handleApiClear();
    void handleGetSettings();
    void handlePostSettings();
};

extern WebServerManager WebDash;

#endif // WEB_SERVER_MANAGER_C3_H
