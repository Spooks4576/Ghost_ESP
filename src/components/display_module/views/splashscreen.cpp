#include "splashscreen.h"

void SplashScreen::UpdateSplash(const char* Text, int Progress)
{
    RenderTextBox(Text, 80, 50, 0, 45);

    if (!TextObjects[0])
    {
        Serial.println("Failed to Create Text Box!!!");
        return;
    }

    if (!ImageObjects[0])
    {
        Serial.println("Failed to Create Image!!!");
        return;
    }

    if (Progress > 99)
    {
        delay(2000);
        Destroy(MenuType::MT_MainMenu);
    }
}   


void SplashScreen::HandleTouch(TS_Point P)
{
    if (Debugging)
    {
        printTouchToSerial(P);
    } // Do nothing but print if we are debugging
}

void SplashScreen::Render()
{
   RenderJpg(&logo, 280, 40, 0, 45);
}