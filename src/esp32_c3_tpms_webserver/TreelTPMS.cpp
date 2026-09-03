#include "TreelTPMS.h"

static const uint8_t AES_KEY[16] = {
    '#', '@', 'T', 'r', 'l', '2', '0', '1', '8', '-', 'l', 'e', 's', 'p', 'l', '$'
};

static const uint8_t TREEL_BEACON_UUID[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0
};

mbedtls_aes_context TreelTPMSC3::s_aesCtx;
bool TreelTPMSC3::s_aesInit = false;

TreelTPMSC3 TreelSensorReceiverC3;

class FastAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
    void onDiscovered(const NimBLEAdvertisedDevice* dev) override {
        TreelSensorReceiverC3.processDevice(dev);
    }
};

TreelTPMSC3::TreelTPMSC3() {
    for (int i = 0; i < 4; i++) {
        strncpy(m_tires[i].mac, SENSOR_MAC_STRS[i], sizeof(m_tires[i].mac) - 1);
        m_tires[i].position = (TirePosition)i;
        m_tires[i].has_received = false;
    }
}

void TreelTPMSC3::initAES() {
    if (!s_aesInit) {
        mbedtls_aes_init(&s_aesCtx);
        mbedtls_aes_setkey_dec(&s_aesCtx, AES_KEY, 128);
        s_aesInit = true;
    }
}

void TreelTPMSC3::setDemoMode(bool enable) {
    m_demoMode = enable;
    if (enable) {
        m_demoStep = 0;
        m_lastDemoStepMs = 0;
    }
}

void TreelTPMSC3::update() {
    if (m_demoMode) {
        if (millis() - m_lastDemoStepMs >= 4000 || m_lastDemoStepMs == 0) {
            m_lastDemoStepMs = millis();
            runDemoStep();
        }
    }
}

void TreelTPMSC3::runDemoStep() {
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
        if (m_callback) {
            m_callback(m_tires[i]);
        }
    }
    m_demoStep++;
}

void TreelTPMSC3::begin(TPMSCallbackC3 callback) {
    m_callback = callback;
    initAES();

    NimBLEDevice::init("ESP32C3-TPMS");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new FastAdvertisedDeviceCallbacks(), true);
    pScan->setActiveScan(false);
    pScan->setInterval(160); // 100 ms scan interval
    pScan->setWindow(40);    // 25 ms scan window (leaves 75% RF radio time for Wi-Fi SoftAP & Web Server)
    pScan->setDuplicateFilter(false);
    pScan->setMaxResults(0);
    pScan->start(0, false);
}

TirePosition FastTreelDecoder::resolvePositionByAddress(const NimBLEAddress& addr) {
    std::string str = addr.toString();
    for (char& c : str) {
        if (c >= 'a' && c <= 'z') c -= 32;
    }
    for (int i = 0; i < 4; i++) {
        if (str.find(SENSOR_MAC_STRS[i]) != std::string::npos) {
            return (TirePosition)i;
        }
    }
    return POS_UNKNOWN;
}

TirePosition FastTreelDecoder::resolvePositionByPayloadSignature(const uint8_t* data, size_t len) {
    if (!data || len < 3) return POS_UNKNOWN;
    for (size_t offset = 0; offset <= len - 3; offset++) {
        for (int i = 0; i < 4; i++) {
            if (data[offset]     == SENSOR_SHORT_SIGS[i][0] &&
                data[offset + 1] == SENSOR_SHORT_SIGS[i][1] &&
                data[offset + 2] == SENSOR_SHORT_SIGS[i][2]) {
                return (TirePosition)i;
            }
        }
    }
    return POS_UNKNOWN;
}

bool FastTreelDecoder::decodeBeacon(const uint8_t* payload, size_t len, float& outPsi, float& outTemp, char* outSensorId, size_t idBufSize) {
    if (!payload || len < 23) return false;
    for (size_t offset = 0; offset <= len - 23; offset++) {
        if (payload[offset] == 0x02 && payload[offset + 1] == 0x15) {
            if (memcmp(payload + offset + 2, TREEL_BEACON_UUID, 16) != 0) continue;

            uint16_t major = (payload[offset + 18] << 8) | payload[offset + 19];
            uint16_t minor = (payload[offset + 20] << 8) | payload[offset + 21];
            uint8_t txTemp = payload[offset + 22];

            float pressPsi = (float)(minor & 0xFF);
            float tempC = (txTemp > 65) ? (float)((int)txTemp - 110) : 0.0f;

            if (pressPsi >= 0.0f && pressPsi <= 217.0f && tempC >= -40.0f && tempC <= 125.0f) {
                outPsi = pressPsi;
                outTemp = tempC;
                if (outSensorId && idBufSize >= 12) {
                    snprintf(outSensorId, idBufSize, "%04X-%04X", major, minor);
                }
                return true;
            }
        }
    }
    return false;
}

bool FastTreelDecoder::decodeGATT(const uint8_t* payload, size_t len, float& outPsi, float& outTemp, int& outBatt, char* outSensorId, size_t idBufSize) {
    if (!payload || len < 16) return false;
    uint8_t dec[16];
    for (size_t offset = 0; offset <= len - 16; offset++) {
        if (!TreelTPMSC3::fastDecryptAES128(payload + offset, dec)) continue;
        if (dec[0] != 0x16) continue;

        uint16_t rawTemp = dec[1] | (dec[2] << 8);
        if (rawTemp == 65535) continue;
        float tempC = (rawTemp <= 32768) ? (rawTemp / 100.0f) : -((rawTemp - 32768) / 100.0f);

        uint16_t rawPress = dec[3] | (dec[4] << 8);
        if (rawPress == 65535) continue;
        float pressPsi = rawPress / 100.0f;

        uint8_t batt = dec[5];

        if (tempC >= -40.0f && tempC <= 125.0f && pressPsi >= 0.0f && pressPsi <= 217.0f && batt <= 100) {
            outPsi = pressPsi;
            outTemp = tempC;
            outBatt = batt;
            return true;
        }
    }
    return false;
}

void TreelTPMSC3::processDevice(const NimBLEAdvertisedDevice* dev) {
    if (!dev) return;
    m_totalBlePackets++;

    TirePosition pos = FastTreelDecoder::resolvePositionByAddress(dev->getAddress());

    const std::vector<uint8_t>& payloadVec = dev->getPayload();
    const uint8_t* payload = payloadVec.data();
    size_t payloadLen = payloadVec.size();

    std::string mfgStr = dev->getManufacturerData();
    const uint8_t* mfgData = (const uint8_t*)mfgStr.data();
    size_t mfgLen = mfgStr.length();

    if (pos >= POS_UNKNOWN) {
        if (mfgData && mfgLen >= 3) pos = FastTreelDecoder::resolvePositionByPayloadSignature(mfgData, mfgLen);
        if (pos >= POS_UNKNOWN && payload && payloadLen >= 3) pos = FastTreelDecoder::resolvePositionByPayloadSignature(payload, payloadLen);
    }

    if (pos >= POS_UNKNOWN) return;

    m_tpmsPackets++;
    int rssi = dev->getRSSI();

    float psi = 0.0f, tempC = 0.0f;
    int batt = -1;
    char sensorId[16] = {0};
    bool decoded = false;
    const char* decMode = "--";

    if (mfgData && mfgLen >= 16) {
        if (FastTreelDecoder::decodeBeacon(mfgData, mfgLen, psi, tempC, sensorId, sizeof(sensorId))) {
            decoded = true;
            decMode = "iBeacon";
        } else if (FastTreelDecoder::decodeGATT(mfgData, mfgLen, psi, tempC, batt, sensorId, sizeof(sensorId))) {
            decoded = true;
            decMode = "GATT/AES";
        }
    }

    if (!decoded && payload && payloadLen >= 16) {
        if (FastTreelDecoder::decodeBeacon(payload, payloadLen, psi, tempC, sensorId, sizeof(sensorId))) {
            decoded = true;
            decMode = "iBeacon";
        } else if (FastTreelDecoder::decodeGATT(payload, payloadLen, psi, tempC, batt, sensorId, sizeof(sensorId))) {
            decoded = true;
            decMode = "GATT/AES";
        }
    }

    if (decoded) {
        m_tires[pos].update(psi, tempC, batt, rssi, decMode, sensorId);
        if (m_callback) {
            m_callback(m_tires[pos]);
        }
    }
}
