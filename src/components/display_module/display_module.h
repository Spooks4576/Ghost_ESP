#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#ifdef DISPLAY_SUPPORT
#include "views/splashscreen.h"
#include "views/mainmenu.h"
#include "views/interface/view_scrollbar_i.h"
LV_IMG_DECLARE(ui_img_spooky_logo_png);
LV_FONT_DECLARE(ui_font_Font1);
LV_IMG_DECLARE(bt_img);
LV_IMG_DECLARE(WiFi_img);
LV_IMG_DECLARE(led_img);

inline lv_display_t * disp;

inline XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

class DisplayModule {
public:
    SPIClass mySpi = SPIClass(VSPI);
    TFT_eSPI tft = TFT_eSPI();
    bool IsOnSplash;
    MenuType mtype;
    LinkedList<ViewInterface*> Views;
    int LastTouchX, LastTouchY;
    uint8_t* draw_buf;
    lv_obj_t * tCScreen;
    lv_indev_t* indev;

    static void RenderMenuType(MenuType Type);
    static void SetTouchRotation(int Index);
    static void Destroy(ViewInterface* Interface, MenuType Nextmenu);
    void checkTouch(TS_Point p);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void FillScreen(lv_color_t color);
    void HandleAnimations(unsigned long Millis, unsigned long LastTick);
};

#endif
#endif