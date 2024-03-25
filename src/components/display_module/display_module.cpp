#include "display_module.h"

#ifdef DISPLAY_SUPPORT

void DisplayModule::UpdateSplashStatus(const char* Text, int Percent)
{
    if (Splash)
    {
        Splash->message = Text;
        Splash->percentage = Percent;
    }
    else 
    {
        Serial.println("Splash is not valid");
    }
}

void DisplayModule::RenderSplashScreen()
{
    if (Splash == NULL)
    {
        Splash = new SplashScreen();
        Splash->show();
    }
    else 
    {
        Splash->show();
    }
}

#endif