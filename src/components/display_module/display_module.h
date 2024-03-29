#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

#ifdef DISPLAY_SUPPORT
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <LinkedList.h>
#include <core/images/bt.h>
#include <core/images/logo.h>


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
    const unsigned char* imageBuffer;
    const int imagebuffersize;
};

const int numCards = 3;
const int cardWidth = 50;
const int cardHeight = 50;
const int cardSpacing = 15;
const int xOffset = 80;      // Starting x-offset from left of the screen, adjust based on preference
const int yOffset = (240 - cardHeight) / 2;  // Center cards height-wise

class DisplayModule {
public:
    SPIClass mySpi = SPIClass(VSPI);
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite spr = TFT_eSprite(&tft);
    uint16_t backgroundColor, buttonColor, textColor, buttonTextColor;
    bool IsOnSplash;
    MenuType mtype;
    SplashScreen Splash;


    Card cards[numCards] = {
        {xOffset, yOffset, cardWidth, cardHeight, "BLE Attacks", TFT_WHITE, TFT_BLACK, false, bt_jpg, bt_jpg_size},
        {xOffset + cardWidth + cardSpacing, yOffset, cardWidth, cardHeight, "WiFi Utils", TFT_WHITE, TFT_BLACK, false, nullptr, 0},
        {xOffset + 2 * (cardWidth + cardSpacing), yOffset, cardWidth, cardHeight, "Led Utils", TFT_WHITE, TFT_BLACK, false, nullptr, 0}
    };

    DisplayModule() : backgroundColor(TFT_BLACK), buttonColor(TFT_BLUE),
                      textColor(TFT_WHITE),
                      buttonTextColor(TFT_WHITE) {
        tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Initialize display
    }
    int LastTouchX;
    int LastTouchY;
    void drawMainMenu();
    void drawSelectedLabel(const String &label);
    void animateMenu();
    void animateCardPop(const Card &card);
    void drawCard(const Card &card);
    void setButtonCallback(int buttonIndex, void (*callback)());
    void checkTouch(int tx, int ty);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void RenderJpg(int x, int y, int w = 0, int h = 0);
    void printTouchToSerial(TS_Point p);
};

#endif
#endif