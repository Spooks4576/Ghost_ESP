#include "system_manager.h"
#include "../core/commandline.h"
#include <components/gps_module/gps_module.h>

void SystemManager::initGPSModule()
{
#ifdef HAS_GPS
    gpsModule = new gps_module();
#endif
}

void SystemManager::setup()
{
    Serial.begin(115200);
    initDisplay();
    initLEDs();
    initSDCard();
    initBLE();
    initWiFi();
    initGPSModule();
    CommandLine::getInstance().RunSetup();

    Settings.loadSettings();

    if (Settings.getRGBMode() == FSettings::RGBMode::Rainbow)
    {
        SystemManager::getInstance().RainbowLEDActive = true;
    }

    Serial.println("System initialized");
}

void SystemManager::loop()
{
#ifdef DISPLAY_SUPPORT
    lv_tick_inc(millis() - lastTick);
    displayModule->HandleAnimations(millis(), lastTick);
    lastTick = millis();
    lv_timer_handler();
#else
    if (!HasRanCommand)
    {
        double currentTime = millis();

        CommandLine::getInstance().main(currentTime);
    }
#endif
}