#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================================
// HARDWARE PIN CONFIGURATION (Standard ESP32 30-Pin / 38-Pin DevKit)
// =====================================================================
#define ENABLE_WEBSERVER     true   // Set to false to disable Wi-Fi & Web Server completely
#define ENABLE_OLED          false  // Set to false if running headless without OLED

#define OLED_SDA_PIN         14     // ESP32 GPIO 14
#define OLED_SCL_PIN         27     // ESP32 GPIO 27
#define OLED_VCC_PIN         -1     // Wire to dedicated 3.3V pin
#define USE_SH1106_1_3_INCH  1      // 1 for 1.3" SH1106, 0 for 0.96" SSD1306

// =====================================================================
// WI-FI CONFIGURATION
// =====================================================================
#if ENABLE_WEBSERVER
const char* const WIFI_SSID     = "Your_WiFi_SSID";     // Replace with your home/car Wi-Fi SSID
const char* const WIFI_PASS     = "Your_WiFi_Password"; // Replace with your Wi-Fi Password
const bool        TRY_STA_FIRST = true;                  // Try connecting to Wi-Fi first
const int         STA_TIMEOUT_S = 10;                    // Seconds before falling back to AP

// Fallback Access Point (AP) Settings
const char* const AP_SSID       = "ESP32_TPMS_Dashboard";
const char* const AP_PASS       = "12345678";            // Minimum 8 chars
#endif

// =====================================================================
// SENSOR WHITELIST
// =====================================================================
// IMPORTANT: Replace these MAC addresses and 6-character Short IDs with your own TPMS sensor MACs!
// Find your MAC addresses in the official JK Tyre SmartTyre app under Settings -> Sensor Debug.
const char* const SENSOR_MACS[4] = {
    "D2:58:6D:8F:16:10",  // FL: Front Left (Replace with your sensor MAC)
    "CA:E8:6C:2D:92:15",  // FR: Front Right (Replace with your sensor MAC)
    "F7:FC:85:AD:35:E2",  // RL: Rear Left (Replace with your sensor MAC)
    "CD:8D:E6:9E:FB:E6"   // RR: Rear Right (Replace with your sensor MAC)
};

const char* const SENSOR_SHORT_IDS[4] = {
    "8F1610",  // FL Short ID
    "2D9215",  // FR Short ID
    "AD35E2",  // RL Short ID
    "9EFBE6"   // RR Short ID
};

const char* const POS_NAMES[4] = {
    "Front Left",
    "Front Right",
    "Rear Left",
    "Rear Right"
};

#endif // CONFIG_H
