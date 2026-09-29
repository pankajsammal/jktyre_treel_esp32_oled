#include "ConfigManager.h"

DisplayManager Display;

DisplayManager::DisplayManager() {}

void DisplayManager::begin() {
#if DISPLAY_TYPE != DISPLAY_TYPE_NONE
    if (ConfigMgr.display_type != DISPLAY_TYPE_NONE) {
        m_driver.begin();
    }
#endif
}

void DisplayManager::render(const TireData tires[4]) {
#if DISPLAY_TYPE != DISPLAY_TYPE_NONE
    if (ConfigMgr.display_type != DISPLAY_TYPE_NONE) {
        m_driver.render(tires);
    }
#else
    (void)tires;
#endif
}
