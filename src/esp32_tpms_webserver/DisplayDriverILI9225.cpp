#include "DisplayDriverILI9225.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_ILI9225

#include "ConfigManager.h"
#include "WebServerManager.h"

// ---------------------------------------------------------------------
// EMBEDDED 5x7 ASCII FONT BITMAP TABLE (Space ' ' to '~')
// ---------------------------------------------------------------------
static const uint8_t font5x7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x26, 0x49, 0x49, 0x49, 0x32}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3E, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x02, 0x01, 0x02, 0x04, 0x02}  // 126 '~'
};

DisplayDriverILI9225::DisplayDriverILI9225() {}

void DisplayDriverILI9225::writeRegister(uint8_t reg, uint16_t data) {
    digitalWrite(TFT_CS_PIN, LOW);
    
    // Command byte (DC = LOW)
    digitalWrite(TFT_DC_PIN, LOW);
    SPI.transfer(0x00);
    SPI.transfer(reg);
    
    // Data byte (DC = HIGH)
    digitalWrite(TFT_DC_PIN, HIGH);
    SPI.transfer(data >> 8);
    SPI.transfer(data & 0xFF);
    
    digitalWrite(TFT_CS_PIN, HIGH);
}

void DisplayDriverILI9225::setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Map landscape coordinates (x: 0..219, y: 0..175) to physical TFT (px: 0..175, py: 0..219)
    uint16_t px1 = y1;
    uint16_t px2 = y2;
    uint16_t py1 = 219 - x2;
    uint16_t py2 = 219 - x1;

    writeRegister(0x36, px2); // Horizontal Window Address 1
    writeRegister(0x37, px1); // Horizontal Window Address 2
    writeRegister(0x38, py2); // Vertical Window Address 1
    writeRegister(0x39, py1); // Vertical Window Address 2
    writeRegister(0x20, px1); // RAM Address X
    writeRegister(0x21, py1); // RAM Address Y

    // Start RAM Data Write
    digitalWrite(TFT_CS_PIN, LOW);
    digitalWrite(TFT_DC_PIN, LOW);
    SPI.transfer(0x00);
    SPI.transfer(0x22); // Write Data to GRAM command
    digitalWrite(TFT_DC_PIN, HIGH);
}

void DisplayDriverILI9225::initHardware() {
    pinMode(TFT_CS_PIN, OUTPUT);
    pinMode(TFT_DC_PIN, OUTPUT);
    pinMode(TFT_RST_PIN, OUTPUT);
    digitalWrite(TFT_CS_PIN, HIGH);
    digitalWrite(TFT_DC_PIN, HIGH);

    if (TFT_LED_PIN >= 0) {
        pinMode(TFT_LED_PIN, OUTPUT);
        digitalWrite(TFT_LED_PIN, HIGH); // Turn on backlight
    }

    // Hardware Reset Pulse
    digitalWrite(TFT_RST_PIN, HIGH);
    delay(10);
    digitalWrite(TFT_RST_PIN, LOW);
    delay(30);
    digitalWrite(TFT_RST_PIN, HIGH);
    delay(100);

    // Initialize SPI
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
    SPI.begin(TFT_SCK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
#else
    SPI.begin(TFT_SCK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
#endif
    SPI.setFrequency(20000000); // 20 MHz Hardware SPI speed

    // ILI9225 Power & Display Register Initialization
    writeRegister(0x10, 0x0000);
    writeRegister(0x11, 0x0000);
    writeRegister(0x12, 0x0000);
    writeRegister(0x13, 0x0000);
    writeRegister(0x14, 0x0000);
    delay(40);

    writeRegister(0x11, 0x0018);
    writeRegister(0x12, 0x6121);
    writeRegister(0x13, 0x006F);
    writeRegister(0x14, 0x495F);
    writeRegister(0x10, 0x0800);
    delay(50);

    writeRegister(0x11, 0x103B);
    delay(50);

    writeRegister(0x01, 0x011C); // Driver Output Control (NL=220, SS=1)
    writeRegister(0x02, 0x0100); // LCD AC Driving Control
    writeRegister(0x03, 0x1030); // Entry Mode (BGR=1, Auto Increment)
    writeRegister(0x07, 0x0000); // Display Control 1
    writeRegister(0x08, 0x0808); // Blank Period Control
    writeRegister(0x0B, 0x1100); // Frame Cycle Control
    writeRegister(0x0C, 0x0000); // Interface Control
    writeRegister(0x0F, 0x0D01); // Osc Control
    writeRegister(0x15, 0x0020); // VCI Recycling
    writeRegister(0x20, 0x0000); // RAM Address Set X
    writeRegister(0x21, 0x0000); // RAM Address Set Y

    writeRegister(0x07, 0x1017); // Display ON
    delay(50);

    fillRect(0, 0, 220, 176, ILI9225_BG);
}

void DisplayDriverILI9225::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= 220 || y >= 176) return;
    setWindow(x, y, x, y);
    SPI.transfer(color >> 8);
    SPI.transfer(color & 0xFF);
    digitalWrite(TFT_CS_PIN, HIGH);
}

void DisplayDriverILI9225::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= 220 || y >= 176) return;
    if (x + w > 220) w = 220 - x;
    if (y + h > 176) h = 176 - y;

    setWindow(x, y, x + w - 1, y + h - 1);

    uint32_t totalPixels = (uint32_t)w * h;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (uint32_t i = 0; i < totalPixels; i++) {
        SPI.transfer(hi);
        SPI.transfer(lo);
    }

    digitalWrite(TFT_CS_PIN, HIGH);
}

void DisplayDriverILI9225::drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
}

void DisplayDriverILI9225::drawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    fillRect(x, y, w, 1, color);
}

void DisplayDriverILI9225::drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    fillRect(x, y, 1, h, color);
}

void DisplayDriverILI9225::drawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < 32 || c > 126) c = '?';
    uint8_t idx = c - 32;

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = pgm_read_byte(&font5x7[idx][col]);
        for (uint8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                if (scale == 1) {
                    drawPixel(x + col, y + row, fg);
                } else {
                    fillRect(x + col * scale, y + row * scale, scale, scale, fg);
                }
            } else if (bg != fg) { // Fill background pixel if specified
                if (scale == 1) {
                    drawPixel(x + col, y + row, bg);
                } else {
                    fillRect(x + col * scale, y + row * scale, scale, scale, bg);
                }
            }
        }
    }
}

void DisplayDriverILI9225::drawString(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale) {
    uint16_t curX = x;
    while (*str) {
        drawChar(curX, y, *str, fg, bg, scale);
        curX += 6 * scale;
        str++;
    }
}

void DisplayDriverILI9225::begin() {
    initHardware();

    // Render Boot Screen (220x176 Landscape)
    fillRect(0, 0, 220, 176, ILI9225_BG);
    drawRect(4, 4, 212, 168, ILI9225_CYAN);
    
    drawString(28, 25, "TREEL TPMS BLE", ILI9225_CYAN, ILI9225_BG, 2);
    drawString(22, 65, "2.0\" TFT DISPLAY (ILI9225)", ILI9225_WHITE, ILI9225_BG, 1);
    drawString(28, 95, "4-TIRE MONITORING SYSTEM", ILI9225_GRAY, ILI9225_BG, 1);
    drawString(50, 135, "INITIALIZING...", ILI9225_YELLOW, ILI9225_BG, 1);

    m_initialized = true;
}

void DisplayDriverILI9225::renderCard(const TireData& tire, const char* posLabel, int x, int y, int w, int h, uint32_t now_ms) {
    bool has_data = tire.has_received;
    AlertState alert = tire.getAlertState(ConfigMgr.alert_min_psi, ConfigMgr.alert_max_psi, ConfigMgr.alert_max_temp_c, ConfigMgr.alert_min_batt);
    bool is_alert = has_data && (alert != ALERT_NORMAL && alert != ALERT_WAITING);

    uint16_t cardBg = is_alert ? ILI9225_DARKRED : ILI9225_CARD_BG;
    uint16_t borderCol = is_alert ? ILI9225_RED : ILI9225_BORDER;
    uint16_t textMain = is_alert ? ILI9225_YELLOW : ILI9225_WHITE;
    uint16_t textSub  = is_alert ? ILI9225_WHITE : ILI9225_GRAY;

    // Draw Card Background & Outer Border
    fillRect(x, y, w, h, cardBg);
    drawRect(x, y, w, h, borderCol);

    // 1. Top-Left: Position Label (FL, FR, RL, RR)
    drawString(x + 6, y + 6, posLabel, ILI9225_CYAN, cardBg, 2);

    // 2. Top-Right: Battery Level & Alert Badge
    char battBuf[10];
    if (has_data) snprintf(battBuf, sizeof(battBuf), "%d%%", tire.battery_percent);
    else snprintf(battBuf, sizeof(battBuf), "--%%");
    uint16_t battColor = (has_data && tire.battery_percent < ConfigMgr.alert_min_batt) ? ILI9225_RED : textSub;
    drawString(x + w - 6 - (strlen(battBuf) * 6), y + 6, battBuf, battColor, cardBg, 1);

    // 3. Mid-Left: Temperature (e.g., 28C / 82F)
    char tempBuf[10];
    if (ConfigMgr.display_temp_unit == UNIT_FAHRENHEIT) {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fF", tire.temperature_f);
        else snprintf(tempBuf, sizeof(tempBuf), "--F");
    } else {
        if (has_data) snprintf(tempBuf, sizeof(tempBuf), "%.0fC", tire.temperature_c);
        else snprintf(tempBuf, sizeof(tempBuf), "--C");
    }
    drawString(x + 6, y + 32, tempBuf, textSub, cardBg, 1);

    // 4. Bottom-Left: Last Updated Age (e.g. 12s, 2m, WAIT)
    char ageBuf[12] = "WAIT";
    if (has_data) {
        uint32_t diff = (now_ms - tire.last_updated_ms) / 1000;
        if (diff < 60) snprintf(ageBuf, sizeof(ageBuf), "%us ago", diff);
        else if (diff < 3600) snprintf(ageBuf, sizeof(ageBuf), "%um ago", diff / 60);
        else snprintf(ageBuf, sizeof(ageBuf), "%uh ago", diff / 3600);
    }
    drawString(x + 6, y + 58, ageBuf, textSub, cardBg, 1);

    // 5. Right Side: Big Pressure Digits
    char psiBuf[12] = "--";
    if (ConfigMgr.display_pressure_unit == UNIT_KPA) {
        if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.0f", tire.pressure_kpa);
        int psiX = x + w - 8 - (strlen(psiBuf) * 18);
        if (psiX < x + 42) psiX = x + 42;
        drawString(psiX, y + 30, psiBuf, textMain, cardBg, 3);
    } else {
        if (ConfigMgr.display_pressure_unit == UNIT_BAR) {
            if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%.1f", tire.pressure_bar);
        } else { // UNIT_PSI
            if (has_data) snprintf(psiBuf, sizeof(psiBuf), "%d", (int)roundf(tire.pressure_psi));
        }
        int psiX = x + w - 8 - (strlen(psiBuf) * 24);
        if (psiX < x + 42) psiX = x + 42;
        drawString(psiX, y + 26, psiBuf, textMain, cardBg, 4);
    }
}

void DisplayDriverILI9225::render(const TireData tires[4]) {
    if (!m_initialized) return;
    uint32_t now_ms = millis();

    // 1. TOP HEADER BAR (y = 0 to 18)
    fillRect(0, 0, 220, 18, ILI9225_BG);
    drawHLine(0, 18, 220, ILI9225_BORDER);

    // Webserver IP address on top left
    String ipStr = WebDash.getIpAddress();
    if (ipStr == "0.0.0.0" || ipStr.length() == 0) ipStr = "--";
    char headerIpBuf[32];
    snprintf(headerIpBuf, sizeof(headerIpBuf), "IP: %s", ipStr.c_str());
    drawString(4, 5, headerIpBuf, ILI9225_CYAN, ILI9225_BG, 1);

    // Active Pressure Unit on top right
    const char* unitLabel = "PSI";
    if (ConfigMgr.display_pressure_unit == UNIT_BAR) unitLabel = "BAR";
    else if (ConfigMgr.display_pressure_unit == UNIT_KPA) unitLabel = "KPA";
    drawString(216 - (strlen(unitLabel) * 6), 5, unitLabel, ILI9225_YELLOW, ILI9225_BG, 1);

    // 2. 4-QUADRANT GRID (from y = 19 to y = 175)
    // Top-Left (FL)
    renderCard(tires[POS_FL], "FL", 0, 19, 110, 78, now_ms);
    // Top-Right (FR)
    renderCard(tires[POS_FR], "FR", 110, 19, 110, 78, now_ms);
    // Bottom-Left (RL)
    renderCard(tires[POS_RL], "RL", 0, 97, 110, 79, now_ms);
    // Bottom-Right (RR)
    renderCard(tires[POS_RR], "RR", 110, 97, 110, 79, now_ms);
}

#endif // DISPLAY_TYPE check
