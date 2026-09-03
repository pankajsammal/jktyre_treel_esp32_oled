#include "DisplayManager.h"

DisplayManager Display;

DisplayManager::DisplayManager() : m_u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}

String DisplayManager::formatAge(unsigned long last_ms, unsigned long now_ms) {
    if (last_ms == 0) return "WAIT";
    unsigned long diff = (now_ms - last_ms) / 1000;
    if (diff < 60) return String(diff) + "s";
    if (diff < 3600) return String(diff / 60) + "m";
    if (diff < 86400) return String(diff / 3600) + "h";
    return String(diff / 86400) + "d";
}

void DisplayManager::begin() {
    if (!ENABLE_OLED) return;

    if (OLED_VCC_PIN >= 0) {
        pinMode(OLED_VCC_PIN, OUTPUT);
        digitalWrite(OLED_VCC_PIN, HIGH);
        delay(50);
    }

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    m_u8g2.begin();
    m_u8g2.clearBuffer();

    m_u8g2.setFont(u8g2_font_7x14B_tr);
    m_u8g2.drawStr(14, 22, "TREEL TPMS BLE");
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(18, 40, "4-TIRE MONITOR");
    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(24, 56, "INITIALIZING...");
    m_u8g2.sendBuffer();
    m_initialized = true;
}

void DisplayManager::renderCard(const TireData& tire, const char* posLabel, int x, int y, unsigned long now_ms) {
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

    // 1. Top-Left: Position Label (FL, FR, RL, RR)
    m_u8g2.setFont(u8g2_font_7x14B_tr);
    m_u8g2.drawStr(x + 2, y + 11, posLabel);

    // 2. Mid-Left: Temperature (e.g. 27C)
    char tempBuf[10];
    if (has_data) {
        snprintf(tempBuf, sizeof(tempBuf), "%.0fC", tire.temperature_c);
    } else {
        snprintf(tempBuf, sizeof(tempBuf), "--C");
    }
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(x + 2, y + 20, tempBuf);

    // 3. Bottom-Left: Age (e.g. 50s, 2m, WAIT)
    String ageStr = has_data ? formatAge(tire.last_updated_ms, now_ms) : "WAIT";
    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(x + 2, y + 29, ageStr.c_str());

    // 4. Right Side: Big Pressure Digits (e.g. 32)
    char psiBuf[10] = "--";
    if (has_data) {
        if (tire.pressure_psi == (float)(int)tire.pressure_psi) {
            snprintf(psiBuf, sizeof(psiBuf), "%d", (int)tire.pressure_psi);
        } else {
            snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_psi);
        }
    }
    m_u8g2.setFont(u8g2_font_logisoso22_tn);
    int psiWidth = m_u8g2.getStrWidth(psiBuf);
    int psiX = x + 62 - psiWidth;
    if (psiX < x + 24) psiX = x + 24;
    m_u8g2.drawStr(psiX, y + 25, psiBuf);

    // 5. Bottom-Right: Small Unit (PSI)
    m_u8g2.setFont(u8g2_font_4x6_tr);
    m_u8g2.drawStr(x + 48, y + 30, "PSI");
}

void DisplayManager::render(const TireData tires[4]) {
    if (!ENABLE_OLED || !m_initialized) return;

    m_u8g2.clearBuffer();
    unsigned long now_ms = millis();

    // Draw 4-quadrant divider grid
    m_u8g2.setDrawColor(1);
    m_u8g2.drawVLine(63, 0, 64);
    m_u8g2.drawHLine(0, 31, 128);

    // Render each tire quadrant
    renderCard(tires[POS_FL], "FL", 0, 0, now_ms);
    renderCard(tires[POS_FR], "FR", 64, 0, now_ms);
    renderCard(tires[POS_RL], "RL", 0, 32, now_ms);
    renderCard(tires[POS_RR], "RR", 64, 32, now_ms);

    m_u8g2.sendBuffer();
}
