#include "WebServerManager.h"
#include "ConfigManager.h"

WebServerManager WebDash;

#if ENABLE_WEBSERVER
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>ESP32-C3 Treel TPMS</title><style>:root{--bg:#090d16;--card:#131b2e;--border:#1e293b;--text:#f8fafc;--muted:#94a3b8;--cyan:#38bdf8;--green:#10b981;--red:#ef4444;--accent:#0284c7}*{box-sizing:border-box;margin:0;padding:0}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);padding:12px;display:flex;justify-content:center}.container{width:100%;max-width:850px}header{text-align:center;margin-bottom:14px;padding-bottom:8px;border-bottom:1px solid var(--border)}h1{color:var(--cyan);font-size:1.4rem}.meta{font-size:.8rem;color:var(--muted);margin-top:4px;display:flex;justify-content:center;gap:12px;flex-wrap:wrap}.nav-tabs{display:flex;gap:8px;justify-content:center;margin-bottom:14px}.tab-btn{background:#1e293b;color:var(--muted);border:1px solid var(--border);padding:6px 14px;border-radius:6px;font-size:.8rem;font-weight:600;cursor:pointer}.tab-btn.active{background:var(--accent);color:#fff;border-color:var(--cyan)}.tab-content{display:none}.tab-content.active{display:block}.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}@media(max-width:550px){.grid{grid-template-columns:1fr}}.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:12px}.card.alert{border-color:var(--red);box-shadow:0 0 12px rgba(239,68,68,.3)}.card-top{display:flex;justify-content:space-between;align-items:center}.pos{font-size:1.05rem;font-weight:700;color:var(--cyan)}.badge{font-size:.7rem;padding:2px 6px;border-radius:10px;background:#334155;color:#cbd5e1}.badge.ok{background:rgba(16,185,129,.2);color:var(--green);border:1px solid var(--green)}.badge.alt{background:rgba(239,68,68,.2);color:var(--red);border:1px solid var(--red)}.hero{display:flex;align-items:baseline;gap:6px;margin:6px 0}.psi{font-size:2.2rem;font-weight:800;color:#fff;line-height:1}.unit{font-size:.95rem;color:var(--cyan);font-weight:600}.bar{font-size:.8rem;color:var(--muted)}.details{display:grid;grid-template-columns:1fr 1fr;gap:4px;margin-top:8px;padding-top:6px;border-top:1px solid var(--border);font-size:.8rem}.details span{color:var(--muted);font-size:.7rem;display:block}.card-foot{margin-top:6px;font-size:.7rem;color:#64748b;display:flex;justify-content:space-between}.log-box,.settings-section{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:14px}.log-head{display:flex;justify-content:space-between;align-items:center;margin-bottom:6px}.log-head h3,.settings-section h3{font-size:.9rem;color:var(--cyan);margin-bottom:10px}.term{background:#050811;border:1px solid var(--border);border-radius:6px;padding:8px;height:140px;overflow-y:auto;font-family:monospace;font-size:.72rem;color:#38bdf8;line-height:1.3}.btn{background:var(--accent);color:#fff;border:none;padding:6px 14px;border-radius:6px;font-size:.8rem;font-weight:600;cursor:pointer}.btn-sm{padding:3px 8px;font-size:.7rem}.form-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}@media(max-width:550px){.form-grid{grid-template-columns:1fr}}.form-group{margin-bottom:8px}.form-group label{display:block;font-size:.78rem;color:var(--muted);margin-bottom:3px}.form-group input,.form-group select{width:100%;background:#0a0f1d;border:1px solid var(--border);color:#fff;padding:6px;border-radius:6px;font-size:.82rem}.form-group.checkbox{display:flex;align-items:center;gap:8px;margin-top:10px}.form-group.checkbox input{width:auto}.toast{display:none;background:var(--green);color:#000;padding:8px;border-radius:6px;font-weight:700;text-align:center;margin-bottom:10px;font-size:.8rem}</style></head><body><div class="container"><header><h1>TREEL TPMS &bull; ESP32-C3 SUPERMINI</h1><div class="meta"><span>Wi-Fi: <strong id="wf">--</strong></span><span>IP: <strong id="ip">--</strong></span><span>Uptime: <strong id="up">0s</strong></span><span>Packets: <strong id="pk">0</strong></span><span>RAM: <strong id="hp">0 KB</strong></span></div></header><div class="nav-tabs"><button class="tab-btn active" onclick="switchTab('dash')">📱 Live Dashboard</button><button class="tab-btn" onclick="switchTab('set')">⚙️ System Settings</button></div><div id="tab-dash" class="tab-content active"><div class="grid"><div class="card" id="c-FL"><div class="card-top"><span class="pos">FL - Front Left</span><span class="badge" id="b-FL">WAITING</span></div><div class="hero"><div class="psi" id="p-FL">--</div><div class="unit">PSI</div><div class="bar" id="bar-FL">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-FL">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-FL">--%</strong></div><div><span>MODE / ID</span><strong id="m-FL">--</strong></div><div><span>SIGNAL</span><strong id="r-FL">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-FL">Never</span></div></div><div class="card" id="c-FR"><div class="card-top"><span class="pos">FR - Front Right</span><span class="badge" id="b-FR">WAITING</span></div><div class="hero"><div class="psi" id="p-FR">--</div><div class="unit">PSI</div><div class="bar" id="bar-FR">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-FR">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-FR">--%</strong></div><div><span>MODE / ID</span><strong id="m-FR">--</strong></div><div><span>SIGNAL</span><strong id="r-FR">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-FR">Never</span></div></div><div class="card" id="c-RL"><div class="card-top"><span class="pos">RL - Rear Left</span><span class="badge" id="b-RL">WAITING</span></div><div class="hero"><div class="psi" id="p-RL">--</div><div class="unit">PSI</div><div class="bar" id="bar-RL">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-RL">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-RL">--%</strong></div><div><span>MODE / ID</span><strong id="m-RL">--</strong></div><div><span>SIGNAL</span><strong id="r-RL">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-RL">Never</span></div></div><div class="card" id="c-RR"><div class="card-top"><span class="pos">RR - Rear Right</span><span class="badge" id="b-RR">WAITING</span></div><div class="hero"><div class="psi" id="p-RR">--</div><div class="unit">PSI</div><div class="bar" id="bar-RR">(-- Bar)</div></div><div class="details"><div><span>TEMP</span><strong id="t-RR">-- &deg;C</strong></div><div><span>BATTERY</span><strong id="bat-RR">--%</strong></div><div><span>MODE / ID</span><strong id="m-RR">--</strong></div><div><span>SIGNAL</span><strong id="r-RR">-- dBm</strong></div></div><div class="card-foot"><span>MAC Configured</span><span id="a-RR">Never</span></div></div></div><div class="log-box"><div class="log-head"><h3>Live BLE Telemetry Stream</h3><button class="btn btn-sm" onclick="fetch('/api/clear')">Clear</button></div><div class="term" id="term">Connecting...</div></div></div><div id="tab-set" class="tab-content"><div class="settings-section"><h3>⚙️ ESP32-C3 System Preferences (Saved to NVS Flash)</h3><div id="toast" class="toast">Settings Saved Successfully!</div><form id="set-form" onsubmit="saveSettings(event)"><div class="form-grid"><div class="form-group"><label>OLED Pressure Display Unit</label><select id="cfg-press-unit"><option value="0">PSI (PSI)</option><option value="1">BAR (Bar)</option><option value="2">KPA (kPa)</option></select></div><div class="form-group"><label>OLED Temperature Unit</label><select id="cfg-temp-unit"><option value="0">Celsius (&deg;C)</option><option value="1">Fahrenheit (&deg;F)</option></select></div><div class="form-group"><label>Low Pressure Alert (PSI)</label><input type="number" step="0.5" id="cfg-min-psi"></div><div class="form-group"><label>High Pressure Alert (PSI)</label><input type="number" step="0.5" id="cfg-max-psi"></div><div class="form-group"><label>High Temp Alert (&deg;C)</label><input type="number" step="1" id="cfg-max-temp"></div><div class="form-group"><label>Low Battery Alert (%)</label><input type="number" step="1" id="cfg-min-batt"></div><div class="form-group"><label>OLED SDA Pin (GPIO)</label><input type="number" id="cfg-sda-pin"></div><div class="form-group"><label>OLED SCL Pin (GPIO)</label><input type="number" id="cfg-scl-pin"></div><div class="form-group"><label>SoftAP Wi-Fi SSID</label><input type="text" id="cfg-ap-ssid"></div><div class="form-group"><label>SoftAP Wi-Fi Password (min 8 chars)</label><input type="password" id="cfg-ap-pass"></div></div><div class="form-group checkbox"><input type="checkbox" id="cfg-demo-mode"><label for="cfg-demo-mode"><strong>Enable Test / Demo Mode</strong> (Simulates live tire readings & alerts)</label></div><div style="margin-top:14px"><button type="submit" class="btn">💾 Save & Apply Settings</button></div></form></div></div></div><script>function switchTab(n){document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));document.querySelectorAll('.tab-content').forEach(c=>c.classList.remove('active'));if(n==='dash'){document.querySelectorAll('.tab-btn')[0].classList.add('active');document.getElementById('tab-dash').classList.add('active')}else{document.querySelectorAll('.tab-btn')[1].classList.add('active');document.getElementById('tab-set').classList.add('active');loadSettings()}}function fmtAge(s){if(s<0)return"Never";if(s<60)return s+"s ago";if(s<3600)return Math.floor(s/60)+"m ago";return Math.floor(s/3600)+"h ago"}async function poll(){try{const res=await fetch('/api/data');if(!res.ok)return;const d=await res.json();document.getElementById('wf').innerText=d.sys.wifi;document.getElementById('ip').innerText=d.sys.ip;document.getElementById('up').innerText=d.sys.up+'s';document.getElementById('pk').innerText=d.sys.tpms+'/'+d.sys.ble;document.getElementById('hp').innerText=Math.round(d.sys.heap/1024)+' KB';['FL','FR','RL','RR'].forEach(p=>{const t=d.tires[p];if(!t)return;const card=document.getElementById('c-'+p);const b=document.getElementById('b-'+p);if(t.has){document.getElementById('p-'+p).innerText=t.psi.toFixed(1);document.getElementById('bar-'+p).innerText='('+t.bar.toFixed(2)+' Bar)';document.getElementById('t-'+p).innerHTML=t.c.toFixed(1)+' &deg;C / '+t.f.toFixed(0)+' &deg;F';document.getElementById('bat-'+p).innerText=t.bat>=0?t.bat+'%':'N/A';document.getElementById('m-'+p).innerText=t.mode+' ('+(t.id||'--')+')';document.getElementById('r-'+p).innerText=t.rssi+' dBm';document.getElementById('a-'+p).innerText=fmtAge(t.age);b.innerText=t.alt;b.className='badge '+(t.alt==='NORMAL'?'ok':'alt');card.className='card '+(t.alt!=='NORMAL'?'alert':'')}})}catch(e){}}async function pollLogs(){try{const res=await fetch('/api/logs');if(!res.ok)return;const logs=await res.json();const term=document.getElementById('term');term.innerHTML=logs.map(l=>`<div>[+${l.t}s] ${l.msg}</div>`).join('');term.scrollTop=term.scrollHeight}catch(e){}}async function loadSettings(){try{const res=await fetch('/api/settings');if(!res.ok)return;const s=await res.json();document.getElementById('cfg-press-unit').value=s.press_unit;document.getElementById('cfg-temp-unit').value=s.temp_unit;document.getElementById('cfg-min-psi').value=s.min_psi;document.getElementById('cfg-max-psi').value=s.max_psi;document.getElementById('cfg-max-temp').value=s.max_temp;document.getElementById('cfg-min-batt').value=s.min_batt;document.getElementById('cfg-sda-pin').value=s.sda_pin;document.getElementById('cfg-scl-pin').value=s.scl_pin;document.getElementById('cfg-ap-ssid').value=s.ap_ssid;document.getElementById('cfg-ap-pass').value=s.ap_pass;document.getElementById('cfg-demo-mode').checked=s.demo_mode}catch(e){}}async function saveSettings(e){e.preventDefault();const body=new URLSearchParams({press_unit:document.getElementById('cfg-press-unit').value,temp_unit:document.getElementById('cfg-temp-unit').value,min_psi:document.getElementById('cfg-min-psi').value,max_psi:document.getElementById('cfg-max-psi').value,max_temp:document.getElementById('cfg-max-temp').value,min_batt:document.getElementById('cfg-min-batt').value,sda_pin:document.getElementById('cfg-sda-pin').value,scl_pin:document.getElementById('cfg-scl-pin').value,ap_ssid:document.getElementById('cfg-ap-ssid').value,ap_pass:document.getElementById('cfg-ap-pass').value,demo_mode:document.getElementById('cfg-demo-mode').checked?'true':'false'});try{const res=await fetch('/api/settings',{method:'POST',body});if(res.ok){const t=document.getElementById('toast');t.style.display='block';setTimeout(()=>t.style.display='none',3000)}}catch(e){}}setInterval(poll,1000);setInterval(pollLogs,2000);poll();pollLogs();</script></body></html>)rawliteral";
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
        WiFi.disconnect(true, true);
        delay(100);

        WiFi.mode(WIFI_AP);

        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(local_IP, gateway, subnet);

        const char* pass = (ConfigMgr.ap_pass.length() >= 8) ? ConfigMgr.ap_pass.c_str() : nullptr;
        bool apSuccess = WiFi.softAP(ConfigMgr.ap_ssid.c_str(), pass);

        if (apSuccess) {
            delay(100);
            m_apActive = true;
            m_ipAddress = WiFi.softAPIP().toString();
            m_wifiModeStr = "AP (" + ConfigMgr.ap_ssid + ")";
            m_dnsServer.start(53, "*", local_IP);
            Logger.addLog("[Wi-Fi] SoftAP Active! Connect to '%s' | URL: http://%s", ConfigMgr.ap_ssid.c_str(), m_ipAddress.c_str());
        } else {
            Logger.addLog("[Wi-Fi ERROR] SoftAP failed to start '%s'!", ConfigMgr.ap_ssid.c_str());
        }
    }

    m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/api/data", HTTP_GET, [this]() { handleApiData(); });
    m_server.on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    m_server.on("/api/clear", HTTP_GET, [this]() { handleApiClear(); });
    m_server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    m_server.on("/api/settings", HTTP_POST, [this]() { handlePostSettings(); });

    m_server.onNotFound([this]() {
        if (m_apActive) {
            m_server.send(200, "text/html", INDEX_HTML);
        } else {
            m_server.send(404, "text/plain", "Not Found");
        }
    });

    m_server.begin();
    Logger.addLog("[HTTP] Web Server started on port 80");
#else
    WiFi.mode(WIFI_OFF);
    Logger.addLog("[Wi-Fi] Web Server Disabled (Pure BLE Receiver Mode)");
#endif
}

void WebServerManager::handleClient() {
#if ENABLE_WEBSERVER
    if (m_apActive) {
        m_dnsServer.processNextRequest();
    }
    m_server.handleClient();
#endif
}

void WebServerManager::handleRoot() {
#if ENABLE_WEBSERVER
    m_server.send(200, "text/html", INDEX_HTML);
#endif
}

void WebServerManager::handleGetSettings() {
#if ENABLE_WEBSERVER
    m_server.send(200, "application/json", ConfigMgr.getSettingsJson());
#endif
}

void WebServerManager::handlePostSettings() {
#if ENABLE_WEBSERVER
    String press_unit = m_server.arg("press_unit");
    String temp_unit  = m_server.arg("temp_unit");
    float min_psi     = m_server.arg("min_psi").toFloat();
    float max_psi     = m_server.arg("max_psi").toFloat();
    float max_temp    = m_server.arg("max_temp").toFloat();
    int min_batt      = m_server.arg("min_batt").toInt();
    int sda_pin       = m_server.arg("sda_pin").toInt();
    int scl_pin       = m_server.arg("scl_pin").toInt();
    String ap_ssid    = m_server.arg("ap_ssid");
    String ap_pass    = m_server.arg("ap_pass");
    bool demo_mode    = (m_server.arg("demo_mode") == "true" || m_server.arg("demo_mode") == "1");

    ConfigMgr.updateFromParams(press_unit, temp_unit, min_psi, max_psi, max_temp, min_batt,
                              sda_pin, scl_pin, ap_ssid, ap_pass, demo_mode);

    TreelSensorReceiverC3.setDemoMode(demo_mode);

    Logger.addLog("[SETTINGS] Settings updated via Web UI & saved to NVS!");
    m_server.send(200, "application/json", "{\"status\":\"OK\"}");
#endif
}

void WebServerManager::handleApiData() {
#if ENABLE_WEBSERVER
    uint32_t nowMs = millis();
    uint32_t upS = nowMs / 1000;

    String json;
    json.reserve(550);
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
        json += "\"alt\":\"" + String(t.getAlertString(ConfigMgr.alert_min_psi, ConfigMgr.alert_max_psi, ConfigMgr.alert_max_temp_c, ConfigMgr.alert_min_batt)) + "\",";
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
