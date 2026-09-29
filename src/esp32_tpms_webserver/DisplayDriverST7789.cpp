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

    // Render Boot Screen
    m_tft.drawRect(4, 4, 312, 232, ST7789_CYAN_COLOR);
    m_tft.setCursor(35, 35);
    m_tft.setTextColor(ST7789_CYAN_COLOR, ST7789_BG_COLOR);
    m_tft.setTextSize(3);
    m_tft.print("TREEL TPMS BLE");

    m_tft.setCursor(20, 85);
    m_tft.setTextColor(ST7789_WHITE_COLOR, ST7789_BG_COLOR);
    m_tft.setTextSize(2);
    m_tft.print("2.0\" TFT (ST7789 320x240)");

    m_tft.setCursor(15, 125);
    m_tft.setTextColor(ST7789_GRAY_COLOR, ST7789_BG_COLOR);
    m_tft.setTextSize(2);
    m_tft.print("4-TIRE MONITORING SYSTEM");

    m_tft.setCursor(75, 175);
    m_tft.setTextColor(ST7789_YELLOW_COLOR, ST7789_BG_COLOR);
    m_tft.setTextSize(2);
    m_tft.print("INITIALIZING...");

    m_initialized = true;
    m_headerDrawn = false;
    m_cardsDrawn = false;
}

void DisplayDriverST7789::renderCard(const TireData& tire, const char* posLabel, int x, int y, int w, int h, uint32_t now_ms) {
    bool has_data = tire.has_received;
    AlertState alert = tire.getAlertState(ConfigMgr.alert_min_psi, ConfigMgr.alert_max_psi, ConfigMgr.alert_max_temp_c, ConfigMgr.alert_min_batt);
    bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

    uint16_t cardBg = is_alert ? ST7789_DARKRED_COLOR : ST7789_CARD_COLOR;
    uint16_t borderCol = is_alert ? ST7789_RED_COLOR : ST7789_BORDER_COLOR;
    uint16_t textMain = is_alert ? ST7789_YELLOW_COLOR : ST7789_WHITE_COLOR;
    uint16_t textSub  = is_alert ? ST7789_WHITE_COLOR : ST7789_GRAY_COLOR;

    // Track card index to draw card background & border ONLY on state transition (0% flicker)
    uint8_t posIdx = 0;
    if (posLabel[0] == 'F' && posLabel[1] == 'R') posIdx = 1;
    else if (posLabel[0] == 'R' && posLabel[1] == 'L') posIdx = 2;
    else if (posLabel[0] == 'R' && posLabel[1] == 'R') posIdx = 3;

    if (m_lastAlertState[posIdx] != (uint8_t)is_alert || !m_cardsDrawn) {
        m_tft.fillRect(x, y, w, h, cardBg);
        m_tft.drawRect(x, y, w, h, borderCol);
        m_lastAlertState[posIdx] = (uint8_t)is_alert;
    }

    // 1. Top-Left: Position Label (FL, FR, RL, RR)
    m_tft.setCursor(x + 10, y + 10);
    m_tft.setTextColor(ST7789_CYAN_COLOR, cardBg);
    m_tft.setTextSize(3);
    m_tft.print(posLabel);

    // 2. Top-Right: Battery Level (Size 2 text)
    char battBuf[10];
    if (has_data) snprintf(battBuf, sizeof(battBuf), "%3d%%", tire.battery_percent);
    else snprintf(battBuf, sizeof(battBuf), " --%%");
    m_tft.setTextSize(2);
    uint16_t battColor = (has_data && tire.battery_percent < ConfigMgr.alert_min_batt) ? ST7789_RED_COLOR : textSub;
    m_tft.setTextColor(battColor, cardBg);
    m_tft.setCursor(x + w - 10 - (strlen(battBuf) * 12), y + 12);
    m_tft.print(battBuf);

    // 3. Mid-Left: Temperature (Size 2 text)
    char tempBuf[10];
    if (ConfigMgr.display_temp_unit == UNIT_FAHRENHEIT) {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fF ", tire.temperature_f);
        else snprintf(tempBuf, sizeof(tempBuf), "--F ");
    } else {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fC ", tire.temperature_c);
        else snprintf(tempBuf, sizeof(tempBuf), "--C ");
    }
    m_tft.setTextSize(2);
    m_tft.setTextColor(textSub, cardBg);
    m_tft.setCursor(x + 10, y + 46);
    m_tft.print(tempBuf);

    // 4. Bottom-Left: Last Updated Age (Size 2 text, exact width so it never overlaps pressure digits)
    char ageBuf[12];
    if (has_data) {
        uint32_t diff = (now_ms - tire.last_updated_ms) / 1000;
        if (diff < 60) snprintf(ageBuf, sizeof(ageBuf), "%us ago", diff);
        else if (diff < 3600) snprintf(ageBuf, sizeof(ageBuf), "%um ago", diff / 60);
        else snprintf(ageBuf, sizeof(ageBuf), "%uh ago", diff / 3600);
    } else {
        snprintf(ageBuf, sizeof(ageBuf), "WAIT");
    }
    m_tft.setTextSize(2);
    m_tft.setTextColor(textSub, cardBg);
    m_tft.setCursor(x + 10, y + 78);
    m_tft.print(ageBuf);

    // 5. Right Side: Big Pressure Digits (Positioned at y + 35 so it stays safely above age text)
    char psiBuf[12];
    if (ConfigMgr.display_pressure_unit == UNIT_KPA) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%3.0f", tire.pressure_kpa);
        else snprintf(psiBuf, sizeof(psiBuf), " --");
        m_tft.setTextSize(4);
        m_tft.setCursor(x + w - 10 - (strlen(psiBuf) * 24), y + 42);
        m_tft.setTextColor(textMain, cardBg);
        m_tft.print(psiBuf);
    } else {
        if (ConfigMgr.display_pressure_unit == UNIT_BAR) {
            if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_bar);
            else snprintf(psiBuf, sizeof(psiBuf), " -- ");
        } else { // UNIT_PSI
            if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%d", (int)roundf(tire.pressure_psi));
            else snprintf(psiBuf, sizeof(psiBuf), "--");
        }
        m_tft.setTextSize(5);
        int digits = strlen(psiBuf);
        int psiX = x + w - 10 - (digits * 30);
        if (psiX < x + 75) psiX = x + 75;
        m_tft.setCursor(psiX, y + 36);
        m_tft.setTextColor(textMain, cardBg);
        m_tft.print(psiBuf);
    }
}

void DisplayDriverST7789::render(const TireData tires[4]) {
    if (!m_initialized) return;
    uint32_t now_ms = millis();

    // 1. TOP HEADER BAR (Draw background once, text updated with background color)
    if (!m_headerDrawn) {
        m_tft.fillRect(0, 0, 320, 25, ST7789_BG_COLOR);
        m_tft.drawFastHLine(0, 24, 320, ST7789_BORDER_COLOR);
        m_headerDrawn = true;
    }

    // Webserver IP address on top left (updated flicker-free)
    String ipStr = WebDash.getIpAddress();
    if (ipStr == "0.0.0.0" || ipStr.length() == 0) ipStr = "--";
    if (ipStr != m_lastIp) {
        char headerIpBuf[32];
        snprintf(headerIpBuf, sizeof(headerIpBuf), "IP: %-15s", ipStr.c_str());
        m_tft.setTextSize(2);
        m_tft.setTextColor(ST7789_CYAN_COLOR, ST7789_BG_COLOR);
        m_tft.setCursor(8, 5);
        m_tft.print(headerIpBuf);
        m_lastIp = ipStr;
    }

    // Active Pressure Unit on top right (updated flicker-free)
    if (m_lastUnit != ConfigMgr.display_pressure_unit) {
        const char* unitLabel = "PSI";
        if (ConfigMgr.display_pressure_unit == UNIT_BAR) unitLabel = "BAR";
        else if (ConfigMgr.display_pressure_unit == UNIT_KPA) unitLabel = "KPA";
        m_tft.setTextSize(2);
        m_tft.setTextColor(ST7789_YELLOW_COLOR, ST7789_BG_COLOR);
        m_tft.setCursor(312 - (strlen(unitLabel) * 12), 5);
        m_tft.print(unitLabel);
        m_lastUnit = ConfigMgr.display_pressure_unit;
    }

    // 2. 4-QUADRANT GRID (from y = 25 to y = 239)
    renderCard(tires[POS_FL], "FL", 0, 25, 160, 107, now_ms);
    renderCard(tires[POS_FR], "FR", 160, 25, 160, 107, now_ms);
    renderCard(tires[POS_RL], "RL", 0, 132, 160, 108, now_ms);
    renderCard(tires[POS_RR], "RR", 160, 132, 160, 108, now_ms);

    m_cardsDrawn = true;
}

#endif // DISPLAY_TYPE check
