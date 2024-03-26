#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))
#define SWIPE_THRESHOLD 30
#define TS_MINX 100
#define TS_MAXX 3800
#define TS_MINY 100
#define TS_MAXY 3800

#ifdef DISPLAY_SUPPORT
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>
#include <LinkedList.h>

inline lv_color_t buf[ DISPLAYWIDTH * DISPLAYHEIGHT / 10 ];

inline XPT2046_Touchscreen* touchscreen; // Global Due to Callback syntax
inline uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800;



struct SplashScreen
{
    const char* Text;
    int Progress;
    const char* lastSplashText = "";
    int lastSplashProgress = -1;
};

struct MenuItem {
    String label;
    uint16_t color;
    uint16_t textColor;
};

enum MenuType
{
    MT_MainMenu,
    MT_BluetoothMenu,
    MT_WifiUtilsMenu,
    MT_LEDUtils,
};

struct MainMenu
{
    LinkedList<MenuItem> menuItems;
    int currentItemIndex = 0;
    TS_Point lastTouchPoint; // Store the last touch point
    bool isTouching = false;
    bool needRedraw = true;  // Flag to indicate when the screen needs to be redrawn
    int prevItemIndex = -1;  // Previous menu item index for comparison

    // Swipe detection variables
    bool swipeStart = false;
    int startX = 0;
    int endX = 0;
};

struct Button {
    int x, y;   // Top left corner
    int w, h;   // Width and height
    String label; // Button text
    uint16_t color; // Button color
    uint16_t textColor; // Text color
    bool lastState = false; // Last touch state
};

class DisplayModule {
public:
    TFT_eSPI tft = TFT_eSPI();
    bool IsOnSplash;
    MainMenu mainmenu;
    MenuType mtype;
    SplashScreen Splash;
    SPIClass touchscreenSpi = SPIClass(VSPI);
    void drawProgressBar(int x, int y, int w, int h, int progress, uint16_t color);
    void UpdateSplashStatus(const char* Text, int Percent);
    void RenderSplashScreen();
    void RenderJpg(int x, int y);
    void drawButton(int x, int y, int w, int h, String text, bool isPressed);
    bool isButtonTouched(Button &btn, TS_Point p);
    void detectSwipeAndSwitchItems();
    void drawCurrentMenuItem();
};

#endif
#endif