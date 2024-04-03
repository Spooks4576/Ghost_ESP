#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#ifdef DISPLAY_SUPPORT
#include "views/splashscreen.h"

enum MenuType
{
    MT_MainMenu,
    MT_BluetoothMenu,
    MT_WifiUtilsMenu,
    MT_LEDUtils,
};

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

    static void SetTouchRotation(int Index);
    void checkTouch(TS_Point p);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void FillScreen(lv_color_t color);
};

#endif
#endif