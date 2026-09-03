#include "DisplayManager.h"

DisplayManager Display;

DisplayManager::DisplayManager() : m_u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}

void DisplayManager::begin() {
    if (!ENABLE_OLED) return;
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    m_u8g2.begin();
    m_u8g2.clearBuffer();
    m_u8g2.setFont(u8g2_font_ncenB08_tr);
    m_u8g2.drawStr(12, 22, "TREEL TPMS C3");
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(18, 40, "SUPERMINI NODE");
    m_u8g2.setFont(u8g2_font_micro_tr);
    m_u8g2.drawStr(24, 58, "INITIALIZING...");
    m_u8g2.sendBuffer();
    m_initialized = true;
}

void DisplayManager::renderCard(const TireData& tire, const char* posLabel, int x, int y, uint32_t now_ms) {
    bool has_data = tire.has_received;
    AlertState alert = tire.getAlertState();
    bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

    if (is_alert) {
        m_u8g2.setDrawColor(1);
        m_u8g2.drawBox(x, y, 63, 31);
        m_u8g2.setDrawColor(0);
    } else {
        m_u8g2.setDrawColor(1);
    }

    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(x + 2, y + 9, posLabel);

    char ageBuf[8] = "WAIT";
    if (has_data) {
        uint32_t diff = (now_ms - tire.last_updated_ms) / 1000;
        if (diff < 60) snprintf(ageBuf, sizeof(ageBuf), "%us", diff);
        else if (diff < 3600) snprintf(ageBuf, sizeof(ageBuf), "%um", diff / 60);
        else snprintf(ageBuf, sizeof(ageBuf), "%uh", diff / 3600);
    }
    m_u8g2.setFont(u8g2_font_5x8_tr);
    int ageW = m_u8g2.getStrWidth(ageBuf);
    m_u8g2.drawStr(x + 61 - ageW, y + 8, ageBuf);

    if (!has_data) {
        m_u8g2.setFont(u8g2_font_7x14B_tr);
        m_u8g2.drawStr(x + 8, y + 21, "--.- P");
        m_u8g2.setFont(u8g2_font_5x8_tr);
        m_u8g2.drawStr(x + 2, y + 30, "-- C");
        m_u8g2.drawStr(x + 44, y + 30, "WAIT");
        return;
    }

    char psiBuf[10];
    snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_psi);
    m_u8g2.setFont(u8g2_font_7x14B_tr);
    m_u8g2.drawStr(x + 6, y + 21, psiBuf);

    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(x + 44, y + 18, "P");

    char tempBuf[10];
    snprintf(tempBuf, sizeof(tempBuf), "%.0fC", tire.temperature_c);
    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(x + 2, y + 30, tempBuf);

    if (is_alert) m_u8g2.drawStr(x + 36, y + 30, "!ALT");
    else m_u8g2.drawStr(x + 46, y + 30, "OK");
}

void DisplayManager::render(const TireData tires[4]) {
    if (!ENABLE_OLED || !m_initialized) return;
    m_u8g2.clearBuffer();
    uint32_t now_ms = millis();

    m_u8g2.setDrawColor(1);
    m_u8g2.drawVLine(63, 0, 64);
    m_u8g2.drawHLine(0, 31, 128);

    renderCard(tires[POS_FL], "FL", 0, 0, now_ms);
    renderCard(tires[POS_FR], "FR", 64, 0, now_ms);
    renderCard(tires[POS_RL], "RL", 0, 32, now_ms);
    renderCard(tires[POS_RR], "RR", 64, 32, now_ms);

    m_u8g2.sendBuffer();
}
