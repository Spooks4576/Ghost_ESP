#include "splashscreen.h"
#include "../UI/Animations/Scaleinout.h"

void SplashScreen::UpdateSplash(const char* Text, int Progress)
{

    if (Progress > 666)
    {
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
   RenderJpg(&logo, 240, 40, 0, 45);
}

void SplashScreen::HandleAnimations(unsigned long millis, unsigned long LastTick)
{
    if (millis >= 700 && !PlayedAnim) {

        PlayedAnim = true;
        isOnSplash = true;
        Serial.println(millis);
        BootAnim = animate_image_scale(ImageObjects[0]);

        LastMillis = millis;
    }

    if (millis >= 5700 && PlayedAnim && isOnSplash) {
        isOnSplash = false;
        lv_anim_del_all();
        lv_obj_del(ImageObjects[0]);
        Destroy(MenuType::MT_MainMenu);
    }
}