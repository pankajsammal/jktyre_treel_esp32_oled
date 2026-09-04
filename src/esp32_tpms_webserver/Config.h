#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================================
// CENTRAL USER CONFIGURATION FILE (ESP32 / ESP32-C3)
// =====================================================================
// Edit your Wi-Fi credentials, sensor MACs, alert thresholds, hardware
// pins, display units, and feature switches below.
// =====================================================================

// ---------------------------------------------------------------------
// 1. FEATURE & TEST MODE SWITCHES
// ---------------------------------------------------------------------
#define ENABLE_WEBSERVER     true    // Set to false to disable Wi-Fi and Web Server (Pure BLE / Ultra Low Power)
#define ENABLE_OLED          true    // Set to true if an I2C OLED screen is physically attached
#define ENABLE_DEMO_MODE     false   // Set to true to test OLED & Web Server with dummy values & warnings

// ---------------------------------------------------------------------
// 2. DISPLAY UNIT PREFERENCES (OLED Display)
// ---------------------------------------------------------------------
#define UNIT_PSI             0      // Pressure in PSI (e.g. 32 PSI)
#define UNIT_BAR             1      // Pressure in Bar (e.g. 2.2 Bar)
#define UNIT_KPA             2      // Pressure in kPa (e.g. 220 kPa)

#define UNIT_CELSIUS         0      // Temperature in Celsius (°C)
#define UNIT_FAHRENHEIT      1      // Temperature in Fahrenheit (°F)

#define DISPLAY_PRESSURE_UNIT UNIT_PSI      // Selected pressure unit: UNIT_PSI, UNIT_BAR, or UNIT_KPA
#define DISPLAY_TEMP_UNIT     UNIT_CELSIUS  // Selected temperature unit: UNIT_CELSIUS or UNIT_FAHRENHEIT

// ---------------------------------------------------------------------
// 3. ALERT & WARNING THRESHOLDS
// ---------------------------------------------------------------------
#define ALERT_MIN_PSI        26.0f  // Trigger Low Pressure alert if pressure drops below this (PSI)
#define ALERT_MAX_PSI        40.0f  // Trigger High Pressure alert if pressure rises above this (PSI)
#define ALERT_MAX_TEMP_C     70.0f  // Trigger High Temperature alert if temp exceeds this (°C)
#define ALERT_MIN_BATT       15     // Trigger Low Battery alert if battery drops below this (%)

// ---------------------------------------------------------------------
// 4. HARDWARE PINOUT (Auto-detects ESP32-C3 vs Standard ESP32 DevKit)
// ---------------------------------------------------------------------
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
#define OLED_SDA_PIN         8      // ESP32-C3 SuperMini I2C SDA (GPIO 8)
#define OLED_SCL_PIN         9      // ESP32-C3 SuperMini I2C SCL (GPIO 9)
#else
#define OLED_SDA_PIN         14     // Standard ESP32 DevKit I2C SDA (GPIO 14)
#define OLED_SCL_PIN         27     // Standard ESP32 DevKit I2C SCL (GPIO 27)
#endif
#define OLED_VCC_PIN         -1     // Dedicated VCC pin (-1 if wired directly to 3.3V)
#define USE_SH1106_1_3_INCH  1      // 1 for 1.3" SH1106 display, 0 for 0.96" SSD1306 display

// ---------------------------------------------------------------------
// 5. WI-FI NETWORK CONFIGURATION
// ---------------------------------------------------------------------
#if ENABLE_WEBSERVER
const char* const WIFI_SSID     = "Your_WiFi_SSID";     // Your home or vehicle Wi-Fi router SSID
const char* const WIFI_PASS     = "Your_WiFi_Password"; // Your Wi-Fi password
const bool        TRY_STA_FIRST = true;                  // Connect to router Wi-Fi first
const int         STA_TIMEOUT_S = 8;                     // Fast fallback timeout

// Fallback Access Point (AP) Settings (used if Wi-Fi router is unreachable)
const char* const AP_SSID       = "ESP32_TPMS_Dashboard";
const char* const AP_PASS       = "12345678";            // AP password (minimum 8 characters)
#endif

// ---------------------------------------------------------------------
// 6. SENSOR WHITELIST & MAC ADDRESSES
// ---------------------------------------------------------------------
// IMPORTANT: Replace these MAC addresses and Short IDs with your own TPMS sensor MACs!
// Find your MAC addresses in the official JK Tyre SmartTyre app under Settings -> Sensor Debug.
const char* const SENSOR_MACS[4] = {
    "D2:58:6D:8F:16:10",  // FL: Front Left (Replace with your sensor MAC)
    "CA:E8:6C:2D:92:15",  // FR: Front Right (Replace with your sensor MAC)
    "F7:FC:85:AD:35:E2",  // RL: Rear Left (Replace with your sensor MAC)
    "CD:8D:E6:9E:FB:E6"   // RR: Rear Right (Replace with your sensor MAC)
};

const char* const SENSOR_SHORT_IDS[4] = {
    "8F1610",  // FL Short ID (Last 6 characters of Forward MAC)
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
