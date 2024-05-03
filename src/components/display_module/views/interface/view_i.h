#pragma once
#include "board_config.h"
#include <LinkedList.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>


LV_IMAGE_DECLARE(GhostESP);
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
    lv_obj_t * status_bar;
    LinkedList<lv_obj_t*> SBIcons;
    lv_obj_t * versionlabel;
    lv_obj_t * batteryversion;
    lv_obj_t * add_battery_module(lv_obj_t * status_bar);
    bool is_point_inside_button(TS_Point p, lv_obj_t* btn);
    LinkedList<lv_obj_t*> ImageObjects;
    LinkedList<lv_obj_t*> TextObjects;
    LinkedList<lv_obj_t*> OtherObjects;
    lv_obj_t * flashIcon;
    lv_obj_t * WifiChannelLabel;
    void RenderTextBox(const char *text, lv_coord_t x, lv_coord_t y, int TextObjectIndex, int angle);
    lv_obj_t * RenderImageToButton(lv_obj_t *parent, const lv_img_dsc_t *img_src, int angle, int x, int y, int width, int height);
    void RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex, int angle, bool ScaleUp = false);
    void printTouchToSerial(TS_Point P);
    virtual void HandleTouch(TS_Point P) = 0;
    lv_obj_t * create_status_bar(lv_obj_t * parent);
    lv_obj_t* add_version_module(lv_obj_t * status_bar);
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