#include "system_manager.h"
#include "../core/commandline.h"
void SystemManager::setup()
{
    Serial.begin(115200);
    initDisplay();
    initLEDs();
    initSDCard();
    initBLE();
    initWiFi();
    CommandLine::getInstance().RunSetup();
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