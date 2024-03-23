#include "splash.h"
#include "../graphics.h"

SplashScreen::SplashScreen(unsigned long durationInMs) {
    this->endAt = millis()+durationInMs;
}

int SplashScreen::update(int button) {
    // update even when there's no keypress
    this->alwaysUpdates(true);
    unsigned int currentTime = millis();
    if (currentTime > this->endAt) {
        return SCREEN_BACK;
    }
    if (button==EVENT_INIT){
        return SCREEN_REDRAW;
    }
    return SCREEN_NONE;
}

bool SplashScreen::draw() {
  display->drawXbm(0, 0, 128, 64, splash_bits);
  display->drawString(94,0,"1.2.1");
  display->drawRect(92,0,36,12);
  return true;
}
