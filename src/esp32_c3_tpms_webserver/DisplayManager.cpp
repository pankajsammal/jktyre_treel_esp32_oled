#include "DisplayManager.h"
#include "ConfigManager.h"
#include "WebServerManager.h"

DisplayManager Display;

DisplayManager::DisplayManager() : m_u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}

void DisplayManager::begin() {
    if (!ENABLE_OLED) return;
    Wire.begin(ConfigMgr.oled_sda_pin, ConfigMgr.oled_scl_pin);
    m_u8g2.begin();
    m_u8g2.clearBuffer();
    m_u8g2.setFont(u8g2_font_7x14B_tr);
    m_u8g2.drawStr(12, 22, "TREEL TPMS C3");
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(18, 40, "SUPERMINI NODE");
    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(24, 56, "INITIALIZING...");
    m_u8g2.sendBuffer();
    m_initialized = true;
}

void DisplayManager::renderCard(const TireData& tire, const char* posLabel, int x, int y, uint32_t now_ms) {
    bool has_data = tire.has_received;
    AlertState alert = tire.getAlertState(ConfigMgr.alert_min_psi, ConfigMgr.alert_max_psi, ConfigMgr.alert_max_temp_c, ConfigMgr.alert_min_batt);
    bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

    int boxHeight = (y == 8) ? 27 : 28;

    if (is_alert) {
        m_u8g2.setDrawColor(1);
        m_u8g2.drawBox(x, y, 63, boxHeight);
        m_u8g2.setDrawColor(0);
    } else {
        m_u8g2.setDrawColor(1);
    }

    // 1. Top-Left: Position Label (FL, FR, RL, RR) - 3px left padding, 1px top gap
    m_u8g2.setFont(u8g2_font_profont11_tr);
    m_u8g2.drawStr(x + 3, y + 9, posLabel);

    // 2. Mid-Left: Temperature (e.g. 27C or 81F)
    char tempBuf[10];
    if (ConfigMgr.display_temp_unit == UNIT_FAHRENHEIT) {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fF", tire.temperature_f);
        else snprintf(tempBuf, sizeof(tempBuf), "--F");
    } else {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fC", tire.temperature_c);
        else snprintf(tempBuf, sizeof(tempBuf), "--C");
    }
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(x + 3, y + 18, tempBuf);

    // 3. Bottom-Left: Age (e.g. 50s, 2m, WAIT) - 2px gap above bottom border
    char ageBuf[8] = "WAIT";
    if (has_data) {
        uint32_t diff = (now_ms - tire.last_updated_ms) / 1000;
        if (diff < 60) snprintf(ageBuf, sizeof(ageBuf), "%us", diff);
        else if (diff < 3600) snprintf(ageBuf, sizeof(ageBuf), "%um", diff / 60);
        else snprintf(ageBuf, sizeof(ageBuf), "%uh", diff / 3600);
    }
    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(x + 3, y + 26, ageBuf);

    // 4. Right Side: BIG Pressure Digits - 3px right padding away from right border line
    char psiBuf[10] = "--";

    if (ConfigMgr.display_pressure_unit == UNIT_BAR) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_bar);
    } else if (ConfigMgr.display_pressure_unit == UNIT_KPA) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.0f", tire.pressure_kpa);
    } else { // UNIT_PSI
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%d", (int)roundf(tire.pressure_psi));
    }

    m_u8g2.setFont(u8g2_font_logisoso24_tn);
    int psiWidth = m_u8g2.getStrWidth(psiBuf);
    int psiX = x + 60 - psiWidth;
    if (psiX < x + 24) psiX = x + 24;
    int baselineY = (y == 8) ? 32 : 60;
    m_u8g2.drawStr(psiX, baselineY, psiBuf);
}

void DisplayManager::render(const TireData tires[4]) {
    if (!ENABLE_OLED || !m_initialized) return;
    m_u8g2.clearBuffer();
    uint32_t now_ms = millis();

    m_u8g2.setDrawColor(1);

    // 1. TOP HEADER BAR (y = 0 to 7, line at y = 7)
    m_u8g2.drawHLine(0, 7, 128);

    // Webserver IP on top left (baseline y = 6, 1px top margin)
    String ipStr = WebDash.getIpAddress();
    if (ipStr == "0.0.0.0" || ipStr.length() == 0) ipStr = "--";
    m_u8g2.setFont(u8g2_font_4x6_tr);
    m_u8g2.drawStr(1, 6, ipStr.c_str());

    // Global Pressure Unit on top right (baseline y = 6, 1px top margin)
    const char* unitLabel = "PSI";
    if (ConfigMgr.display_pressure_unit == UNIT_BAR) unitLabel = "BAR";
    else if (ConfigMgr.display_pressure_unit == UNIT_KPA) unitLabel = "KPA";

    m_u8g2.setFont(u8g2_font_4x6_tr);
    int unitW = m_u8g2.getStrWidth(unitLabel);
    m_u8g2.drawStr(127 - unitW, 6, unitLabel);

    // 2. 4-QUADRANT GRID (from y = 8 to y = 63, height = 56px)
    m_u8g2.drawVLine(63, 8, 56);
    m_u8g2.drawHLine(0, 35, 128);

    // Render each tire quadrant
    renderCard(tires[POS_FL], "FL", 0, 8, now_ms);
    renderCard(tires[POS_FR], "FR", 64, 8, now_ms);
    renderCard(tires[POS_RL], "RL", 0, 36, now_ms);
    renderCard(tires[POS_RR], "RR", 64, 36, now_ms);

    m_u8g2.sendBuffer();
}
