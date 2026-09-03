#ifndef TREEL_TPMS_C3_H
#define TREEL_TPMS_C3_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "mbedtls/aes.h"
#include "Config.h"

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
    char mac[18];
    char sensor_id[12];
    TirePosition position;
    float pressure_psi = 0.0f;
    float pressure_bar = 0.0f;
    float pressure_kpa = 0.0f;
    float temperature_c = 0.0f;
    float temperature_f = 32.0f;
    int battery_percent = -1;
    int rssi = -100;
    char mode[12];
    uint32_t last_updated_ms = 0;
    bool has_received = false;

    void update(float psi, float tempC, int batt, int sigRssi, const char* decMode, const char* sId) {
        pressure_psi = psi;
        pressure_bar = psi * 0.0689476f;
        pressure_kpa = psi * 6.89476f;
        temperature_c = tempC;
        temperature_f = (tempC * 9.0f / 5.0f) + 32.0f;
        if (batt >= 0) battery_percent = batt;
        rssi = sigRssi;
        strncpy(mode, decMode, sizeof(mode) - 1);
        if (sId && strlen(sId) > 0) strncpy(sensor_id, sId, sizeof(sensor_id) - 1);
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

typedef void (*TPMSCallbackC3)(const TireData& tire);

class FastTreelDecoder {
public:
    static TirePosition resolvePositionByAddress(const NimBLEAddress& addr);
    static TirePosition resolvePositionByPayloadSignature(const uint8_t* data, size_t len);
    static bool decodeBeacon(const uint8_t* payload, size_t len, float& outPsi, float& outTemp, char* outSensorId, size_t idBufSize);
    static bool decodeGATT(const uint8_t* payload, size_t len, float& outPsi, float& outTemp, int& outBatt, char* outSensorId, size_t idBufSize);
};

class TreelTPMSC3 {
private:
    static mbedtls_aes_context s_aesCtx;
    static bool s_aesInit;

    TireData m_tires[4];
    TPMSCallbackC3 m_callback = nullptr;
    volatile uint32_t m_totalBlePackets = 0;
    volatile uint32_t m_tpmsPackets = 0;

public:
    TreelTPMSC3();

    void begin(TPMSCallbackC3 callback = nullptr);
    void processDevice(const NimBLEAdvertisedDevice* dev);

    TireData getTire(TirePosition pos) const { return m_tires[pos]; }
    const TireData* getAllTires() const { return m_tires; }

    uint32_t getTotalBlePackets() const { return m_totalBlePackets; }
    uint32_t getTpmsPackets() const { return m_tpmsPackets; }

    static void initAES();
    static inline bool fastDecryptAES128(const uint8_t* ciphertext, uint8_t* plaintext) {
        return (mbedtls_aes_crypt_ecb(&s_aesCtx, MBEDTLS_AES_DECRYPT, ciphertext, plaintext) == 0);
    }
};

extern TreelTPMSC3 TreelSensorReceiverC3;

#endif // TREEL_TPMS_C3_H
