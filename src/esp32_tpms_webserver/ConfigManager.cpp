#include "ConfigManager.h"

ConfigManager ConfigMgr;

ConfigManager::ConfigManager() {
    display_pressure_unit = DISPLAY_PRESSURE_UNIT;
    display_temp_unit     = DISPLAY_TEMP_UNIT;
    alert_min_psi         = ALERT_MIN_PSI;
    alert_max_psi         = ALERT_MAX_PSI;
    alert_max_temp_c      = ALERT_MAX_TEMP_C;
    alert_min_batt        = ALERT_MIN_BATT;
    oled_sda_pin          = OLED_SDA_PIN;
    oled_scl_pin          = OLED_SCL_PIN;
    ap_ssid               = AP_SSID;
    ap_pass               = AP_PASS;
    enable_demo_mode      = ENABLE_DEMO_MODE;
}

void ConfigManager::begin() {
    m_prefs.begin("tpms_cfg", false);

    // Read from NVS Preferences with Config.h fallback
    display_pressure_unit = m_prefs.getUChar("press_unit", DISPLAY_PRESSURE_UNIT);
    display_temp_unit     = m_prefs.getUChar("temp_unit", DISPLAY_TEMP_UNIT);
    alert_min_psi         = m_prefs.getFloat("min_psi", ALERT_MIN_PSI);
    alert_max_psi         = m_prefs.getFloat("max_psi", ALERT_MAX_PSI);
    alert_max_temp_c      = m_prefs.getFloat("max_temp", ALERT_MAX_TEMP_C);
    alert_min_batt        = m_prefs.getInt("min_batt", ALERT_MIN_BATT);
    oled_sda_pin          = m_prefs.getInt("sda_pin", OLED_SDA_PIN);
    oled_scl_pin          = m_prefs.getInt("scl_pin", OLED_SCL_PIN);
    ap_ssid               = m_prefs.getString("ap_ssid", AP_SSID);
    ap_pass               = m_prefs.getString("ap_pass", AP_PASS);
    enable_demo_mode      = m_prefs.getBool("demo_mode", ENABLE_DEMO_MODE);

    m_prefs.end();
}

void ConfigManager::save() {
    m_prefs.begin("tpms_cfg", false);

    m_prefs.putUChar("press_unit", display_pressure_unit);
    m_prefs.putUChar("temp_unit", display_temp_unit);
    m_prefs.putFloat("min_psi", alert_min_psi);
    m_prefs.putFloat("max_psi", alert_max_psi);
    m_prefs.putFloat("max_temp", alert_max_temp_c);
    m_prefs.putInt("min_batt", alert_min_batt);
    m_prefs.putInt("sda_pin", oled_sda_pin);
    m_prefs.putInt("scl_pin", oled_scl_pin);
    m_prefs.putString("ap_ssid", ap_ssid);
    m_prefs.putString("ap_pass", ap_pass);
    m_prefs.putBool("demo_mode", enable_demo_mode);

    m_prefs.end();
}

void ConfigManager::resetToDefaults() {
    display_pressure_unit = DISPLAY_PRESSURE_UNIT;
    display_temp_unit     = DISPLAY_TEMP_UNIT;
    alert_min_psi         = ALERT_MIN_PSI;
    alert_max_psi         = ALERT_MAX_PSI;
    alert_max_temp_c      = ALERT_MAX_TEMP_C;
    alert_min_batt        = ALERT_MIN_BATT;
    oled_sda_pin          = OLED_SDA_PIN;
    oled_scl_pin          = OLED_SCL_PIN;
    ap_ssid               = AP_SSID;
    ap_pass               = AP_PASS;
    enable_demo_mode      = ENABLE_DEMO_MODE;
    save();
}

String ConfigManager::getSettingsJson() {
    String json = "{";
    json += "\"press_unit\":" + String(display_pressure_unit) + ",";
    json += "\"temp_unit\":" + String(display_temp_unit) + ",";
    json += "\"min_psi\":" + String(alert_min_psi, 1) + ",";
    json += "\"max_psi\":" + String(alert_max_psi, 1) + ",";
    json += "\"max_temp\":" + String(alert_max_temp_c, 1) + ",";
    json += "\"min_batt\":" + String(alert_min_batt) + ",";
    json += "\"sda_pin\":" + String(oled_sda_pin) + ",";
    json += "\"scl_pin\":" + String(oled_scl_pin) + ",";
    json += "\"ap_ssid\":\"" + ap_ssid + "\",";
    json += "\"ap_pass\":\"" + ap_pass + "\",";
    json += "\"demo_mode\":" + String(enable_demo_mode ? "true" : "false");
    json += "}";
    return json;
}

bool ConfigManager::updateFromParams(const String& pressure_unit, const String& temp_unit,
                                    float min_psi, float max_psi, float max_temp, int min_batt,
                                    int sda_pin, int scl_pin, const String& ap_s, const String& ap_p, bool demo_mode) {
    if (pressure_unit == "BAR" || pressure_unit == "1") display_pressure_unit = UNIT_BAR;
    else if (pressure_unit == "KPA" || pressure_unit == "2") display_pressure_unit = UNIT_KPA;
    else display_pressure_unit = UNIT_PSI;

    if (temp_unit == "F" || temp_unit == "FAHRENHEIT" || temp_unit == "1") display_temp_unit = UNIT_FAHRENHEIT;
    else display_temp_unit = UNIT_CELSIUS;

    alert_min_psi    = min_psi;
    alert_max_psi    = max_psi;
    alert_max_temp_c = max_temp;
    alert_min_batt   = min_batt;
    oled_sda_pin     = sda_pin;
    oled_scl_pin     = scl_pin;
    if (ap_s.length() > 0) ap_ssid = ap_s;
    if (ap_p.length() >= 8) ap_pass = ap_p;
    enable_demo_mode = demo_mode;

    save();
    return true;
}
