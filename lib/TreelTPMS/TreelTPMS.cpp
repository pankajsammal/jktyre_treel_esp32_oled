#include "TreelTPMS.h"

static const uint8_t AES_KEY[16] = {
    '#', '@', 'T', 'r', 'l', '2', '0', '1', '8', '-', 'l', 'e', 's', 'p', 'l', '$'
};

static const uint8_t TREEL_BEACON_UUID[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0
};

mbedtls_aes_context TreelTPMS::s_aesCtx;
bool TreelTPMS::s_aesInit = false;

TreelTPMS TreelSensorReceiver;

class TreelNimBLEScanCallbacks : public NimBLEScanCallbacks {
    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
        TreelSensorReceiver.processAdvertisedDevice(advertisedDevice);
    }
};

TreelTPMS::TreelTPMS() {
    for (int i = 0; i < 4; i++) {
        m_tires[i].position = (TirePosition)i;
    }
}

void TreelTPMS::initAES() {
    if (!s_aesInit) {
        mbedtls_aes_init(&s_aesCtx);
        mbedtls_aes_setkey_dec(&s_aesCtx, AES_KEY, 128);
        s_aesInit = true;
    }
}

bool TreelTPMS::decryptAES128(const uint8_t* ciphertext, uint8_t* plaintext) {
    if (!s_aesInit) initAES();
    return (mbedtls_aes_crypt_ecb(&s_aesCtx, MBEDTLS_AES_DECRYPT, ciphertext, plaintext) == 0);
}

void TreelTPMS::setWhitelist(const char* macs[4], const char* shortIds[4]) {
    for (int i = 0; i < 4; i++) {
        if (macs && macs[i]) {
            m_whitelistedMacs[i] = macs[i];
            m_tires[i].mac = macs[i];
        }
        if (shortIds && shortIds[i]) {
            m_whitelistedShortIds[i] = shortIds[i];
        }
    }
}

void TreelTPMS::setDemoMode(bool enable) {
    bool wasDemo = m_demoMode;
    m_demoMode = enable;
    if (enable) {
        m_demoStep = 0;
        m_lastDemoStepMs = 0;
    } else if (wasDemo) {
        clearAllTires();
    }
}

void TreelTPMS::clearAllTires() {
    for (int i = 0; i < 4; i++) {
        m_tires[i].clear();
    }
}

void TreelTPMS::update() {
    if (m_demoMode) {
        if (millis() - m_lastDemoStepMs >= 4000 || m_lastDemoStepMs == 0) {
            m_lastDemoStepMs = millis();
            runDemoStep();
        }
    }
}

void TreelTPMS::runDemoStep() {
    m_totalBlePackets += 4;
    m_tpmsPackets += 4;

    struct DemoVal { float psi; float temp; int batt; const char* mode; };
    static const DemoVal stepVals[4][4] = {
        // Step 0: All Normal
        { {32.0f, 27.0f, 90, "GATT/DEMO"}, {32.0f, 27.0f, 88, "GATT/DEMO"}, {30.0f, 26.0f, 85, "iBeacon"}, {30.0f, 26.0f, 82, "GATT/DEMO"} },
        // Step 1: FL Low Pressure Warning (21.5 PSI)
        { {21.5f, 28.0f, 90, "GATT/DEMO"}, {32.0f, 27.0f, 88, "GATT/DEMO"}, {30.0f, 26.0f, 85, "iBeacon"}, {30.0f, 26.0f, 82, "GATT/DEMO"} },
        // Step 2: RR High Temp Warning (76.0 °C)
        { {32.0f, 27.0f, 90, "GATT/DEMO"}, {32.0f, 27.0f, 88, "GATT/DEMO"}, {30.0f, 26.0f, 85, "iBeacon"}, {34.0f, 76.0f, 82, "GATT/DEMO"} },
        // Step 3: FR High Pressure (48.5 PSI) & RL Low Battery (12%)
        { {32.0f, 27.0f, 90, "GATT/DEMO"}, {48.5f, 35.0f, 88, "GATT/DEMO"}, {30.0f, 26.0f, 12, "iBeacon"}, {30.0f, 26.0f, 82, "GATT/DEMO"} }
    };

    int s = m_demoStep % 4;
    for (int i = 0; i < 4; i++) {
        m_tires[i].update(stepVals[s][i].psi, stepVals[s][i].temp, stepVals[s][i].batt, -65, stepVals[s][i].mode, "DEMO-ID");
        if (m_userCallback) {
            m_userCallback(m_tires[i]);
        }
    }
    m_demoStep++;
}

void TreelTPMS::setCallback(TPMSCallback callback) {
    m_userCallback = callback;
}

void TreelTPMS::begin(bool activeScan) {
    initAES();
    NimBLEDevice::init("ESP32-TPMS");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new TreelNimBLEScanCallbacks(), true);
    pScan->setActiveScan(activeScan);
    pScan->setInterval(160); // 100 ms scan interval
    pScan->setWindow(40);    // 25 ms scan window (leaves 75% RF radio time for Wi-Fi SoftAP & Web Server)
    pScan->setDuplicateFilter(false);
    pScan->start(0, false);
}

TireData TreelTPMS::getTire(TirePosition pos) const {
    if (pos >= POS_FL && pos <= POS_RR) {
        return m_tires[pos];
    }
    return TireData();
}

bool TreelTPMS::decodeGATT(const uint8_t* payload, size_t len, const String& mac, int rssi, TireData& outReading) {
    if (!payload || len < 16) return false;

    uint8_t decrypted[16];
    for (size_t offset = 0; offset <= len - 16; offset++) {
        if (!decryptAES128(payload + offset, decrypted)) continue;

        uint8_t dataType = decrypted[0];
        if (dataType != 0x16) continue;

        uint16_t rawTemp = decrypted[1] | (decrypted[2] << 8);
        if (rawTemp == 65535) continue;
        float tempC = (rawTemp <= 32768) ? (rawTemp / 100.0f) : -((rawTemp - 32768) / 100.0f);

        uint16_t rawPress = decrypted[3] | (decrypted[4] << 8);
        if (rawPress == 65535) continue;
        float pressPsi = rawPress / 100.0f;

        uint8_t batt = decrypted[5];

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

bool TreelTPMS::decodeBeacon(const uint8_t* payload, size_t len, const String& mac, int rssi, TireData& outReading) {
    if (!payload || len < 23) return false;

    for (size_t offset = 0; offset <= len - 23; offset++) {
        if (payload[offset] == 0x02 && payload[offset + 1] == 0x15) {
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
            float tempC = (txTemp > 65) ? (float)((int)txTemp - 110) : 0.0f;

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

TirePosition TreelTPMS::resolvePosition(const String& macFwd, const String& macRev, const String& payloadHex, const String& sensorId) {
    String cleanFwd = macFwd; cleanFwd.replace(":", ""); cleanFwd.replace("-", ""); cleanFwd.toUpperCase();
    String cleanRev = macRev; cleanRev.replace(":", ""); cleanRev.replace("-", ""); cleanRev.toUpperCase();
    String cleanHex = payloadHex; cleanHex.replace(" ", ""); cleanHex.toUpperCase();
    String cleanId  = sensorId; cleanId.replace(":", ""); cleanId.replace("-", ""); cleanId.toUpperCase();

    for (int i = 0; i < 4; i++) {
        String targetMac = m_whitelistedMacs[i];
        targetMac.replace(":", "");
        targetMac.toUpperCase();
        String targetShort = m_whitelistedShortIds[i];
        targetShort.toUpperCase();

        if (cleanFwd.indexOf(targetMac) >= 0 || cleanRev.indexOf(targetMac) >= 0) {
            return (TirePosition)i;
        }

        if (targetShort.length() >= 4) {
            if (cleanFwd.indexOf(targetShort) >= 0 || cleanRev.indexOf(targetShort) >= 0 ||
                cleanId.indexOf(targetShort) >= 0  || cleanHex.indexOf(targetShort) >= 0) {
                return (TirePosition)i;
            }
        }
    }
    return POS_UNKNOWN;
}

void TreelTPMS::processAdvertisedDevice(const NimBLEAdvertisedDevice* advertisedDevice) {
    if (!advertisedDevice) return;
    m_totalBlePackets++;

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
    const std::vector<uint8_t>& payloadVec = advertisedDevice->getPayload();
    const uint8_t* payload = payloadVec.data();
    size_t payloadLen = payloadVec.size();

    std::string mfgStr = advertisedDevice->getManufacturerData();
    const uint8_t* mfgData = (const uint8_t*)mfgStr.data();
    size_t mfgLen = mfgStr.length();

    String payloadHex = "";
    for (size_t i = 0; i < payloadLen; i++) {
        char hBuf[3];
        snprintf(hBuf, sizeof(hBuf), "%02X", payload[i]);
        payloadHex += hBuf;
    }

    TireData reading;
    bool decoded = false;

    if (mfgData && mfgLen >= 16) {
        decoded = decodeBeacon(mfgData, mfgLen, macFwd, rssi, reading) ||
                  decodeGATT(mfgData, mfgLen, macFwd, rssi, reading);
    }
    if (!decoded && payload && payloadLen >= 16) {
        decoded = decodeBeacon(payload, payloadLen, macFwd, rssi, reading) ||
                  decodeGATT(payload, payloadLen, macFwd, rssi, reading);
    }

    TirePosition pos = resolvePosition(macFwd, macRev, payloadHex, reading.sensor_id);

    if (pos < POS_UNKNOWN) {
        m_tpmsPackets++;
        if (decoded) {
            m_tires[pos].update(
                reading.pressure_psi,
                reading.temperature_c,
                reading.battery_percent,
                rssi,
                reading.mode,
                reading.sensor_id
            );
            if (m_userCallback) {
                m_userCallback(m_tires[pos]);
            }
        }
    }
}
