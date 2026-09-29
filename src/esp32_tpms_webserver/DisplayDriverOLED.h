#ifndef DISPLAY_DRIVER_OLED_H
#define DISPLAY_DRIVER_OLED_H

#include "Config.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_SSD1306 || DISPLAY_TYPE == DISPLAY_TYPE_SH1106

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "TreelTPMS.h"

class DisplayDriverOLED {
private:
#if DISPLAY_TYPE == DISPLAY_TYPE_SH1106
    U8G2_SH1106_128X64_NONAME_F_HW_I2C m_u8g2;
#else
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C m_u8g2;
#endif
    bool m_initialized = false;

    void renderCard(const TireData& tire, const char* posLabel, int x, int y, uint32_t now_ms);

public:
    DisplayDriverOLED();
    void begin();
    void render(const TireData tires[4]);
};

#endif // DISPLAY_TYPE check
#endif // DISPLAY_DRIVER_OLED_H
