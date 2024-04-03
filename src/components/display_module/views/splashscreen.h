#pragma once
#include "interface/view_i.h"



class SplashScreen : public ViewInterface
{
public:

    SplashScreen(const char* ID = "splash") // Should Always = Splash
    {
        ViewID = ID;
    }

    SplashScreen()
    {

    }

    const char* Text;
    int Progress;
    const char* lastSplashText = "";
    void UpdateSplash(const char* text, int Progress);
    virtual void HandleTouch(TS_Point P) override;
    virtual void Render() override;
};