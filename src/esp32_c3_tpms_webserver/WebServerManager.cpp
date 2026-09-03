#include "WebServerManager.h"

WebServerManager WebDash;

#if ENABLE_WEBSERVER
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-C3 Treel TPMS</title>
    <style>
        :root {
            --bg: #090d16;
            --card: #131b2e;
            --border: #1e293b;
            --text: #f8fafc;
            --muted: #94a3b8;
            --cyan: #38bdf8;
            --green: #10b981;
            --red: #ef4444;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg);
            color: var(--text);
            padding: 14px;
            display: flex;
            justify-content: center;
        }
        .container { width: 100%; max-width: 850px; }
        header { text-align: center; margin-bottom: 16px; padding-bottom: 10px; border-bottom: 1px solid var(--border); }
        h1 { color: var(--cyan); font-size: 1.5rem; }
        .meta { font-size: 0.82rem; color: var(--muted); margin-top: 6px; display: flex; justify-content: center; gap: 14px; flex-wrap: wrap; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px; }
        @media (max-width: 550px) { .grid { grid-template-columns: 1fr; } }
        .card { background: var(--card); border: 1px solid var(--border); border-radius: 10px; padding: 14px; }
        .card.alert { border-color: var(--red); box-shadow: 0 0 12px rgba(239, 68, 68, 0.3); }
        .card-top { display: flex; justify-content: space-between; align-items: center; }
        .pos { font-size: 1.1rem; font-weight: 700; color: var(--cyan); }
        .badge { font-size: 0.72rem; padding: 2px 7px; border-radius: 10px; background: #334155; color: #cbd5e1; }
        .badge.ok { background: rgba(16, 185, 129, 0.2); color: var(--green); border: 1px solid var(--green); }
        .badge.alt { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid var(--red); }
        .hero { display: flex; align-items: baseline; gap: 6px; margin: 8px 0; }
        .psi { font-size: 2.4rem; font-weight: 800; color: #fff; line-height: 1; }
        .unit { font-size: 1rem; color: var(--cyan); font-weight: 600; }
        .bar { font-size: 0.85rem; color: var(--muted); }
        .details { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-top: 10px; padding-top: 8px; border-top: 1px solid var(--border); font-size: 0.82rem; }
        .details span { color: var(--muted); font-size: 0.72rem; display: block; }
        .card-foot { margin-top: 8px; font-size: 0.72rem; color: #64748b; display: flex; justify-content: space-between; }
        .log-box { background: var(--card); border: 1px solid var(--border); border-radius: 10px; padding: 12px; }
        .log-head { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
        .log-head h3 { font-size: 0.92rem; color: var(--cyan); }
        .term { background: #050811; border: 1px solid var(--border); border-radius: 6px; padding: 8px; height: 160px; overflow-y: auto; font-family: monospace; font-size: 0.75rem; color: #38bdf8; line-height: 1.35; }
        .btn { background: #0284c7; color: #fff; border: none; padding: 3px 8px; border-radius: 4px; font-size: 0.72rem; cursor: pointer; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>TREEL TPMS &bull; ESP32-C3 SUPERMINI</h1>
            <div class="meta">
                <span>Wi-Fi: <strong id="wf">--</strong></span>
                <span>IP: <strong id="ip">--</strong></span>
                <span>Uptime: <strong id="up">0s</strong></span>
                <span>Packets: <strong id="pk">0</strong></span>
                <span>RAM: <strong id="hp">0 KB</strong></span>
            </div>
        </header>

        <div class="grid">
            <div class="card" id="c-FL"><div class="card-top"><span class="pos">FL - Front Left</span><span class="badge" id="b-FL">WAITING</span></div><div class="hero"><div class="psi" id="p-FL">--</div><div class="unit">PSI</div><div class="bar" id="bar-FL">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-FL">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-FL">--%</strong></div><div><span>MODE / ID</span><strong id="m-FL">--</strong></div><div><span>SIGNAL</span><strong id="r-FL">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-FL">Never</span></div></div>
            <div class="card" id="c-FR"><div class="card-top"><span class="pos">FR - Front Right</span><span class="badge" id="b-FR">WAITING</span></div><div class="hero"><div class="psi" id="p-FR">--</div><div class="unit">PSI</div><div class="bar" id="bar-FR">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-FR">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-FR">--%</strong></div><div><span>MODE / ID</span><strong id="m-FR">--</strong></div><div><span>SIGNAL</span><strong id="r-FR">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-FR">Never</span></div></div>
            <div class="card" id="c-RL"><div class="card-top"><span class="pos">RL - Rear Left</span><span class="badge" id="b-RL">WAITING</span></div><div class="hero"><div class="psi" id="p-RL">--</div><div class="unit">PSI</div><div class="bar" id="bar-RL">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-RL">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-RL">--%</strong></div><div><span>MODE / ID</span><strong id="m-RL">--</strong></div><div><span>SIGNAL</span><strong id="r-RL">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-RL">Never</span></div></div>
            <div class="card" id="c-RR"><div class="card-top"><span class="pos">RR - Rear Right</span><span class="badge" id="b-RR">WAITING</span></div><div class="hero"><div class="psi" id="p-RR">--</div><div class="unit">PSI</div><div class="bar" id="bar-RR">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-RR">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-RR">--%</strong></div><div><span>MODE / ID</span><strong id="m-RR">--</strong></div><div><span>SIGNAL</span><strong id="r-RR">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-RR">Never</span></div></div>
        </div>

        <div class="log-box">
            <div class="log-head"><h3>Live BLE Telemetry Stream</h3><button class="btn" onclick="fetch('/api/clear')">Clear</button></div>
            <div class="term" id="term">Connecting...</div>
        </div>
    </div>

    <script>
        function fmtAge(s) { if (s < 0) return "Never"; if (s < 60) return s + "s ago"; if (s < 3600) return Math.floor(s / 60) + "m ago"; return Math.floor(s / 3600) + "h ago"; }
        async function poll() {
            try {
                const res = await fetch('/api/data'); if (!res.ok) return; const d = await res.json();
                document.getElementById('wf').innerText = d.sys.wifi;
                document.getElementById('ip').innerText = d.sys.ip;
                document.getElementById('up').innerText = d.sys.up + 's';
                document.getElementById('pk').innerText = d.sys.tpms + '/' + d.sys.ble;
                document.getElementById('hp').innerText = Math.round(d.sys.heap / 1024) + ' KB';
                ['FL','FR','RL','RR'].forEach(p => {
                    const t = d.tires[p]; if (!t) return;
                    const card = document.getElementById('c-' + p); const b = document.getElementById('b-' + p);
                    if (t.has) {
                        document.getElementById('p-' + p).innerText = t.psi.toFixed(1);
                        document.getElementById('bar-' + p).innerText = '(' + t.bar.toFixed(2) + ' Bar)';
                        document.getElementById('t-' + p).innerHTML = t.c.toFixed(1) + ' &deg;C / ' + t.f.toFixed(0) + ' &deg;F';
                        document.getElementById('bat-' + p).innerText = t.bat >= 0 ? t.bat + '%' : 'N/A';
                        document.getElementById('m-' + p).innerText = t.mode + ' (' + (t.id || '--') + ')';
                        document.getElementById('r-' + p).innerText = t.rssi + ' dBm';
                        document.getElementById('a-' + p).innerText = fmtAge(t.age);
                        b.innerText = t.alt; b.className = 'badge ' + (t.alt === 'NORMAL' ? 'ok' : 'alt');
                        card.className = 'card ' + (t.alt !== 'NORMAL' ? 'alert' : '');
                    }
                });
            } catch (e) {}
        }
        async function pollLogs() {
            try {
                const res = await fetch('/api/logs'); if (!res.ok) return; const logs = await res.json();
                const term = document.getElementById('term');
                term.innerHTML = logs.map(l => `<div>[+${l.t}s] ${l.msg}</div>`).join('');
                term.scrollTop = term.scrollHeight;
            } catch(e) {}
        }
        setInterval(poll, 1000); setInterval(pollLogs, 2000); poll(); pollLogs();
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
            delay(250);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            staConnected = true;
            m_ipAddress = WiFi.localIP().toString();
            m_wifiModeStr = "STA (" + String(WIFI_SSID) + ")";
            Logger.addLog("[Wi-Fi] Connected! IP: http://%s", m_ipAddress.c_str());
        }
    }

    if (!staConnected) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        m_ipAddress = WiFi.softAPIP().toString();
        m_wifiModeStr = "AP (" + String(AP_SSID) + ")";
        Logger.addLog("[Wi-Fi] SoftAP active! URL: http://%s", m_ipAddress.c_str());
    }

    m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/api/data", HTTP_GET, [this]() { handleApiData(); });
    m_server.on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    m_server.on("/api/clear", HTTP_GET, [this]() { handleApiClear(); });
    m_server.begin();
    Logger.addLog("[HTTP] Web Server started on port 80");
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
    uint32_t upS = nowMs / 1000;

    String json;
    json.reserve(650);
    json = "{\"sys\":{";
    json += "\"up\":" + String(upS) + ",";
    json += "\"wifi\":\"" + m_wifiModeStr + "\",";
    json += "\"ip\":\"" + m_ipAddress + "\",";
    json += "\"ble\":" + String(TreelSensorReceiverC3.getTotalBlePackets()) + ",";
    json += "\"tpms\":" + String(TreelSensorReceiverC3.getTpmsPackets()) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += "},\"tires\":{";

    const char* keys[] = {"FL", "FR", "RL", "RR"};
    for (int i = 0; i < 4; i++) {
        TireData t = TreelSensorReceiverC3.getTire((TirePosition)i);
        int ageS = t.has_received ? (int)((nowMs - t.last_updated_ms) / 1000) : -1;
        json += "\"" + String(keys[i]) + "\":{";
        json += "\"has\":" + String(t.has_received ? "true" : "false") + ",";
        json += "\"mac\":\"" + String(SENSOR_MAC_STRS[i]) + "\",";
        json += "\"id\":\"" + String(t.sensor_id) + "\",";
        json += "\"psi\":" + String(t.pressure_psi, 1) + ",";
        json += "\"bar\":" + String(t.pressure_bar, 2) + ",";
        json += "\"c\":" + String(t.temperature_c, 1) + ",";
        json += "\"f\":" + String(t.temperature_f, 1) + ",";
        json += "\"bat\":" + String(t.battery_percent) + ",";
        json += "\"rssi\":" + String(t.rssi) + ",";
        json += "\"mode\":\"" + String(t.mode) + "\",";
        json += "\"alt\":\"" + String(t.getAlertString()) + "\",";
        json += "\"age\":" + String(ageS);
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
