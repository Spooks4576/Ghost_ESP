#include "splashscreen.h"

void SplashScreen::UpdateSplash(const char* Text, int Progress)
{
    RenderTextBox(Text, 0, 0, 0);

    if (!TextObjects[0])
    {
        Serial.println("Failed to Create Text Box!!!");
        return;
    }

    if (Progress > 99)
    {
        // TODO Handle Callback
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
    // Update Splash Status Handles this
}