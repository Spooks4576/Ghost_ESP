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
   RenderJpg(&GhostESP, 240, 40, 0, 45);
   HasRendered = true;
}

void SplashScreen::HandleAnimations(unsigned long milliss, unsigned long LastTick)
{
    if (milliss >= 700 && !PlayedAnim) {

        PlayedAnim = true;
        isOnSplash = true;
        Serial.println(milliss);
        BootAnim = animate_image_scale(ImageObjects[0]);

        LastMillis = milliss;
    }

    if (milliss >= 5700 && PlayedAnim && isOnSplash) {
        isOnSplash = false;
        lv_obj_fade_out(ImageObjects[0], 200, 0);
        for( int i=0; i < 100; i++ ){
            lv_task_handler();
            lv_timer_handler();
            lv_tick_inc(millis() - LastTick);
            delay(5);
        }
        lv_obj_del(ImageObjects[0]);
        Destroy(MenuType::MT_MainMenu);
    }
}