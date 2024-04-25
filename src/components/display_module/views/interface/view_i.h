#pragma once
#include "board_config.h"
#include <LinkedList.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

LV_IMAGE_DECLARE(logo);
LV_IMAGE_DECLARE(GhostESP);
LV_FONT_DECLARE(strike);
LV_FONT_DECLARE(Juma);

enum MenuType
{
    MT_MainMenu,
    MT_BluetoothMenu,
    MT_WifiUtilsMenu,
    MT_LEDUtils,
};

class ViewInterface
{
public:
    ViewInterface(const char* InID)
    {
        ViewID = InID;
    }

    ViewInterface()
    {

    }

    const char* ViewID;
    bool Debugging;
    bool HasRendered;
    bool PlayedAnim = false;
    LinkedList<lv_obj_t*> ImageObjects;
    LinkedList<lv_obj_t*> TextObjects;
    LinkedList<lv_obj_t*> OtherObjects;
    void RenderTextBox(const char *text, lv_coord_t x, lv_coord_t y, int TextObjectIndex, int angle);
    lv_obj_t * RenderImageToButton(lv_obj_t *parent, const lv_img_dsc_t *img_src, int angle, int x, int y, int width, int height);
    void RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex, int angle, bool ScaleUp = false);
    void printTouchToSerial(TS_Point P);
    virtual void HandleTouch(TS_Point P) = 0;
    void (*UpdateRotationCallback)(int);
    void (*DestroyCallback)(ViewInterface* Interface, MenuType Nextmenu);
    virtual void Render() = 0;
    virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) = 0;
    unsigned long LastMillis;
    void Destroy(MenuType type)
    {
        DestroyCallback(this, type);
    };
    void UpdateRotation(int Rotation)
    {
        UpdateRotationCallback(Rotation);
    }
};