#ifndef TREEL_TPMS_H
#define TREEL_TPMS_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "mbedtls/aes.h"

// Alert Thresholds
#define ALERT_MIN_PSI     26.0f
#define ALERT_MAX_PSI     45.0f
#define ALERT_MAX_TEMP_C  70.0f
#define ALERT_MIN_BATT    15

enum TirePosition {
    POS_FL = 0,
    POS_FR = 1,
    POS_RL = 2,
    POS_RR = 3,
    POS_UNKNOWN = 4
};

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
    TirePosition position = POS_UNKNOWN;
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

    void clear() {
        pressure_psi = 0.0f;
        pressure_bar = 0.0f;
        pressure_kpa = 0.0f;
        temperature_c = 0.0f;
        temperature_f = 32.0f;
        battery_percent = -1;
        rssi = -100;
        mode = "--";
        sensor_id = "";
        last_updated_ms = 0;
        has_received = false;
    }

    AlertState getAlertState(float minPsi = ALERT_MIN_PSI, float maxPsi = ALERT_MAX_PSI, float maxTempC = ALERT_MAX_TEMP_C, int minBatt = ALERT_MIN_BATT) const {
        if (!has_received) return ALERT_WAITING;
        if (pressure_psi < minPsi) return ALERT_LOW_PRESSURE;
        if (pressure_psi > maxPsi) return ALERT_HIGH_PRESSURE;
        if (temperature_c > maxTempC) return ALERT_HIGH_TEMP;
        if (battery_percent >= 0 && battery_percent < minBatt) return ALERT_LOW_BATT;
        return ALERT_NORMAL;
    }

    const char* getAlertString(float minPsi = ALERT_MIN_PSI, float maxPsi = ALERT_MAX_PSI, float maxTempC = ALERT_MAX_TEMP_C, int minBatt = ALERT_MIN_BATT) const {
        switch (getAlertState(minPsi, maxPsi, maxTempC, minBatt)) {
            case ALERT_LOW_PRESSURE:  return "LOW_PRESSURE";
            case ALERT_HIGH_PRESSURE: return "HIGH_PRESSURE";
            case ALERT_HIGH_TEMP:     return "HIGH_TEMP";
            case ALERT_LOW_BATT:      return "LOW_BATTERY";
            case ALERT_NORMAL:        return "NORMAL";
            default:                  return "WAITING";
        }
    }
};

typedef void (*TPMSCallback)(const TireData& tire);

class TreelTPMS {
private:
    static mbedtls_aes_context s_aesCtx;
    static bool s_aesInit;
    
    String m_whitelistedMacs[4];
    String m_whitelistedShortIds[4];
    TireData m_tires[4];
    TPMSCallback m_userCallback = nullptr;
    
    volatile uint32_t m_totalBlePackets = 0;
    volatile uint32_t m_tpmsPackets = 0;

    // Demo / Test Mode (Simulates normal & warning states)
    void setDemoMode(bool enable);
    bool isDemoMode() const { return m_demoMode; }
    void update(); // Main periodic update loop (handles demo mode simulation)
    void clearAllTires();

    void processAdvertisedDevice(const NimBLEAdvertisedDevice* advertisedDevice);

private:
    bool m_demoMode = false;
    unsigned long m_lastDemoStepMs = 0;
    int m_demoStep = 0;
    void runDemoStep();
};

extern TreelTPMS TreelSensorReceiver;

#endif // TREEL_TPMS_H
