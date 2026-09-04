#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "TreelTPMS.h"
#include "Config.h"

class DisplayManager {
private:
#if USE_SH1106_1_3_INCH
    U8G2_SH1106_128X64_NONAME_F_HW_I2C m_u8g2;
#else
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C m_u8g2;
#endif
    bool m_initialized = false;

    void renderCard(const TireData& tire, const char* posLabel, int x, int y, uint32_t now_ms);

public:
    DisplayManager();
    void begin();
    void render(const TireData tires[4]);
};

extern DisplayManager Display;

#endif // DISPLAY_MANAGER_H
