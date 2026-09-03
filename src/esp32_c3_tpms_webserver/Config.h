#ifndef CONFIG_C3_H
#define CONFIG_C3_H

#include <Arduino.h>

// =====================================================================
// HARDWARE PIN CONFIGURATION & FEATURE SWITCHES (ESP32-C3 SuperMini)
// =====================================================================
#define ENABLE_WEBSERVER     true    // Set to false to disable Wi-Fi and Web Server (Pure BLE / Ultra Low Power)
#define ENABLE_OLED          true   // Set to true if an I2C OLED screen is physically attached
#define ENABLE_DEMO_MODE     false  // Set to true to test OLED, Web Dashboard & REST API with dummy values & alerts

#define OLED_SDA_PIN         8      // ESP32-C3 SuperMini I2C SDA (GPIO 8)
#define OLED_SCL_PIN         9      // ESP32-C3 SuperMini I2C SCL (GPIO 9)
#define USE_SH1106_1_3_INCH  1      // 1 for 1.3" SH1106, 0 for 0.96" SSD1306

// =====================================================================
// WI-FI CONFIGURATION
// =====================================================================
#if ENABLE_WEBSERVER
const char* const WIFI_SSID     = "Your_WiFi_SSID";     // Replace with your home/car Wi-Fi SSID
const char* const WIFI_PASS     = "Your_WiFi_Password"; // Replace with your Wi-Fi Password
const bool        TRY_STA_FIRST = true;                  // Try connecting to Wi-Fi first
const int         STA_TIMEOUT_S = 8;                     // Fast fallback timeout for single-core RISC-V

// Fallback Access Point (AP) Settings
const char* const AP_SSID       = "ESP32C3_TPMS_Dashboard";
const char* const AP_PASS       = "12345678";            // Minimum 8 chars
#endif

// =====================================================================
// SENSOR WHITELIST & BINARY SIGNATURES (ZERO-ALLOCATION MATCHING)
// =====================================================================
// IMPORTANT: Replace these MAC addresses and Short IDs with your own TPMS sensor MACs!
// Find your MAC addresses in the official JK Tyre SmartTyre app under Settings -> Sensor Debug.
static const uint8_t SENSOR_MACS_BIN[4][6] = {
    {0xD2, 0x58, 0x6D, 0x8F, 0x16, 0x10}, // FL: Front Left (Replace with your MAC)
    {0xCA, 0xE8, 0x6C, 0x2D, 0x92, 0x15}, // FR: Front Right (Replace with your MAC)
    {0xF7, 0xFC, 0x85, 0xAD, 0x35, 0xE2}, // RL: Rear Left (Replace with your MAC)
    {0xCD, 0x8D, 0xE6, 0x9E, 0xFB, 0xE6}  // RR: Rear Right (Replace with your MAC)
};

static const uint8_t SENSOR_SHORT_SIGS[4][3] = {
    {0x8F, 0x16, 0x10}, // FL Short ID (8F1610)
    {0x2D, 0x92, 0x15}, // FR Short ID (2D9215)
    {0xAD, 0x35, 0xE2}, // RL Short ID (AD35E2)
    {0x9E, 0xFB, 0xE6}  // RR Short ID (9EFBE6)
};

const char* const SENSOR_MAC_STRS[4] = {
    "D2:58:6D:8F:16:10",
    "CA:E8:6C:2D:92:15",
    "F7:FC:85:AD:35:E2",
    "CD:8D:E6:9E:FB:E6"
};

const char* const POS_NAMES[4] = {
    "Front Left",
    "Front Right",
    "Rear Left",
    "Rear Right"
};

#endif // CONFIG_C3_H
