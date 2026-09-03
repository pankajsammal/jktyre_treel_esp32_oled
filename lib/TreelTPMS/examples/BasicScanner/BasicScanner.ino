// =====================================================================
// BasicScanner.ino — TreelTPMS Library Basic Example
// =====================================================================
// Demonstrates how to import and use the TreelTPMS library in any ESP32 project.
// =====================================================================

#include <Arduino.h>
#include <TreelTPMS.h>

// 1. Configure your 4 TPMS sensor MAC addresses & Short IDs
const char* SENSOR_MACS[4] = {
    "D2:58:6D:8F:16:10",  // FL: Front Left (Replace with your MAC)
    "CA:E8:6C:2D:92:15",  // FR: Front Right (Replace with your MAC)
    "F7:FC:85:AD:35:E2",  // RL: Rear Left (Replace with your MAC)
    "CD:8D:E6:9E:FB:E6"   // RR: Rear Right (Replace with your MAC)
};

const char* SENSOR_SHORT_IDS[4] = {
    "8F1610",  // FL Short ID
    "2D9215",  // FR Short ID
    "AD35E2",  // RL Short ID
    "9EFBE6"   // RR Short ID
};

// 2. Callback fired every time a tire telemetry packet is received
void onTireUpdate(const TireData& tire) {
    const char* posNames[] = {"Front Left", "Front Right", "Rear Left", "Rear Right"};
    Serial.printf("[TPMS UPDATE] %s | %.1f PSI (%.2f Bar) | %.1f °C | Batt: %d%% | Mode: %s | RSSI: %d dBm\n",
                  posNames[tire.position],
                  tire.pressure_psi,
                  tire.pressure_bar,
                  tire.temperature_c,
                  tire.battery_percent,
                  tire.mode.c_str(),
                  tire.rssi);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=================================");
    Serial.println(" TreelTPMS Basic Scanner Example ");
    Serial.println("=================================");

    // Initialize Whitelist & Callback
    TreelSensorReceiver.setWhitelist(SENSOR_MACS, SENSOR_SHORT_IDS);
    TreelSensorReceiver.setCallback(onTireUpdate);

    // Start Continuous BLE Scanning
    TreelSensorReceiver.begin();
    Serial.println("[BLE] Scanner started listening for TPMS sensors...");
}

void loop() {
    // Print periodic status summary every 10 seconds
    static unsigned long lastSummary = 0;
    if (millis() - lastSummary >= 10000) {
        lastSummary = millis();
        Serial.printf("\n--- Telemetry Summary (Total BLE: %u, TPMS: %u) ---\n",
                      TreelSensorReceiver.getTotalBlePackets(),
                      TreelSensorReceiver.getTpmsPackets());

        const char* posNames[] = {"FL", "FR", "RL", "RR"};
        for (int i = 0; i < 4; i++) {
            TireData t = TreelSensorReceiver.getTire((TirePosition)i);
            if (t.has_received) {
                Serial.printf("  %s: %.1f PSI | %.1f °C | Status: %s\n",
                              posNames[i], t.pressure_psi, t.temperature_c, t.getAlertString());
            } else {
                Serial.printf("  %s: Waiting for telemetry...\n", posNames[i]);
            }
        }
        Serial.println("---------------------------------------------------\n");
    }

    delay(10);
}
