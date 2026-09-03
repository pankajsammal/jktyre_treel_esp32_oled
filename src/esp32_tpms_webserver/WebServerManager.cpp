#include "WebServerManager.h"

WebServerManager WebDash;

#if ENABLE_WEBSERVER
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
        .container { width: 100%; max-width: 900px; }
        header { text-align: center; margin-bottom: 20px; padding-bottom: 12px; border-bottom: 1px solid var(--card-border); }
        h1 { color: var(--accent-cyan); font-size: 1.6rem; font-weight: 700; letter-spacing: 0.5px; }
        .meta-bar { margin-top: 6px; font-size: 0.85rem; color: var(--text-dim); display: flex; flex-wrap: wrap; justify-content: center; gap: 16px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; margin-bottom: 20px; }
        @media (max-width: 600px) { .grid { grid-template-columns: 1fr; } }
        .card { background: var(--bg-card); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3); transition: border-color 0.3s ease; }
        .card.alert-low, .card.alert-high, .card.alert-temp { border-color: var(--alert-red); box-shadow: 0 0 16px rgba(239, 68, 68, 0.25); }
        .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
        .pos-title { font-size: 1.1rem; font-weight: 700; color: var(--accent-cyan); }
        .status-badge { font-size: 0.75rem; font-weight: 600; padding: 3px 8px; border-radius: 12px; background: #334155; color: #94a3b8; }
        .status-badge.normal { background: rgba(16, 185, 129, 0.2); color: var(--alert-green); border: 1px solid var(--alert-green); }
        .status-badge.alert  { background: rgba(239, 68, 68, 0.2); color: var(--alert-red); border: 1px solid var(--alert-red); }
        .press-hero { display: flex; align-items: baseline; gap: 8px; margin: 8px 0; }
        .press-val { font-size: 2.6rem; font-weight: 800; color: #ffffff; line-height: 1; }
        .press-unit { font-size: 1.1rem; font-weight: 600; color: var(--accent-cyan); }
        .press-bar { font-size: 0.9rem; color: var(--text-dim); }
        .metrics-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 12px; padding-top: 10px; border-top: 1px solid #1e293b; font-size: 0.85rem; }
        .metric-item span { color: var(--text-dim); font-size: 0.75rem; display: block; }
        .metric-item strong { color: #f1f5f9; font-size: 0.95rem; }
        .card-footer { margin-top: 10px; font-size: 0.75rem; color: #64748b; display: flex; justify-content: space-between; }
        .logs-section { background: var(--bg-card); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; }
        .logs-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; }
        .logs-header h3 { font-size: 1rem; color: var(--accent-cyan); }
        .terminal { background: #060911; border: 1px solid #1e293b; border-radius: 6px; padding: 10px; height: 180px; overflow-y: auto; font-family: monospace; font-size: 0.78rem; color: #38bdf8; line-height: 1.4; }
        .terminal div { margin-bottom: 2px; }
        .btn { background: var(--accent); color: #fff; border: none; padding: 4px 10px; border-radius: 4px; font-size: 0.75rem; cursor: pointer; }
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
            <div class="card" id="card-FL"><div class="card-header"><span class="pos-title">FL - Front Left</span><span class="status-badge" id="badge-FL">WAITING</span></div><div class="press-hero"><div class="press-val" id="psi-FL">--</div><div class="press-unit">PSI</div><div class="press-bar" id="bar-FL">(-- Bar)</div></div><div class="metrics-grid"><div class="metric-item"><span>TEMPERATURE</span><strong id="temp-FL">-- &deg;C</strong></div><div class="metric-item"><span>BATTERY</span><strong id="batt-FL">--%</strong></div><div class="metric-item"><span>MODE / ID</span><strong id="mode-FL">--</strong></div><div class="metric-item"><span>SIGNAL</span><strong id="rssi-FL">-- dBm</strong></div></div><div class="card-footer"><span id="mac-FL">MAC: Configured</span><span id="age-FL">Last: Never</span></div></div>
            <div class="card" id="card-FR"><div class="card-header"><span class="pos-title">FR - Front Right</span><span class="status-badge" id="badge-FR">WAITING</span></div><div class="press-hero"><div class="press-val" id="psi-FR">--</div><div class="press-unit">PSI</div><div class="press-bar" id="bar-FR">(-- Bar)</div></div><div class="metrics-grid"><div class="metric-item"><span>TEMPERATURE</span><strong id="temp-FR">-- &deg;C</strong></div><div class="metric-item"><span>BATTERY</span><strong id="batt-FR">--%</strong></div><div class="metric-item"><span>MODE / ID</span><strong id="mode-FR">--</strong></div><div class="metric-item"><span>SIGNAL</span><strong id="rssi-FR">-- dBm</strong></div></div><div class="card-footer"><span id="mac-FR">MAC: Configured</span><span id="age-FR">Last: Never</span></div></div>
            <div class="card" id="card-RL"><div class="card-header"><span class="pos-title">RL - Rear Left</span><span class="status-badge" id="badge-RL">WAITING</span></div><div class="press-hero"><div class="press-val" id="psi-RL">--</div><div class="press-unit">PSI</div><div class="press-bar" id="bar-RL">(-- Bar)</div></div><div class="metrics-grid"><div class="metric-item"><span>TEMPERATURE</span><strong id="temp-RL">-- &deg;C</strong></div><div class="metric-item"><span>BATTERY</span><strong id="batt-RL">--%</strong></div><div class="metric-item"><span>MODE / ID</span><strong id="mode-RL">--</strong></div><div class="metric-item"><span>SIGNAL</span><strong id="rssi-RL">-- dBm</strong></div></div><div class="card-footer"><span id="mac-RL">MAC: Configured</span><span id="age-RL">Last: Never</span></div></div>
            <div class="card" id="card-RR"><div class="card-header"><span class="pos-title">RR - Rear Right</span><span class="status-badge" id="badge-RR">WAITING</span></div><div class="press-hero"><div class="press-val" id="psi-RR">--</div><div class="press-unit">PSI</div><div class="press-bar" id="bar-RR">(-- Bar)</div></div><div class="metrics-grid"><div class="metric-item"><span>TEMPERATURE</span><strong id="temp-RR">-- &deg;C</strong></div><div class="metric-item"><span>BATTERY</span><strong id="batt-RR">--%</strong></div><div class="metric-item"><span>MODE / ID</span><strong id="mode-RR">--</strong></div><div class="metric-item"><span>SIGNAL</span><strong id="rssi-RR">-- dBm</strong></div></div><div class="card-footer"><span id="mac-RR">MAC: Configured</span><span id="age-RR">Last: Never</span></div></div>
        </div>
        <div class="logs-section">
            <div class="logs-header"><h3>Live BLE Packet Stream</h3><button class="btn" onclick="clearLogs()">Clear</button></div>
            <div class="terminal" id="terminal">Loading telemetry stream...</div>
        </div>
    </div>
    <script>
        function formatAge(ageSec) { if (ageSec < 0) return "Never"; if (ageSec < 60) return ageSec + "s ago"; if (ageSec < 3600) return Math.floor(ageSec / 60) + "m ago"; return Math.floor(ageSec / 3600) + "h ago"; }
        async function fetchTPMS() {
            try {
                const res = await fetch('/api/data'); if (!res.ok) return; const data = await res.json();
                document.getElementById('wifi-mode').innerText = data.system.wifi_mode;
                document.getElementById('ip-addr').innerText = data.system.ip;
                document.getElementById('uptime').innerText = data.system.uptime_s + 's';
                document.getElementById('total-pkts').innerText = data.system.total_ble + ' (' + data.system.tpms_pkts + ' TPMS)';
                ['FL', 'FR', 'RL', 'RR'].forEach(pos => {
                    const tire = data.tires[pos]; if (!tire) return;
                    const card = document.getElementById('card-' + pos); const badge = document.getElementById('badge-' + pos);
                    if (tire.has_data) {
                        document.getElementById('psi-' + pos).innerText = tire.psi.toFixed(1);
                        document.getElementById('bar-' + pos).innerText = '(' + tire.bar.toFixed(2) + ' Bar)';
                        document.getElementById('temp-' + pos).innerHTML = tire.temp_c.toFixed(1) + ' &deg;C / ' + tire.temp_f.toFixed(0) + ' &deg;F';
                        document.getElementById('batt-' + pos).innerText = tire.battery >= 0 ? tire.battery + '%' : 'N/A';
                        document.getElementById('mode-' + pos).innerText = tire.mode + ' (' + (tire.sensor_id || 'ID') + ')';
                        document.getElementById('rssi-' + pos).innerText = tire.rssi + ' dBm';
                        document.getElementById('mac-' + pos).innerText = 'MAC: ' + tire.mac;
                        document.getElementById('age-' + pos).innerText = 'Last: ' + formatAge(tire.age_s);
                        badge.innerText = tire.alert; badge.className = 'status-badge ' + (tire.alert === 'NORMAL' ? 'normal' : 'alert');
                        card.className = 'card ' + (tire.alert !== 'NORMAL' ? 'alert-low' : '');
                    } else { badge.innerText = 'WAITING'; badge.className = 'status-badge'; }
                });
            } catch (e) {}
        }
        async function fetchLogs() {
            try {
                const res = await fetch('/api/logs'); if (!res.ok) return; const logs = await res.json();
                const term = document.getElementById('terminal');
                term.innerHTML = logs.map(l => `<div>[+${l.t}s] ${l.msg}</div>`).join('');
                term.scrollTop = term.scrollHeight;
            } catch (e) {}
        }
        async function clearLogs() { await fetch('/api/clear'); fetchLogs(); }
        setInterval(fetchTPMS, 1000); setInterval(fetchLogs, 2000); fetchTPMS(); fetchLogs();
    </script>
</body>
</html>
)rawliteral";
#endif

WebServerManager::WebServerManager()
#if ENABLE_WEBSERVER
    : m_server(80)
#endif
{}

void WebServerManager::begin() {
#if ENABLE_WEBSERVER
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
            m_ipAddress = WiFi.localIP().toString();
            m_wifiModeStr = "STA (" + String(WIFI_SSID) + ")";
            Logger.addLog("[Wi-Fi] Connected to %s | IP: http://%s", WIFI_SSID, m_ipAddress.c_str());
        } else {
            Serial.println("[Wi-Fi] STA connection timed out!");
        }
    }

    if (!staConnected) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        m_ipAddress = WiFi.softAPIP().toString();
        m_wifiModeStr = "AP (" + String(AP_SSID) + ")";
        Logger.addLog("[Wi-Fi] SoftAP Started! SSID: %s | URL: http://%s", AP_SSID, m_ipAddress.c_str());
    }

    m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/api/data", HTTP_GET, [this]() { handleApiData(); });
    m_server.on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    m_server.on("/api/clear", HTTP_GET, [this]() { handleApiClear(); });
    m_server.begin();
    Logger.addLog("[HTTP] Web Server active on port 80");
#else
    WiFi.mode(WIFI_OFF);
    Logger.addLog("[Wi-Fi] Web Server Disabled (Pure BLE Receiver Mode)");
#endif
}

void WebServerManager::handleClient() {
#if ENABLE_WEBSERVER
    m_server.handleClient();
#endif
}

void WebServerManager::handleRoot() {
#if ENABLE_WEBSERVER
    m_server.send(200, "text/html", INDEX_HTML);
#endif
}

void WebServerManager::handleApiData() {
#if ENABLE_WEBSERVER
    uint32_t nowMs = millis();
    uint32_t uptimeS = nowMs / 1000;

    String json = "{";
    json += "\"system\":{";
    json += "\"uptime_s\":" + String(uptimeS) + ",";
    json += "\"wifi_mode\":\"" + m_wifiModeStr + "\",";
    json += "\"ip\":\"" + m_ipAddress + "\",";
    json += "\"total_ble\":" + String(TreelSensorReceiver.getTotalBlePackets()) + ",";
    json += "\"tpms_pkts\":" + String(TreelSensorReceiver.getTpmsPackets()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "},";

    json += "\"tires\":{";
    const char* keys[] = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; i++) {
        TireData t = TreelSensorReceiver.getTire((TirePosition)i);
        int ageS = t.has_received ? (int)((nowMs - t.last_updated_ms) / 1000) : -1;

        json += "\"" + String(keys[i]) + "\":{";
        json += "\"has_data\":" + String(t.has_received ? "true" : "false") + ",";
        json += "\"name\":\"" + String(POS_NAMES[i]) + "\",";
        json += "\"mac\":\"" + t.mac + "\",";
        json += "\"sensor_id\":\"" + t.sensor_id + "\",";
        json += "\"psi\":" + String(t.pressure_psi, 1) + ",";
        json += "\"bar\":" + String(t.pressure_bar, 2) + ",";
        json += "\"kpa\":" + String(t.pressure_kpa, 1) + ",";
        json += "\"temp_c\":" + String(t.temperature_c, 1) + ",";
        json += "\"temp_f\":" + String(t.temperature_f, 1) + ",";
        json += "\"battery\":" + String(t.battery_percent) + ",";
        json += "\"rssi\":" + String(t.rssi) + ",";
        json += "\"mode\":\"" + t.mode + "\",";
        json += "\"alert\":\"" + String(t.getAlertString()) + "\",";
        json += "\"age_s\":" + String(ageS);
        json += "}";
        if (i < 3) json += ",";
    }
    json += "}}";

    m_server.send(200, "application/json", json);
#endif
}

void WebServerManager::handleApiLogs() {
#if ENABLE_WEBSERVER
    m_server.send(200, "application/json", Logger.getLogsJson());
#endif
}

void WebServerManager::handleApiClear() {
#if ENABLE_WEBSERVER
    Logger.clearLogs();
    m_server.send(200, "text/plain", "OK");
#endif
}
