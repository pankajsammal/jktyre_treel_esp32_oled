#include "DisplayDriverST7789.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_ST7789

#include "ConfigManager.h"
#include "WebServerManager.h"

DisplayDriverST7789::DisplayDriverST7789()
    : m_tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN) {}

void DisplayDriverST7789::begin() {
    if (TFT_LED_PIN >= 0) {
        pinMode(TFT_LED_PIN, OUTPUT);
        digitalWrite(TFT_LED_PIN, HIGH); // Turn on backlight
    }

    // Initialize ST7789 240x320 display
    m_tft.init(240, 320);
    m_tft.setRotation(1); // 320 wide x 240 high Landscape mode
    m_tft.fillScreen(ST7789_BG_COLOR);

    // Render Boot Screen with smooth FreeSans typography
    m_tft.drawRoundRect(8, 8, 304, 224, 12, ST7789_CYAN_COLOR);

    m_tft.setFont(&FreeSansBold12pt7b);
    m_tft.setTextColor(ST7789_CYAN_COLOR);
    m_tft.setCursor(45, 55);
    m_tft.print("TREEL TPMS BLE");

    m_tft.setFont(&FreeSansBold9pt7b);
    m_tft.setTextColor(ST7789_WHITE_COLOR);
    m_tft.setCursor(35, 105);
    m_tft.print("2.0\" TFT (ST7789 320x240)");

    m_tft.setFont(&FreeSans9pt7b);
    m_tft.setTextColor(ST7789_GRAY_COLOR);
    m_tft.setCursor(30, 145);
    m_tft.print("4-TIRE MONITORING SYSTEM");

    m_tft.setFont(&FreeSansBold9pt7b);
    m_tft.setTextColor(ST7789_YELLOW_COLOR);
    m_tft.setCursor(90, 190);
    m_tft.print("INITIALIZING...");

    m_initialized = true;
    m_headerDrawn = false;
    m_cardsDrawn = false;
    m_lastIp = "";
    m_lastUnit = 0xFF;
    for (int i = 0; i < 4; i++) {
        m_lastAlertState[i] = 0xFF;
        m_lastBatt[i][0] = '\0';
        m_lastTemp[i][0] = '\0';
        m_lastAge[i][0] = '\0';
        m_lastPsi[i][0] = '\0';
    }
}

void DisplayDriverST7789::renderCard(const TireData& tire, const char* posLabel, int x, int y, int w, int h, uint32_t now_ms) {
    bool has_data = tire.has_received;
    AlertState alert = tire.getAlertState(ConfigMgr.alert_min_psi, ConfigMgr.alert_max_psi, ConfigMgr.alert_max_temp_c, ConfigMgr.alert_min_batt);
    bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

    uint16_t cardBg = is_alert ? ST7789_DARKRED_COLOR : ST7789_CARD_COLOR;
    uint16_t borderCol = is_alert ? ST7789_RED_COLOR : ST7789_BORDER_COLOR;
    uint16_t textMain = is_alert ? ST7789_YELLOW_COLOR : ST7789_WHITE_COLOR;
    uint16_t textSub  = is_alert ? ST7789_WHITE_COLOR : ST7789_GRAY_COLOR;

    // Determine card index for state transition tracking
    uint8_t posIdx = 0;
    if (posLabel[0] == 'F' && posLabel[1] == 'R') posIdx = 1;
    else if (posLabel[0] == 'R' && posLabel[1] == 'L') posIdx = 2;
    else if (posLabel[0] == 'R' && posLabel[1] == 'R') posIdx = 3;

    bool alertStateChanged = (m_lastAlertState[posIdx] != (uint8_t)is_alert);

    // 1. Draw Glassmorphism Card Base & Position Label ONLY on first draw or alert state change
    if (!m_cardsDrawn || alertStateChanged) {
        m_tft.fillRoundRect(x, y, w, h, 8, cardBg);
        m_tft.drawRoundRect(x, y, w, h, 8, borderCol);
        m_lastAlertState[posIdx] = (uint8_t)is_alert;

        // Position Label (FL, FR, RL, RR)
        m_tft.setFont(&FreeSansBold12pt7b);
        m_tft.setTextColor(ST7789_CYAN_COLOR);
        m_tft.setCursor(x + 12, y + 26);
        m_tft.print(posLabel);

        // Clear tracking buffers so all dynamic fields get rendered on the fresh background
        m_lastBatt[posIdx][0] = '\0';
        m_lastTemp[posIdx][0] = '\0';
        m_lastAge[posIdx][0] = '\0';
        m_lastPsi[posIdx][0] = '\0';
    }

    // 2. Battery Level - Redraw ONLY when string changes
    char battBuf[10];
    if (has_data) snprintf(battBuf, sizeof(battBuf), "%d%%", tire.battery_percent);
    else snprintf(battBuf, sizeof(battBuf), "--%%");

    if (strcmp(battBuf, m_lastBatt[posIdx]) != 0) {
        int16_t bx1, by1;
        uint16_t bw, bh;
        m_tft.setFont(&FreeSansBold9pt7b);
        m_tft.getTextBounds(battBuf, 0, 0, &bx1, &by1, &bw, &bh);
        uint16_t battColor = (has_data && tire.battery_percent < ConfigMgr.alert_min_batt) ? ST7789_RED_COLOR : textSub;

        m_tft.fillRect(x + w - 65, y + 6, 55, 22, cardBg);
        m_tft.setTextColor(battColor);
        m_tft.setCursor(x + w - 12 - bw, y + 22);
        m_tft.print(battBuf);
        snprintf(m_lastBatt[posIdx], sizeof(m_lastBatt[posIdx]), "%s", battBuf);
    }

    // 3. Temperature - Redraw ONLY when string changes
    char tempBuf[12];
    if (ConfigMgr.display_temp_unit == UNIT_FAHRENHEIT) {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0f F", tire.temperature_f);
        else snprintf(tempBuf, sizeof(tempBuf), "-- F");
    } else {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0f C", tire.temperature_c);
        else snprintf(tempBuf, sizeof(tempBuf), "-- C");
    }

    if (strcmp(tempBuf, m_lastTemp[posIdx]) != 0) {
        m_tft.setFont(&FreeSans9pt7b);
        m_tft.fillRect(x + 10, y + 36, 62, 20, cardBg);
        m_tft.setTextColor(textSub);
        m_tft.setCursor(x + 12, y + 52);
        m_tft.print(tempBuf);
        snprintf(m_lastTemp[posIdx], sizeof(m_lastTemp[posIdx]), "%s", tempBuf);
    }

    // 4. Last Updated Age - Redraw ONLY when string changes
    char ageBuf[14];
    if (has_data) {
        uint32_t diff = (now_ms - tire.last_updated_ms) / 1000;
        if (diff < 60) snprintf(ageBuf, sizeof(ageBuf), "%us ago", diff);
        else if (diff < 3600) snprintf(ageBuf, sizeof(ageBuf), "%um ago", diff / 60);
        else snprintf(ageBuf, sizeof(ageBuf), "%uh ago", diff / 3600);
    } else {
        snprintf(ageBuf, sizeof(ageBuf), "WAIT");
    }

    if (strcmp(ageBuf, m_lastAge[posIdx]) != 0) {
        m_tft.setFont(&FreeSans9pt7b);
        m_tft.fillRect(x + 10, y + 62, 70, 24, cardBg);
        m_tft.setTextColor(textSub);
        m_tft.setCursor(x + 12, y + 78);
        m_tft.print(ageBuf);
        snprintf(m_lastAge[posIdx], sizeof(m_lastAge[posIdx]), "%s", ageBuf);
    }

    // 5. Big Pressure Digits - Redraw ONLY when string changes
    char psiBuf[12];
    if (ConfigMgr.display_pressure_unit == UNIT_KPA) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.0f", tire.pressure_kpa);
        else snprintf(psiBuf, sizeof(psiBuf), "--");
    } else if (ConfigMgr.display_pressure_unit == UNIT_BAR) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_bar);
        else snprintf(psiBuf, sizeof(psiBuf), "--");
    } else { // UNIT_PSI
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%d", (int)roundf(tire.pressure_psi));
        else snprintf(psiBuf, sizeof(psiBuf), "--");
    }

    if (strcmp(psiBuf, m_lastPsi[posIdx]) != 0) {
        m_tft.setFont(&FreeSansBold24pt7b);
        int16_t px1, py1;
        uint16_t pw, ph;
        m_tft.getTextBounds(psiBuf, 0, 0, &px1, &py1, &pw, &ph);

        m_tft.fillRect(x + 75, y + 28, w - 85, 55, cardBg);

        int psiX = x + w - 12 - pw;
        if (psiX < x + 75) psiX = x + 75;

        m_tft.setTextColor(textMain);
        m_tft.setCursor(psiX, y + 70);
        m_tft.print(psiBuf);
        snprintf(m_lastPsi[posIdx], sizeof(m_lastPsi[posIdx]), "%s", psiBuf);
    }
}

void DisplayDriverST7789::render(const TireData tires[4]) {
    if (!m_initialized) return;
    uint32_t now_ms = millis();

    // 1. TOP HEADER BAR (Draw background once and clear boot screen text)
    if (!m_headerDrawn) {
        m_tft.fillScreen(ST7789_BG_COLOR);
        m_tft.fillRect(0, 0, 320, 26, ST7789_BG_COLOR);
        m_tft.drawFastHLine(0, 26, 320, ST7789_BORDER_COLOR);
        m_headerDrawn = true;
    }

    // Webserver IP address on top left (updated flicker-free)
    String ipStr = WebDash.getIpAddress();
    if (ipStr == "0.0.0.0" || ipStr.length() == 0) ipStr = "--";
    if (ipStr != m_lastIp) {
        char headerIpBuf[32];
        snprintf(headerIpBuf, sizeof(headerIpBuf), "IP: %s", ipStr.c_str());
        m_tft.setFont(&FreeSansBold9pt7b);
        m_tft.fillRect(6, 4, 180, 18, ST7789_BG_COLOR);
        m_tft.setTextColor(ST7789_CYAN_COLOR);
        m_tft.setCursor(6, 18);
        m_tft.print(headerIpBuf);
        m_lastIp = ipStr;
    }

    // Active Pressure Unit on top right (updated flicker-free)
    if (m_lastUnit != ConfigMgr.display_pressure_unit) {
        const char* unitLabel = "PSI";
        if (ConfigMgr.display_pressure_unit == UNIT_BAR) unitLabel = "BAR";
        else if (ConfigMgr.display_pressure_unit == UNIT_KPA) unitLabel = "KPA";

        int16_t ux1, uy1;
        uint16_t uw, uh;
        m_tft.setFont(&FreeSansBold9pt7b);
        m_tft.getTextBounds(unitLabel, 0, 0, &ux1, &uy1, &uw, &uh);

        m_tft.fillRect(250, 4, 64, 18, ST7789_BG_COLOR);
        m_tft.setTextColor(ST7789_YELLOW_COLOR);
        m_tft.setCursor(312 - uw, 18);
        m_tft.print(unitLabel);
        m_lastUnit = ConfigMgr.display_pressure_unit;

        // Force text redraws when unit changes
        for (int i = 0; i < 4; i++) {
            m_lastTemp[i][0] = '\0';
            m_lastPsi[i][0] = '\0';
        }
    }

    // 2. 4-QUADRANT GRID WITH SMOOTH ROUNDED CARDS
    // Top-Left (FL)
    renderCard(tires[POS_FL], "FL", 6, 32, 150, 96, now_ms);
    // Top-Right (FR)
    renderCard(tires[POS_FR], "FR", 164, 32, 150, 96, now_ms);
    // Bottom-Left (RL)
    renderCard(tires[POS_RL], "RL", 6, 134, 150, 96, now_ms);
    // Bottom-Right (RR)
    renderCard(tires[POS_RR], "RR", 164, 134, 150, 96, now_ms);

    m_cardsDrawn = true;
}

#endif // DISPLAY_TYPE check
