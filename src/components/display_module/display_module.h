#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#ifdef DISPLAY_SUPPORT
#include "views/splashscreen.h"
LV_IMG_DECLARE(ui_img_spooky_logo_png);
LV_FONT_DECLARE(ui_font_Font1);

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

    static void RenderMenuType(MenuType Type);
    static void SetTouchRotation(int Index);
    static void Destroy(ViewInterface* Interface, MenuType Nextmenu);
    void checkTouch(TS_Point p);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void FillScreen(lv_color_t color);
};

#endif
#endif