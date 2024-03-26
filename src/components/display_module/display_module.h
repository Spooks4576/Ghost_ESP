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

struct Card {
    int x, y, w, h;
    String title;
    uint16_t bgColor;
    uint16_t fgColor;
    bool isSelected;
    const uint16_t* imageBuffer;
};

const int numCards = 3;
const int cardWidth = 70;
const int cardHeight = 70;
const int cardSpacing = 15;
const int xOffset = (240 - cardWidth) / 2;
const int yOffset = 20;

class DisplayModule {
public:
    SPIClass mySpi = SPIClass(VSPI);
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite spr = TFT_eSprite(&tft);
    uint16_t backgroundColor, buttonColor, textColor, buttonTextColor;
    bool IsOnSplash;
    MenuType mtype;
    SplashScreen Splash;
    SPIClass touchscreenSpi = SPIClass(VSPI);


    Card cards[numCards] = {
        {xOffset, yOffset, cardWidth, cardHeight, "BLE Attacks", TFT_WHITE, TFT_BLACK, false, nullptr},
        {xOffset, yOffset + cardHeight + cardSpacing, cardWidth, cardHeight, "WiFi Utils", TFT_WHITE, TFT_BLACK, false, nullptr},
        {xOffset, yOffset + 2 * (cardHeight + cardSpacing), cardWidth, cardHeight, "Led Utils", TFT_WHITE, TFT_BLACK, false, nullptr}
    };

    DisplayModule() : backgroundColor(TFT_BLACK), buttonColor(TFT_BLUE),
                      textColor(TFT_WHITE),
                      buttonTextColor(TFT_WHITE) {
        tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Initialize display
    }

    void drawMainMenu();
    void drawSelectedLabel(const String &label);
    void animateMenu();
    void animateCardPop(const Card &card);
    void drawCard(const Card &card);
    void setButtonCallback(int buttonIndex, void (*callback)());
    void checkTouch(int tx, int ty);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void RenderJpg(int x, int y);
    void printTouchToSerial(TS_Point p);
};

#endif
#endif