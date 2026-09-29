#ifndef DISPLAY_DRIVER_ST7789_H
#define DISPLAY_DRIVER_ST7789_H

#include "Config.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_ST7789

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "TreelTPMS.h"

// 16-bit RGB565 Color Palette (Modern Automotive Theme)
#define ST7789_BG_COLOR       0x0842  // Obsidian Dark Background (#080C14)
#define ST7789_CARD_COLOR     0x11E6  // Dark Navy Glassmorphism Card (#131C2E)
#define ST7789_BORDER_COLOR   0x1D1A  // Card Border (#1E293B)
#define ST7789_CYAN_COLOR     0x3DFE  // Electric Cyan (#38BDF8)
#define ST7789_GREEN_COLOR    0x15D0  // Emerald Green (#10B981)
#define ST7789_RED_COLOR      0xF224  // Crimson Red (#EF4444)
#define ST7789_DARKRED_COLOR  0x7800  // Dark Alert Red Card Background (#800000)
#define ST7789_YELLOW_COLOR   0xFEC0  // Warning Amber (#FACC15)
#define ST7789_GRAY_COLOR     0x9517  // Muted Gray (#94A3B8)
#define ST7789_WHITE_COLOR    0xFFFF  // Pure White

class DisplayDriverST7789 {
private:
    Adafruit_ST7789 m_tft;
    bool m_initialized = false;
    bool m_headerDrawn = false;
    bool m_cardsDrawn = false;
    uint8_t m_lastAlertState[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    String m_lastIp = "";
    uint8_t m_lastUnit = 0xFF;

    // Last rendered string buffers for differential zero-flicker rendering
    char m_lastBatt[4][10];
    char m_lastTemp[4][12];
    char m_lastAge[4][14];
    char m_lastPsi[4][12];

    void renderCard(const TireData& tire, const char* posLabel, int x, int y, int w, int h, uint32_t now_ms);

public:
    DisplayDriverST7789();
    void begin();
    void render(const TireData tires[4]);
};

#endif // DISPLAY_TYPE check
#endif // DISPLAY_DRIVER_ST7789_H
