#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "TreelTPMS.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_ST7789
#include "DisplayDriverST7789.h"
#elif DISPLAY_TYPE == DISPLAY_TYPE_ILI9225
#include "DisplayDriverILI9225.h"
#elif DISPLAY_TYPE == DISPLAY_TYPE_SSD1306 || DISPLAY_TYPE == DISPLAY_TYPE_SH1106
#include "DisplayDriverOLED.h"
#endif

class DisplayManager {
private:
#if DISPLAY_TYPE == DISPLAY_TYPE_ST7789
    DisplayDriverST7789 m_driver;
#elif DISPLAY_TYPE == DISPLAY_TYPE_ILI9225
    DisplayDriverILI9225 m_driver;
#elif DISPLAY_TYPE == DISPLAY_TYPE_SSD1306 || DISPLAY_TYPE == DISPLAY_TYPE_SH1106
    DisplayDriverOLED m_driver;
#endif

public:
    DisplayManager();
    void begin();
    void render(const TireData tires[4]);
};

extern DisplayManager Display;

#endif // DISPLAY_MANAGER_H
