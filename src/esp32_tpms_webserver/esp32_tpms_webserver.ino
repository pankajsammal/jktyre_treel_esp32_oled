// =====================================================================
// ESP32 Treel TPMS BLE Receiver & Remote Web Server Dashboard
// =====================================================================
// Standard ESP32 (30-Pin / 38-Pin DevKit) Firmware
// =====================================================================

#include <Arduino.h>
#include "TreelTPMS.h"
#include "Config.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "DisplayManager.h"
#include "WebServerManager.h"

// Callback fired when a tire telemetry packet is received
void onTirePacketReceived(const TireData& tire) {
    const char* posAbbr[] = {"FL", "FR", "RL", "RR"};
    if (tire.position < POS_UNKNOWN) {
        Logger.addLog("[TPMS-%s] %s | %.1f PSI (%.2f Bar) | %.1f C | Batt: %d%% | %s | RSSI: %d dBm",
                      posAbbr[tire.position], tire.mac, tire.pressure_psi, tire.pressure_bar,
                      tire.temperature_c, tire.battery_percent, tire.mode, tire.rssi);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("  ESP32 Treel TPMS Receiver + Remote Web Dashboard     ");
    Serial.println("=======================================================");

    // 0. Load Dynamic Settings from NVS Flash
    ConfigMgr.begin();

    // 1. Initialize OLED Display (if enabled)
    Display.begin();

    // 2. Initialize Wi-Fi & Web Server Dashboard (if enabled)
    WebDash.begin();

    // 3. Initialize & Start TreelTPMS Receiver
    TreelSensorReceiver.setWhitelist(SENSOR_MACS, SENSOR_SHORT_IDS);
    TreelSensorReceiver.setCallback(onTirePacketReceived);
    if (ConfigMgr.enable_demo_mode) {
        TreelSensorReceiver.setDemoMode(true);
        Logger.addLog("[DEMO] Test Mode ENABLED! Simulating live telemetry & warnings.");
    }
    TreelSensorReceiver.begin(true);

    Logger.addLog("[BLE] TreelTPMS receiver started! Monitoring 4 tires.");
}

void loop() {
    // 0. Update Receiver / Demo mode logic
    TreelSensorReceiver.update();

    // 1. Handle Web Server requests
    WebDash.handleClient();

    // 2. Render OLED Display periodically (every 250ms)
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = millis();
        Display.render(TreelSensorReceiver.getAllTires());
    }

    // 3. Periodic Heartbeat to Serial Monitor every 15 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 15000) {
        lastHeartbeat = millis();
        const TireData* tires = TreelSensorReceiver.getAllTires();
        Serial.printf("[HEARTBEAT] Free Heap: %d B | Total BLE: %u | TPMS: %u | Tires OK: [FL:%d FR:%d RL:%d RR:%d]\n",
                      ESP.getFreeHeap(), TreelSensorReceiver.getTotalBlePackets(), TreelSensorReceiver.getTpmsPackets(),
                      tires[0].has_received, tires[1].has_received, tires[2].has_received, tires[3].has_received);
    }

    delay(2);
}
