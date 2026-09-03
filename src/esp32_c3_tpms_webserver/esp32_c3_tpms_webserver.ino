// =====================================================================
// ESP32-C3 SuperMini Ultra-Optimized Treel TPMS BLE Receiver & Web Server
// =====================================================================
// Dedicated Single-Core RISC-V Architecture Firmware
// =====================================================================

#include <Arduino.h>
#include <TreelTPMS.h>
#include "Config.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "DisplayManager.h"
#include "WebServerManager.h"

void onTirePacketReceivedC3(const TireData& tire) {
    const char* posAbbr[] = {"FL", "FR", "RL", "RR"};
    if (tire.position < POS_UNKNOWN) {
        Logger.addLog("[TPMS-%s] %.1f PSI | %.1f C | Batt: %d%% | %s | RSSI: %d",
                      posAbbr[tire.position], tire.pressure_psi, tire.temperature_c,
                      tire.battery_percent, tire.mode, tire.rssi);
    }
}

void setup() {
    Serial.begin(115200);
    delay(400);

    Serial.println("\n=======================================================");
    Serial.println("  ESP32-C3 SuperMini Treel TPMS Receiver + Web Server  ");
    Serial.println("=======================================================");

    // 0. Load Dynamic Settings from NVS Flash
    ConfigMgr.begin();

    // 1. Initialize Display (if enabled)
    Display.begin();

    // 2. Connect Wi-Fi & Start Web Server (if enabled)
    WebDash.begin();

    // 3. Start Zero-Allocation NimBLE Scanner
    if (ConfigMgr.enable_demo_mode) {
        TreelSensorReceiverC3.setDemoMode(true);
        Logger.addLog("[DEMO] Test Mode ENABLED! Simulating live telemetry & warnings.");
    }
    TreelSensorReceiverC3.begin(onTirePacketReceivedC3);
    Logger.addLog("[BLE] Ultra-Fast Zero-Allocation Scanner active! Monitoring 4 Tires.");
}

void loop() {
    // 0. Update Receiver / Demo mode logic
    TreelSensorReceiverC3.update();

    // 1. Handle HTTP Client requests
    WebDash.handleClient();

    // 2. Update OLED Display (every 250ms)
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = millis();
        Display.render(TreelSensorReceiverC3.getAllTires());
    }

    // 3. Periodic Heartbeat to Serial Monitor
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 15000) {
        lastHeartbeat = millis();
        const TireData* tires = TreelSensorReceiverC3.getAllTires();
        Serial.printf("[HEARTBEAT] Free Heap: %d B | Total BLE: %u | TPMS: %u | [FL:%d FR:%d RL:%d RR:%d]\n",
                      ESP.getFreeHeap(), TreelSensorReceiverC3.getTotalBlePackets(), TreelSensorReceiverC3.getTpmsPackets(),
                      tires[0].has_received, tires[1].has_received, tires[2].has_received, tires[3].has_received);
    }

    delay(2);
}
