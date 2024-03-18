#pragma once

#include "board_config.h"


#ifdef DISPLAY_SUPPORT

struct SplashScreen
{

};


class DisplayModule {
public:
    void init();
    void UpdateSplashStatus(const char* Text, int Percent);
    void RenderSplashScreen();
};

#endif