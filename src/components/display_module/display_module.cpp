#include "display_module.h"
#ifdef DISPLAY_SUPPORT

void DisplayModule::SetTouchRotation(int Index)
{
    ts.setRotation(Index);

    lv_display_rotation_t TargetRot;

    switch (Index)
    {
        case 0:
        {
            TargetRot = lv_display_rotation_t::LV_DISPLAY_ROTATION_0;
            break;
        }

        case 1:
        {
            TargetRot = lv_display_rotation_t::LV_DISPLAY_ROTATION_90;
            break;
        }
        case 2:
        {
            TargetRot = lv_display_rotation_t::LV_DISPLAY_ROTATION_180;
            break;
        }

        case 3:
        {
            TargetRot = lv_display_rotation_t::LV_DISPLAY_ROTATION_270;
            break;
        } 
    } // Might Need Adjusting

    lv_display_set_rotation(disp, TargetRot);
}

void DisplayModule::UpdateSplashStatus(const char* Text, int Percent)
{
    for (int i = 0; i < Views.size(); i++) 
    {
        if (Views[i]->ViewID == "splash")
        {   
            SplashScreen* SplashI = (SplashScreen*)Views[i];
            if (SplashI)
            {
                SplashI->UpdateSplash(Text, Percent);
            }
        }
    }
}

void DisplayModule::checkTouch(TS_Point p) {
    if (LastTouchX != p.x && LastTouchY != p.y)
    {
        for (int i = 0; i < Views.size(); i++) 
        {
            Views[i]->HandleTouch(p);
        }
        LastTouchX = p.x;
        LastTouchY = p.y;
    }
}

void DisplayModule::FillScreen(lv_color_t color)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, color); 
    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    lv_obj_add_style(lv_scr_act(), &style, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void DisplayModule::Init()
{
    lv_init();
    draw_buf = new uint8_t[DRAW_BUF_SIZE];
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
    ts.begin();
    SetTouchRotation(1);
    ViewInterface* SplashI = new SplashScreen("splash");
    Views.add(SplashI);

    // After Registering all the Views Register Callbacks

    for (int i = 0; i < Views.size(); i++) 
    {
        Views[i]->UpdateRotationCallback = this->SetTouchRotation;
    }

    FillScreen(lv_color_black());
}

#endif