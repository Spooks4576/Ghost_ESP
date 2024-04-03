#pragma once
#include "board_config.h"
#include <LinkedList.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

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
    LinkedList<lv_obj_t*> ImageObjects;
    LinkedList<lv_obj_t*> TextObjects;
    void RenderTextBox(const char *text, lv_coord_t x, lv_coord_t y, int TextObjectIndex);
    void RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex);
    void printTouchToSerial(TS_Point P);
    virtual void HandleTouch(TS_Point P) = 0;
    void (*UpdateRotationCallback)(int);
    virtual void Render() = 0;
    void UpdateRotation(int Rotation)
    {
        UpdateRotationCallback(Rotation);
    }
};