#include "display_module.h"
#ifdef DISPLAY_SUPPORT
#include "core/globals.h"

void tc_finish_cb(
        lv_event_t *event
    ) {
    displaymodule->Views[0]->Render();
}

void DriverReadCallback(lv_indev_t* indev_drv, lv_indev_data_t* data) 
{
    uint16_t x, y, z;
    z = displaymodule->tft.getTouchRawZ();

    if (z > 12) {
        bool touchReadSuccess = displaymodule->tft.getTouch(&x, &y);
        if (touchReadSuccess) {
            y = 240 - y;
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PR;
            displaymodule->checkTouch({x, y, z});
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void DisplayModule::RenderMenuType(MenuType Type)
{
    switch (Type)
    {
        case MenuType::MT_MainMenu:
        {
            ViewInterface* MM = new MainMenu("mainmenu");
            displaymodule->Views.add(MM);
            for (int i = 0; i < displaymodule->Views.size(); i++) 
            {
                displaymodule->Views[i]->UpdateRotationCallback = displaymodule->SetTouchRotation;
                displaymodule->Views[i]->DestroyCallback = displaymodule->Destroy;
            }
            MM->Render();
            break;
        }
        case MenuType::MT_WifiUtilsMenu:
        {
            break;
        }
        case MenuType::MT_BluetoothMenu:
        {
            break;
        }
        case MenuType::MT_LEDUtils:
        {
            break;
        }
    }
}

void DisplayModule::Destroy(ViewInterface* Interface, MenuType Nextmenu)
{

    for (int i = 0; i < displaymodule->Views.size(); i++)
    {
        if (displaymodule->Views[i]->ViewID == Interface->ViewID)
        {
            displaymodule->Views.remove(i);
            break;
        }
    }
    delete Interface;
    Serial.println("Destroy.....");
    displaymodule->FillScreen(lv_color_black());
    RenderMenuType(Nextmenu);
}

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
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(lv_scr_act(), &style, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void DisplayModule::HandleAnimations(unsigned long Millis, unsigned long LastTick)
{
    for (int i = 0; i < Views.size(); i++) 
    {
        if (Views[i] && Views[i]->HasRendered)
        {
            Views[i]->HandleAnimations(Millis, LastTick);
        }
    }
}

void DisplayModule::Init()
{
    lv_init();
    draw_buf = new uint8_t[DRAW_BUF_SIZE];
    disp = lv_tft_espi_create(TFT_VER_RES, TFT_HOR_RES, draw_buf, DRAW_BUF_SIZE);
    ts.begin();
    SetTouchRotation(1);
    ViewInterface* SplashI = new SplashScreen("splash");
    Views.add(SplashI);

    lv_display_set_resolution(disp, TFT_VER_RES, TFT_HOR_RES);

    // After Registering Views Register Callbacks

    for (int i = 0; i < Views.size(); i++) 
    {
        Views[i]->UpdateRotationCallback = this->SetTouchRotation;
        Views[i]->DestroyCallback = this->Destroy;
    }

    FillScreen(lv_color_black());

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, DriverReadCallback);

    SplashI->Render();
}

#endif