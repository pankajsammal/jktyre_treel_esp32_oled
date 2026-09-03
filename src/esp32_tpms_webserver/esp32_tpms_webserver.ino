// =====================================================================
// ESP32 Treel TPMS BLE Receiver & Remote Web Server Dashboard
// =====================================================================
// Matches Raspberry Pi / Desktop Dual-Mode Decoding Architecture:
// 1. Apple iBeacon Broadcast Mode (...FFE0 UUID)
// 2. Hardware-Accelerated AES-128-ECB GATT Mode (mbedTLS)
// 3. Dual-Endian MAC Matching (Forward MAC, Reversed MAC & Short ID)
// 4. Wi-Fi Station (STA) with automatic Access Point (AP) Fallback
// 5. Real-Time Responsive Web Dashboard (AJAX live polling 1Hz) & JSON API
//
// REQUIRED ARDUINO LIBRARIES:
// - "NimBLE-Arduino" by h2zero (Tools -> Manage Libraries -> search "NimBLE-Arduino")
// - "U8g2" by Oliver Kraus (only if ENABLE_OLED is set to true)
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NimBLEDevice.h>
#include "mbedtls/aes.h"

// =====================================================================
// 1. HARDWARE PIN CONFIGURATION & BOARD AUTO-DETECTION
// =====================================================================
#define ENABLE_WEBSERVER     true   // Set to false to disable Wi-Fi & Web Server completely
#define ENABLE_OLED          false  // Set to false if running headless without OLED

#if defined(CONFIG_IDF_TARGET_ESP32C3)
// --- ESP32-C3 SuperMini Configuration ---
// Pinout: 3.3V, GND, SDA (GPIO 8), SCL (GPIO 9)
#define OLED_SDA_PIN         8     // ESP32-C3 I2C SDA (GPIO 8)
#define OLED_SCL_PIN         9     // ESP32-C3 I2C SCL (GPIO 9)
#define OLED_VCC_PIN         -1    // Wire to dedicated 3.3V pin
#else
// --- Standard ESP32 (38-Pin / 30-Pin DevKit) ---
// Left-Side Pinout: 3.3V (Pin 1), GND (Pin 14), SDA (GPIO 14 / Pin 12), SCL (GPIO 27 / Pin 11)
#define OLED_SDA_PIN         14    // ESP32 GPIO 14 (Pin 12)
#define OLED_SCL_PIN         27    // ESP32 GPIO 27 (Pin 11)
#define OLED_VCC_PIN         -1    // Wire to dedicated 3.3V (Pin 1)
#endif

// Display Driver Selection:
// - 1.3" OLED (SH1106): U8G2_SH1106_128X64_NONAME_F_HW_I2C (Default)
// - 0.96" OLED (SSD1306): U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#define USE_SH1106_1_3_INCH  1     // 1 for 1.3" SH1106, 0 for 0.96" SSD1306

// =====================================================================
// 2. CONFIGURATION & SENSOR WHITELIST
// =====================================================================
#if ENABLE_WEBSERVER
// --- Wi-Fi Settings ---
const char* WIFI_SSID     = "Your_WiFi_SSID";     // Replace with your home/car Wi-Fi SSID
const char* WIFI_PASS     = "Your_WiFi_Password"; // Replace with your Wi-Fi Password
const bool  TRY_STA_FIRST = true;                  // Try connecting to Wi-Fi first
const int   STA_TIMEOUT_S = 10;                    // Seconds before falling back to AP

// --- Fallback Access Point (AP) Settings ---
const char* AP_SSID       = "ESP32_TPMS_Dashboard";
const char* AP_PASS       = "12345678";            // Minimum 8 chars
#endif

// --- 4 Whitelisted TPMS Sensors ---
enum TirePosition {
    POS_FL = 0,
    POS_FR = 1,
    POS_RL = 2,
    POS_RR = 3,
    POS_UNKNOWN = 4
};

// IMPORTANT: Replace these sample MAC addresses and 6-character Short IDs with your own TPMS sensor MACs!
// Find your MAC addresses in the official JK Tyre SmartTyre app under Settings -> Sensor Debug.
const char* SENSOR_MACS[4] = {
    "D2:58:6D:8F:16:10",  // FL: Front Left (Replace with your sensor MAC)
    "CA:E8:6C:2D:92:15",  // FR: Front Right (Replace with your sensor MAC)
    "F7:FC:85:AD:35:E2",  // RL: Rear Left (Replace with your sensor MAC)
    "CD:8D:E6:9E:FB:E6"   // RR: Rear Right (Replace with your sensor MAC)
};

const char* SENSOR_SHORT_IDS[4] = {
    "8F1610",  // FL Short ID (Last 6 hex characters of MAC)
    "2D9215",  // FR Short ID
    "AD35E2",  // RL Short ID
    "9EFBE6"   // RR Short ID
};

const char* POS_NAMES[4] = {
    "Front Left",
    "Front Right",
    "Rear Left",
    "Rear Right"
};

// Alert Thresholds
#define ALERT_MIN_PSI     26.0f
#define ALERT_MAX_PSI     45.0f
#define ALERT_MAX_TEMP_C  70.0f
#define ALERT_MIN_BATT    15

// Constants
static const uint8_t AES_KEY[16] = {
    '#', '@', 'T', 'r', 'l', '2', '0', '1', '8', '-', 'l', 'e', 's', 'p', 'l', '$'
};

static const uint8_t TREEL_BEACON_UUID[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0
};

// =====================================================================
// 2. DATA STRUCTURES & ROLLING LOGS
// =====================================================================
enum AlertState {
    ALERT_NORMAL = 0,
    ALERT_LOW_PRESSURE,
    ALERT_HIGH_PRESSURE,
    ALERT_HIGH_TEMP,
    ALERT_LOW_BATT,
    ALERT_WAITING
};

struct TireData {
    String mac;
    String sensor_id;
    TirePosition position;
    float pressure_psi = 0.0f;
    float pressure_bar = 0.0f;
    float pressure_kpa = 0.0f;
    float temperature_c = 0.0f;
    float temperature_f = 32.0f;
    int battery_percent = -1;
    int rssi = -100;
    String mode = "--";
    unsigned long last_updated_ms = 0;
    bool has_received = false;

    void update(float psi, float tempC, int batt, int sigRssi, const String& decMode, const String& sId) {
        pressure_psi = psi;
        pressure_bar = psi * 0.0689476f;
        pressure_kpa = psi * 6.89476f;
        temperature_c = tempC;
        temperature_f = (tempC * 9.0f / 5.0f) + 32.0f;
        if (batt >= 0) battery_percent = batt; // Retain battery level across beacon frames
        rssi = sigRssi;
        mode = decMode;
        if (sId.length() > 0) sensor_id = sId;
        last_updated_ms = millis();
        has_received = true;
    }

    AlertState getAlertState() const {
        if (!has_received) return ALERT_WAITING;
        if (pressure_psi < ALERT_MIN_PSI) return ALERT_LOW_PRESSURE;
        if (pressure_psi > ALERT_MAX_PSI) return ALERT_HIGH_PRESSURE;
        if (temperature_c > ALERT_MAX_TEMP_C) return ALERT_HIGH_TEMP;
        if (battery_percent >= 0 && battery_percent < ALERT_MIN_BATT) return ALERT_LOW_BATT;
        return ALERT_NORMAL;
    }

    const char* getAlertString() const {
        switch (getAlertState()) {
            case ALERT_LOW_PRESSURE:  return "LOW_PRESSURE";
            case ALERT_HIGH_PRESSURE: return "HIGH_PRESSURE";
            case ALERT_HIGH_TEMP:     return "HIGH_TEMP";
            case ALERT_LOW_BATT:      return "LOW_BATTERY";
            case ALERT_NORMAL:        return "NORMAL";
            default:                  return "WAITING";
        }
    }
};

TireData g_tires[4];
volatile uint32_t g_totalBlePackets = 0;
volatile uint32_t g_tpmsPackets = 0;
String g_wifiModeStr = "Disconnected";
String g_ipAddress = "0.0.0.0";

// Rolling Log Buffer (In-memory ring buffer)
#define MAX_LOG_ENTRIES 40
struct LogEntry {
    uint32_t timestamp_s;
    char text[120];
};
LogEntry g_logs[MAX_LOG_ENTRIES];
int g_logHead = 0;
int g_logCount = 0;
portMUX_TYPE g_logMux = portMUX_INITIALIZER_UNLOCKED;

void addSystemLog(const char* format, ...) {
    char buf[120];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.println(buf);

    portENTER_CRITICAL(&g_logMux);
    g_logs[g_logHead].timestamp_s = millis() / 1000;
    strncpy(g_logs[g_logHead].text, buf, sizeof(g_logs[g_logHead].text) - 1);
    g_logs[g_logHead].text[sizeof(g_logs[g_logHead].text) - 1] = '\0';
    g_logHead = (g_logHead + 1) % MAX_LOG_ENTRIES;
    if (g_logCount < MAX_LOG_ENTRIES) g_logCount++;
    portEXIT_CRITICAL(&g_logMux);
}

// =====================================================================
// OLED DISPLAY RENDERER (128x64 I2C SH1106 / SSD1306)
// Option 1: 4-Quadrant Precision Grid
// =====================================================================
class TPMSDisplay {
private:
#if USE_SH1106_1_3_INCH
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
#else
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
#endif
    bool initialized = false;

    static String formatAge(unsigned long last_ms, unsigned long now_ms) {
        if (last_ms == 0) return "WAIT";
        unsigned long diff = (now_ms - last_ms) / 1000;
        if (diff < 60) return String(diff) + "s";
        if (diff < 3600) return String(diff / 60) + "m";
        if (diff < 86400) return String(diff / 3600) + "h";
        return String(diff / 86400) + "d";
    }

public:
#if USE_SH1106_1_3_INCH
    TPMSDisplay() : u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}
#else
    TPMSDisplay() : u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}
#endif

    void begin() {
        if (!ENABLE_OLED) return;

        if (OLED_VCC_PIN >= 0) {
            pinMode(OLED_VCC_PIN, OUTPUT);
            digitalWrite(OLED_VCC_PIN, HIGH);
            delay(50); // Allow OLED power rail to stabilize
        }

        Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
        u8g2.begin();
        u8g2.clearBuffer();

        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(14, 22, "TREEL TPMS BLE");
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(18, 40, "4-TIRE MONITOR");
        u8g2.setFont(u8g2_font_micro_tr);
        u8g2.drawStr(24, 58, "INITIALIZING...");
        u8g2.sendBuffer();
        initialized = true;
    }

    void render(const TireData tires[4]) {
        if (!ENABLE_OLED || !initialized) return;

        u8g2.clearBuffer();
        unsigned long now_ms = millis();

        // 1. Draw Center Dividers (Chassis Crosshair)
        u8g2.setDrawColor(1);
        u8g2.drawVLine(63, 0, 64);
        u8g2.drawHLine(0, 31, 128);

        // 2. Render 4 Quadrants
        renderCard(tires[POS_FL], "FL", 0, 0, now_ms);
        renderCard(tires[POS_FR], "FR", 64, 0, now_ms);
        renderCard(tires[POS_RL], "RL", 0, 32, now_ms);
        renderCard(tires[POS_RR], "RR", 64, 32, now_ms);

        u8g2.sendBuffer();
    }

private:
    void renderCard(const TireData& tire, const char* posLabel, int x, int y, unsigned long now_ms) {
        bool has_data = tire.has_received;
        AlertState alert = tire.getAlertState();
        bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

        if (is_alert) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y, 63, 31);
            u8g2.setDrawColor(0); // Inverted text color
        } else {
            u8g2.setDrawColor(1);
        }

        // 1. Top Row: Position (FL) on left, Elapsed Age (12s / 2m) on right
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(x + 2, y + 9, posLabel);

        String ageStr = has_data ? formatAge(tire.last_updated_ms, now_ms) : "WAIT";
        u8g2.setFont(u8g2_font_5x8_tr);
        int ageWidth = u8g2.getStrWidth(ageStr.c_str());
        u8g2.drawStr(x + 61 - ageWidth, y + 8, ageStr.c_str());

        if (!has_data) {
            u8g2.setFont(u8g2_font_7x14B_tr);
            u8g2.drawStr(x + 8, y + 21, "--.- P");
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(x + 2, y + 30, "-- C");
            u8g2.drawStr(x + 44, y + 30, "WAIT");
            return;
        }

        // 2. Middle Row: Extra-Large PSI Digits
        char psiBuf[10];
        snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_psi);
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(x + 6, y + 21, psiBuf);

        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(x + 44, y + 18, "P");

        // 3. Bottom Row: Temperature (Celsius) on left, Status on right
        char tempBuf[10];
        snprintf(tempBuf, sizeof(tempBuf), "%.0fC", tire.temperature_c);
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(x + 2, y + 30, tempBuf);

        if (is_alert) {
            u8g2.drawStr(x + 36, y + 30, "!ALT");
        } else {
            u8g2.drawStr(x + 46, y + 30, "OK");
        }
    }
};

TPMSDisplay g_display;

class TreelDecoder {
private:
    static mbedtls_aes_context s_aesCtx;
    static bool s_aesInit;

public:
    static void initAES() {
        if (!s_aesInit) {
            mbedtls_aes_init(&s_aesCtx);
            mbedtls_aes_setkey_dec(&s_aesCtx, AES_KEY, 128);
            s_aesInit = true;
        }
    }

    static bool decryptAES128(const uint8_t* ciphertext, uint8_t* plaintext) {
        if (!s_aesInit) initAES();
        return (mbedtls_aes_crypt_ecb(&s_aesCtx, MBEDTLS_AES_DECRYPT, ciphertext, plaintext) == 0);
    }

    static bool decodeGATT(const uint8_t* payload, size_t len, const String& mac, int rssi, TireData& outReading) {
        if (!payload || len < 16) return false;

        uint8_t decrypted[16];
        for (size_t offset = 0; offset <= len - 16; offset++) {
            if (!decryptAES128(payload + offset, decrypted)) continue;

            // Treel Encrypted Mode tag MUST be 0x16 (decimal 22)
            uint8_t dataType = decrypted[0];
            if (dataType != 0x16) continue;

            // Temperature (Bytes 1-2, Little Endian)
            uint16_t rawTemp = decrypted[1] | (decrypted[2] << 8);
            if (rawTemp == 65535) continue;
            float tempC = 0.0f;
            if (rawTemp <= 32768) {
                tempC = rawTemp / 100.0f;
            } else {
                tempC = -((rawTemp - 32768) / 100.0f);
            }

            // Pressure (Bytes 3-4, Little Endian PSI x 100)
            uint16_t rawPress = decrypted[3] | (decrypted[4] << 8);
            if (rawPress == 65535) continue;
            float pressPsi = rawPress / 100.0f;

            // Battery (Byte 5)
            uint8_t batt = decrypted[5];

            // Validation Sanity Check
            if (tempC >= -40.0f && tempC <= 125.0f && pressPsi >= 0.0f && pressPsi <= 217.0f && batt <= 100) {
                String cleanMac = mac;
                cleanMac.replace(":", "");
                cleanMac.replace("-", "");
                String sId = (cleanMac.length() >= 6) ? cleanMac.substring(cleanMac.length() - 6) : "TREEL";

                outReading.update(pressPsi, tempC, batt, rssi, "GATT/AES", sId);
                outReading.mac = mac;
                return true;
            }
        }
        return false;
    }

    static bool decodeBeacon(const uint8_t* payload, size_t len, const String& mac, int rssi, TireData& outReading) {
        if (!payload || len < 23) return false;

        for (size_t offset = 0; offset <= len - 23; offset++) {
            if (payload[offset] == 0x02 && payload[offset + 1] == 0x15) {
                // Verify Treel 16-byte UUID ending in 0xE0
                bool match = true;
                for (int i = 0; i < 16; i++) {
                    if (payload[offset + 2 + i] != TREEL_BEACON_UUID[i]) {
                        match = false;
                        break;
                    }
                }
                if (!match) continue;

                uint16_t major = (payload[offset + 18] << 8) | payload[offset + 19];
                uint16_t minor = (payload[offset + 20] << 8) | payload[offset + 21];
                uint8_t txTemp = payload[offset + 22];

                float pressPsi = (float)(minor & 0xFF);
                float tempC = 0.0f;
                if (txTemp > 65) {
                    tempC = (float)((int)txTemp - 110);
                }

                if (pressPsi >= 0.0f && pressPsi <= 217.0f && tempC >= -40.0f && tempC <= 125.0f) {
                    char idBuf[16];
                    snprintf(idBuf, sizeof(idBuf), "%04X-%04X", major, minor);

                    outReading.update(pressPsi, tempC, -1, rssi, "iBeacon", String(idBuf));
                    outReading.mac = mac;
                    return true;
                }
            }
        }
        return false;
    }

    // Resolve sensor position matching Forward MAC, Reversed MAC, Short ID, or Hex Payload
    static TirePosition resolvePosition(const String& macFwd, const String& macRev, const String& payloadHex, const String& sensorId = "") {
        String cleanFwd = macFwd; cleanFwd.replace(":", ""); cleanFwd.replace("-", ""); cleanFwd.toUpperCase();
        String cleanRev = macRev; cleanRev.replace(":", ""); cleanRev.replace("-", ""); cleanRev.toUpperCase();
        String cleanHex = payloadHex; cleanHex.replace(" ", ""); cleanHex.toUpperCase();
        String cleanId  = sensorId; cleanId.replace(":", ""); cleanId.replace("-", ""); cleanId.toUpperCase();

        for (int i = 0; i < 4; i++) {
            String targetMac = SENSOR_MACS[i];
            targetMac.replace(":", "");
            targetMac.toUpperCase();
            String targetShort = SENSOR_SHORT_IDS[i];
            targetShort.toUpperCase();

            // 1. Direct or Reversed MAC match
            if (cleanFwd.indexOf(targetMac) >= 0 || cleanRev.indexOf(targetMac) >= 0) {
                return (TirePosition)i;
            }

            // 2. Short ID match in MAC, decoded ID, or raw hex
            if (targetShort.length() >= 4) {
                if (cleanFwd.indexOf(targetShort) >= 0 || cleanRev.indexOf(targetShort) >= 0 ||
                    cleanId.indexOf(targetShort) >= 0  || cleanHex.indexOf(targetShort) >= 0) {
                    return (TirePosition)i;
                }
            }
        }
        return POS_UNKNOWN;
    }
};

mbedtls_aes_context TreelDecoder::s_aesCtx;
bool TreelDecoder::s_aesInit = false;

// =====================================================================
// 4. NIMBLE SCAN CALLBACKS (High-Speed & Zero Packet Loss)
// =====================================================================
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
        if (!advertisedDevice) return;
        g_totalBlePackets++;

        // 1. Extract Addresses (Forward & Reversed)
        String macFwd = advertisedDevice->getAddress().toString().c_str();
        macFwd.toUpperCase();

        String macRev = "";
        if (macFwd.length() == 17) {
            macRev += macFwd.substring(15, 17) + ":";
            macRev += macFwd.substring(12, 14) + ":";
            macRev += macFwd.substring(9, 11) + ":";
            macRev += macFwd.substring(6, 8) + ":";
            macRev += macFwd.substring(3, 5) + ":";
            macRev += macFwd.substring(0, 2);
        } else {
            macRev = macFwd;
        }

        int rssi = advertisedDevice->getRSSI();

        // 2. Extract Raw Payload & Manufacturer Data
        const std::vector<uint8_t>& payloadVec = advertisedDevice->getPayload();
        const uint8_t* payload = payloadVec.data();
        size_t payloadLen = payloadVec.size();

        std::string mfgStr = advertisedDevice->getManufacturerData();
        const uint8_t* mfgData = (const uint8_t*)mfgStr.data();
        size_t mfgLen = mfgStr.length();

        // Convert payload to Hex string
        String payloadHex = "";
        for (size_t i = 0; i < payloadLen; i++) {
            char hBuf[3];
            snprintf(hBuf, sizeof(hBuf), "%02X", payload[i]);
            payloadHex += hBuf;
        }

        // 3. Attempt Telemetry Decoding (Manufacturer Data -> Raw Payload)
        TireData reading;
        bool decoded = false;

        if (mfgData && mfgLen >= 16) {
            decoded = TreelDecoder::decodeBeacon(mfgData, mfgLen, macFwd, rssi, reading) ||
                      TreelDecoder::decodeGATT(mfgData, mfgLen, macFwd, rssi, reading);
        }
        if (!decoded && payload && payloadLen >= 16) {
            decoded = TreelDecoder::decodeBeacon(payload, payloadLen, macFwd, rssi, reading) ||
                      TreelDecoder::decodeGATT(payload, payloadLen, macFwd, rssi, reading);
        }

        // 4. Resolve Wheel Position
        TirePosition pos = TreelDecoder::resolvePosition(macFwd, macRev, payloadHex, reading.sensor_id);

        if (pos < POS_UNKNOWN) {
            g_tpmsPackets++;
            const char* posAbbr[] = {"FL", "FR", "RL", "RR"};

            if (decoded) {
                g_tires[pos].update(
                    reading.pressure_psi,
                    reading.temperature_c,
                    reading.battery_percent,
                    rssi,
                    reading.mode,
                    reading.sensor_id
                );

                addSystemLog("[TPMS-%s] %s | %.1f PSI (%.2f Bar) | %.1f C | Batt: %d%% | %s | RSSI: %d dBm",
                             posAbbr[pos], macFwd.c_str(), g_tires[pos].pressure_psi, g_tires[pos].pressure_bar,
                             g_tires[pos].temperature_c, g_tires[pos].battery_percent, reading.mode.c_str(), rssi);
            }
        }
    }
};

// =====================================================================
// 5. EMBEDDED HTTP WEB SERVER & REST API
// =====================================================================
#if ENABLE_WEBSERVER
WebServer server(80);

// HTML Dashboard UI (Stored in PROGMEM)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Treel TPMS Monitor</title>
    <style>
        :root {
            --bg-base: #0b0f19;
            --bg-card: #151d2f;
            --card-border: #1e293b;
            --text-main: #f8fafc;
            --text-dim: #94a3b8;
            --accent: #0284c7;
            --accent-cyan: #38bdf8;
            --alert-red: #ef4444;
            --alert-yellow: #f59e0b;
            --alert-green: #10b981;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-base);
            color: var(--text-main);
            padding: 16px;
            display: flex;
            justify-content: center;
        }
        .container {
            width: 100%;
            max-width: 900px;
        }
        header {
            text-align: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid var(--card-border);
        }
        h1 {
            color: var(--accent-cyan);
            font-size: 1.6rem;
            font-weight: 700;
            letter-spacing: 0.5px;
        }
        .meta-bar {
            margin-top: 6px;
            font-size: 0.85rem;
            color: var(--text-dim);
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 16px;
        }
        .badge {
            background: #1e293b;
            padding: 2px 8px;
            border-radius: 4px;
            border: 1px solid #334155;
            color: #e2e8f0;
        }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 14px;
            margin-bottom: 20px;
        }
        @media (max-width: 600px) {
            .grid { grid-template-columns: 1fr; }
        }
        .card {
            background: var(--bg-card);
            border: 1px solid var(--card-border);
            border-radius: 12px;
            padding: 16px;
            position: relative;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            transition: border-color 0.3s ease;
        }
        .card.alert-low, .card.alert-high, .card.alert-temp {
            border-color: var(--alert-red);
            box-shadow: 0 0 16px rgba(239, 68, 68, 0.25);
        }
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }
        .pos-title {
            font-size: 1.1rem;
            font-weight: 700;
            color: var(--accent-cyan);
        }
        .status-badge {
            font-size: 0.75rem;
            font-weight: 600;
            padding: 3px 8px;
            border-radius: 12px;
            background: #334155;
            color: #94a3b8;
        }
        .status-badge.normal { background: rgba(16, 185, 129, 0.2); color: var(--alert-green); border: 1px solid var(--alert-green); }
        .status-badge.alert  { background: rgba(239, 68, 68, 0.2); color: var(--alert-red); border: 1px solid var(--alert-red); }
        .press-hero {
            display: flex;
            align-items: baseline;
            gap: 8px;
            margin: 8px 0;
        }
        .press-val {
            font-size: 2.6rem;
            font-weight: 800;
            color: #ffffff;
            line-height: 1;
        }
        .press-unit {
            font-size: 1.1rem;
            font-weight: 600;
            color: var(--accent-cyan);
        }
        .press-bar {
            font-size: 0.9rem;
            color: var(--text-dim);
        }
        .metrics-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            margin-top: 12px;
            padding-top: 10px;
            border-top: 1px solid #1e293b;
            font-size: 0.85rem;
        }
        .metric-item span {
            color: var(--text-dim);
            font-size: 0.75rem;
            display: block;
        }
        .metric-item strong {
            color: #f1f5f9;
            font-size: 0.95rem;
        }
        .card-footer {
            margin-top: 10px;
            font-size: 0.75rem;
            color: #64748b;
            display: flex;
            justify-content: space-between;
        }
        .logs-section {
            background: var(--bg-card);
            border: 1px solid var(--card-border);
            border-radius: 12px;
            padding: 16px;
        }
        .logs-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 8px;
        }
        .logs-header h3 {
            font-size: 1rem;
            color: var(--accent-cyan);
        }
        .terminal {
            background: #060911;
            border: 1px solid #1e293b;
            border-radius: 6px;
            padding: 10px;
            height: 180px;
            overflow-y: auto;
            font-family: 'Courier New', Courier, monospace;
            font-size: 0.78rem;
            line-height: 1.4;
            color: #38bdf8;
        }
        .terminal div { margin-bottom: 2px; }
        .btn {
            background: var(--accent);
            color: #fff;
            border: none;
            padding: 4px 10px;
            border-radius: 4px;
            font-size: 0.75rem;
            cursor: pointer;
        }
        .btn:hover { background: #0284c7; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>TREEL TPMS BLE MONITOR</h1>
            <div class="meta-bar">
                <span>Wi-Fi: <strong id="wifi-mode">--</strong></span>
                <span>IP: <strong id="ip-addr">--</strong></span>
                <span>Uptime: <strong id="uptime">0s</strong></span>
                <span>Total Packets: <strong id="total-pkts">0</strong></span>
            </div>
        </header>

        <div class="grid">
            <!-- Front Left -->
            <div class="card" id="card-FL">
                <div class="card-header">
                    <span class="pos-title">FL - Front Left</span>
                    <span class="status-badge" id="badge-FL">WAITING</span>
                </div>
                <div class="press-hero">
                    <div class="press-val" id="psi-FL">--</div>
                    <div class="press-unit">PSI</div>
                    <div class="press-bar" id="bar-FL">(-- Bar)</div>
                </div>
                <div class="metrics-grid">
                    <div class="metric-item"><span>TEMPERATURE</span><strong id="temp-FL">-- &deg;C</strong></div>
                    <div class="metric-item"><span>BATTERY</span><strong id="batt-FL">--%</strong></div>
                    <div class="metric-item"><span>MODE / ID</span><strong id="mode-FL">--</strong></div>
                    <div class="metric-item"><span>SIGNAL</span><strong id="rssi-FL">-- dBm</strong></div>
                </div>
                <div class="card-footer">
                    <span id="mac-FL">MAC: Configured</span>
                    <span id="age-FL">Last: Never</span>
                </div>
            </div>

            <!-- Front Right -->
            <div class="card" id="card-FR">
                <div class="card-header">
                    <span class="pos-title">FR - Front Right</span>
                    <span class="status-badge" id="badge-FR">WAITING</span>
                </div>
                <div class="press-hero">
                    <div class="press-val" id="psi-FR">--</div>
                    <div class="press-unit">PSI</div>
                    <div class="press-bar" id="bar-FR">(-- Bar)</div>
                </div>
                <div class="metrics-grid">
                    <div class="metric-item"><span>TEMPERATURE</span><strong id="temp-FR">-- &deg;C</strong></div>
                    <div class="metric-item"><span>BATTERY</span><strong id="batt-FR">--%</strong></div>
                    <div class="metric-item"><span>MODE / ID</span><strong id="mode-FR">--</strong></div>
                    <div class="metric-item"><span>SIGNAL</span><strong id="rssi-FR">-- dBm</strong></div>
                </div>
                <div class="card-footer">
                    <span id="mac-FR">MAC: Configured</span>
                    <span id="age-FR">Last: Never</span>
                </div>
            </div>

            <!-- Rear Left -->
            <div class="card" id="card-RL">
                <div class="card-header">
                    <span class="pos-title">RL - Rear Left</span>
                    <span class="status-badge" id="badge-RL">WAITING</span>
                </div>
                <div class="press-hero">
                    <div class="press-val" id="psi-RL">--</div>
                    <div class="press-unit">PSI</div>
                    <div class="press-bar" id="bar-RL">(-- Bar)</div>
                </div>
                <div class="metrics-grid">
                    <div class="metric-item"><span>TEMPERATURE</span><strong id="temp-RL">-- &deg;C</strong></div>
                    <div class="metric-item"><span>BATTERY</span><strong id="batt-RL">--%</strong></div>
                    <div class="metric-item"><span>MODE / ID</span><strong id="mode-RL">--</strong></div>
                    <div class="metric-item"><span>SIGNAL</span><strong id="rssi-RL">-- dBm</strong></div>
                </div>
                <div class="card-footer">
                    <span id="mac-RL">MAC: Configured</span>
                    <span id="age-RL">Last: Never</span>
                </div>
            </div>

            <!-- Rear Right -->
            <div class="card" id="card-RR">
                <div class="card-header">
                    <span class="pos-title">RR - Rear Right</span>
                    <span class="status-badge" id="badge-RR">WAITING</span>
                </div>
                <div class="press-hero">
                    <div class="press-val" id="psi-RR">--</div>
                    <div class="press-unit">PSI</div>
                    <div class="press-bar" id="bar-RR">(-- Bar)</div>
                </div>
                <div class="metrics-grid">
                    <div class="metric-item"><span>TEMPERATURE</span><strong id="temp-RR">-- &deg;C</strong></div>
                    <div class="metric-item"><span>BATTERY</span><strong id="batt-RR">--%</strong></div>
                    <div class="metric-item"><span>MODE / ID</span><strong id="mode-RR">--</strong></div>
                    <div class="metric-item"><span>SIGNAL</span><strong id="rssi-RR">-- dBm</strong></div>
                </div>
                <div class="card-footer">
                    <span id="mac-RR">MAC: Configured</span>
                    <span id="age-RR">Last: Never</span>
                </div>
            </div>
        </div>

        <!-- System & BLE Packet Logs -->
        <div class="logs-section">
            <div class="logs-header">
                <h3>Live BLE Packet Stream</h3>
                <button class="btn" onclick="clearLogs()">Clear</button>
            </div>
            <div class="terminal" id="terminal">Loading telemetry stream...</div>
        </div>
    </div>

    <script>
        function formatAge(ageSec) {
            if (ageSec < 0) return "Never";
            if (ageSec < 60) return ageSec + "s ago";
            if (ageSec < 3600) return Math.floor(ageSec / 60) + "m ago";
            return Math.floor(ageSec / 3600) + "h ago";
        }

        async function fetchTPMS() {
            try {
                const res = await fetch('/api/data');
                if (!res.ok) return;
                const data = await res.json();

                document.getElementById('wifi-mode').innerText = data.system.wifi_mode;
                document.getElementById('ip-addr').innerText = data.system.ip;
                document.getElementById('uptime').innerText = data.system.uptime_s + 's';
                document.getElementById('total-pkts').innerText = data.system.total_ble + ' (' + data.system.tpms_pkts + ' TPMS)';

                const positions = ['FL', 'FR', 'RL', 'RR'];
                positions.forEach(pos => {
                    const tire = data.tires[pos];
                    if (!tire) return;

                    const card = document.getElementById('card-' + pos);
                    const badge = document.getElementById('badge-' + pos);

                    if (tire.has_data) {
                        document.getElementById('psi-' + pos).innerText = tire.psi.toFixed(1);
                        document.getElementById('bar-' + pos).innerText = '(' + tire.bar.toFixed(2) + ' Bar)';
                        document.getElementById('temp-' + pos).innerHTML = tire.temp_c.toFixed(1) + ' &deg;C / ' + tire.temp_f.toFixed(0) + ' &deg;F';
                        document.getElementById('batt-' + pos).innerText = tire.battery >= 0 ? tire.battery + '%' : 'N/A';
                        document.getElementById('mode-' + pos).innerText = tire.mode + ' (' + (tire.sensor_id || 'ID') + ')';
                        document.getElementById('rssi-' + pos).innerText = tire.rssi + ' dBm';
                        document.getElementById('mac-' + pos).innerText = 'MAC: ' + tire.mac;
                        document.getElementById('age-' + pos).innerText = 'Last: ' + formatAge(tire.age_s);

                        badge.innerText = tire.alert;
                        badge.className = 'status-badge ' + (tire.alert === 'NORMAL' ? 'normal' : 'alert');

                        card.className = 'card ' + (tire.alert !== 'NORMAL' ? 'alert-low' : '');
                    } else {
                        badge.innerText = 'WAITING';
                        badge.className = 'status-badge';
                    }
                });
            } catch (e) {
                console.error("Fetch error", e);
            }
        }

        async function fetchLogs() {
            try {
                const res = await fetch('/api/logs');
                if (!res.ok) return;
                const logs = await res.json();
                const term = document.getElementById('terminal');
                term.innerHTML = logs.map(l => `<div>[+${l.t}s] ${l.msg}</div>`).join('');
                term.scrollTop = term.scrollHeight;
            } catch (e) {}
        }

        async function clearLogs() {
            await fetch('/api/clear');
            fetchLogs();
        }

        setInterval(fetchTPMS, 1000);
        setInterval(fetchLogs, 2000);
        fetchTPMS();
        fetchLogs();
    </script>
</body>
</html>
)rawliteral";

// Handle Root URL
void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

// Handle /api/data Endpoint (Live Telemetry JSON)
void handleApiData() {
    uint32_t nowMs = millis();
    uint32_t uptimeS = nowMs / 1000;

    String json = "{";
    json += "\"system\":{";
    json += "\"uptime_s\":" + String(uptimeS) + ",";
    json += "\"wifi_mode\":\"" + g_wifiModeStr + "\",";
    json += "\"ip\":\"" + g_ipAddress + "\",";
    json += "\"total_ble\":" + String(g_totalBlePackets) + ",";
    json += "\"tpms_pkts\":" + String(g_tpmsPackets) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "},";

    json += "\"tires\":{";
    const char* keys[] = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; i++) {
        int ageS = g_tires[i].has_received ? (int)((nowMs - g_tires[i].last_updated_ms) / 1000) : -1;

        json += "\"" + String(keys[i]) + "\":{";
        json += "\"has_data\":" + String(g_tires[i].has_received ? "true" : "false") + ",";
        json += "\"name\":\"" + String(POS_NAMES[i]) + "\",";
        json += "\"mac\":\"" + g_tires[i].mac + "\",";
        json += "\"sensor_id\":\"" + g_tires[i].sensor_id + "\",";
        json += "\"psi\":" + String(g_tires[i].pressure_psi, 1) + ",";
        json += "\"bar\":" + String(g_tires[i].pressure_bar, 2) + ",";
        json += "\"kpa\":" + String(g_tires[i].pressure_kpa, 1) + ",";
        json += "\"temp_c\":" + String(g_tires[i].temperature_c, 1) + ",";
        json += "\"temp_f\":" + String(g_tires[i].temperature_f, 1) + ",";
        json += "\"battery\":" + String(g_tires[i].battery_percent) + ",";
        json += "\"rssi\":" + String(g_tires[i].rssi) + ",";
        json += "\"mode\":\"" + g_tires[i].mode + "\",";
        json += "\"alert\":\"" + String(g_tires[i].getAlertString()) + "\",";
        json += "\"age_s\":" + String(ageS);
        json += "}";
        if (i < 3) json += ",";
    }
    json += "}}";

    server.send(200, "application/json", json);
}

// Handle /api/logs Endpoint (Rolling Event Log)
void handleApiLogs() {
    String json = "[";
    portENTER_CRITICAL(&g_logMux);
    int startIdx = (g_logCount < MAX_LOG_ENTRIES) ? 0 : g_logHead;
    for (int i = 0; i < g_logCount; i++) {
        int idx = (startIdx + i) % MAX_LOG_ENTRIES;
        json += "{\"t\":" + String(g_logs[idx].timestamp_s) + ",\"msg\":\"" + String(g_logs[idx].text) + "\"}";
        if (i < g_logCount - 1) json += ",";
    }
    portEXIT_CRITICAL(&g_logMux);
    json += "]";

    server.send(200, "application/json", json);
}

// Handle /api/clear
void handleApiClear() {
    portENTER_CRITICAL(&g_logMux);
    g_logCount = 0;
    g_logHead = 0;
    portEXIT_CRITICAL(&g_logMux);
    server.send(200, "text/plain", "OK");
}
#endif

// =====================================================================
// 6. MAIN ARDUINO SETUP & LOOP
// =====================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("  ESP32 Treel TPMS Receiver + Remote Web Dashboard     ");
    Serial.println("=======================================================");

    // Initialize 4 Tire Data Objects
    for (int i = 0; i < 4; i++) {
        g_tires[i].mac = SENSOR_MACS[i];
        g_tires[i].position = (TirePosition)i;
        g_tires[i].has_received = false;
    }

    // Initialize 1.3" / 0.96" I2C OLED Display
    g_display.begin();

#if ENABLE_WEBSERVER
    // 1. Initialize Wi-Fi (STA mode first, fallback to AP mode)
    WiFi.mode(WIFI_STA);
    bool staConnected = false;

    if (TRY_STA_FIRST && strlen(WIFI_SSID) > 0) {
        Serial.printf("[Wi-Fi] Connecting to %s ...\n", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        unsigned long startMs = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startMs < (unsigned long)STA_TIMEOUT_S * 1000)) {
            delay(300);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            staConnected = true;
            g_ipAddress = WiFi.localIP().toString();
            g_wifiModeStr = "STA (" + String(WIFI_SSID) + ")";
            addSystemLog("[Wi-Fi] Connected to %s | IP: http://%s", WIFI_SSID, g_ipAddress.c_str());
        } else {
            Serial.println("[Wi-Fi] STA connection timed out!");
        }
    }

    if (!staConnected) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        g_ipAddress = WiFi.softAPIP().toString();
        g_wifiModeStr = "AP (" + String(AP_SSID) + ")";
        addSystemLog("[Wi-Fi] SoftAP Started! SSID: %s | URL: http://%s", AP_SSID, g_ipAddress.c_str());
    }

    // 2. Start HTTP Web Server
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/logs", HTTP_GET, handleApiLogs);
    server.on("/api/clear", HTTP_GET, handleApiClear);
    server.begin();
    addSystemLog("[HTTP] Web Server active on port 80");
#else
    // Completely shut down Wi-Fi radio for maximum BLE performance & lowest power consumption
    WiFi.mode(WIFI_OFF);
    addSystemLog("[Wi-Fi] Web Server Disabled (Pure BLE Receiver Mode)");
#endif

    // 3. Initialize NimBLE Active Scanner (Continuous Background Scan)
    NimBLEDevice::init("ESP32-TPMS");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new AdvertisedDeviceCallbacks(), true); // wantDuplicates = true
    pScan->setActiveScan(true);  // Send SCAN_REQ to capture scan response payloads
    pScan->setInterval(45);      // 45 ms scan interval
    pScan->setWindow(45);        // 45 ms scan window (100% duty cycle continuous)
    pScan->setDuplicateFilter(false);

    pScan->start(0, false);      // Start 100% continuous non-blocking scan
    addSystemLog("[BLE] NimBLE Active Continuous Scanner started! Monitoring 4 Tires.");
}

void loop() {
#if ENABLE_WEBSERVER
    // Handle Web Server Client Requests
    server.handleClient();
#endif

    // Render OLED Display periodically (every 250ms)
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = millis();
        g_display.render(g_tires);
    }

    // Periodic Heartbeat to Serial Monitor every 15 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 15000) {
        lastHeartbeat = millis();
        Serial.printf("[HEARTBEAT] Free Heap: %d B | Total BLE: %d | TPMS: %d | Tires OK: [FL:%d FR:%d RL:%d RR:%d]\n",
                      ESP.getFreeHeap(), g_totalBlePackets, g_tpmsPackets,
                      g_tires[0].has_received, g_tires[1].has_received, g_tires[2].has_received, g_tires[3].has_received);
    }

    delay(2);
}
