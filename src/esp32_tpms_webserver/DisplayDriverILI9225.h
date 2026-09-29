#ifndef DISPLAY_DRIVER_ILI9225_H
#define DISPLAY_DRIVER_ILI9225_H

#include "Config.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_ILI9225

#include <Arduino.h>
#include <SPI.h>
#include "TreelTPMS.h"

// 16-bit RGB565 Color Palette
#define ILI9225_BLACK       0x0000
#define ILI9225_WHITE       0xFFFF
#define ILI9225_CYAN        0x3DFE
#define ILI9225_GREEN       0x15D0
#define ILI9225_RED         0xF800
#define ILI9225_DARKRED     0x7800
#define ILI9225_YELLOW      0xFFE0
#define ILI9225_BLUE        0x001F
#define ILI9225_BG          0x0863
#define ILI9225_CARD_BG     0x10E6
#define ILI9225_GRAY        0x9517
#define ILI9225_BORDER      0x1D9A

class DisplayDriverILI9225 {
private:
    bool m_initialized = false;

    void writeRegister(uint8_t reg, uint16_t data);
    void setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void initHardware();

    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
    void drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

    void drawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t scale);

    void renderCard(const TireData& tire, const char* posLabel, int x, int y, int w, int h, uint32_t now_ms);

public:
    DisplayDriverILI9225();
    void begin();
    void render(const TireData tires[4]);
};

#endif // DISPLAY_TYPE check
#endif // DISPLAY_DRIVER_ILI9225_H
