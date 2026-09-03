#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

class ConfigManager {
private:
    Preferences m_prefs;

public:
    // Runtime Configurable Parameters
    uint8_t display_pressure_unit;
    uint8_t display_temp_unit;
    float alert_min_psi;
    float alert_max_psi;
    float alert_max_temp_c;
    int alert_min_batt;
    int oled_sda_pin;
    int oled_scl_pin;
    String ap_ssid;
    String ap_pass;
    bool enable_demo_mode;

    ConfigManager();

    void begin();
    void save();
    void resetToDefaults();

    String getSettingsJson();
    bool updateFromParams(const String& pressure_unit, const String& temp_unit,
                         float min_psi, float max_psi, float max_temp, int min_batt,
                         int sda_pin, int scl_pin, const String& ap_s, const String& ap_p, bool demo_mode);
};

extern ConfigManager ConfigMgr;

#endif // CONFIG_MANAGER_H
