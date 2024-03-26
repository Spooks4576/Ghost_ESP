#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

#ifdef DISPLAY_SUPPORT
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>
#include <LinkedList.h>


inline lv_color_t buf[ DISPLAYWIDTH * DISPLAYHEIGHT / 10 ];
inline XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

struct SplashScreen
{
    const char* Text;
    int Progress;
    const char* lastSplashText = "";
    int lastSplashProgress = -1;
};

enum MenuType
{
    MT_MainMenu,
    MT_BluetoothMenu,
    MT_WifiUtilsMenu,
    MT_LEDUtils,
};

class DisplayModule {
public:
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite spr = TFT_eSprite(&tft);
    uint16_t backgroundColor, buttonColor, textColor, buttonTextColor;
    bool IsOnSplash;
    MenuType mtype;
    SplashScreen Splash;
    SPIClass touchscreenSpi = SPIClass(VSPI);


    struct Button {
        int x, y, w, h;
        uint16_t color, textColor, buttonPressedColor;
        String label;
        bool isPressed, isVisible;
        void (*callback)();

        bool contains(int tx, int ty) {
            return tx >= x && tx <= x + w && ty >= y && ty <= y + h;
        }

        void draw(TFT_eSprite &spr) {
            if (!isVisible) return;
            spr.fillRect(x, y, w, h, isPressed ? buttonPressedColor : color);
            spr.setTextColor(textColor);
            spr.drawCentreString(label, x + w / 2, y + h / 2 - 8, 2); // Center text vertically and horizontally
        }
    };

    Button MainMenuButtons[3] = {
        {60, 50, 200, 40, TFT_BLUE, TFT_WHITE, TFT_RED, "BLE Attacks", false, true, nullptr},
        {60, 110, 200, 40, TFT_BLUE, TFT_WHITE, TFT_RED, "WiFi Utils", false, true, nullptr},
        {60, 170, 200, 40, TFT_BLUE, TFT_WHITE, TFT_RED, "LED Utils", false, true, nullptr}
    };
    DisplayModule() : backgroundColor(TFT_BLACK), buttonColor(TFT_BLUE),
                      textColor(TFT_WHITE),
                      buttonTextColor(TFT_WHITE) {
        tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Initialize display
    }

    void drawMainMenu();
    void animateMenu();
    void setButtonCallback(int buttonIndex, void (*callback)());
    void checkTouch(int tx, int ty);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void RenderJpg(int x, int y);
    void printTouchToSerial(TS_Point p);
};

#endif
#endif