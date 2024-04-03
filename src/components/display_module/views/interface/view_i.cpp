#include "view_i.h"

void ViewInterface::RenderTextBox(const char *text, lv_coord_t x, lv_coord_t y, int TextObjectIndex)
{

    if (TextObjectIndex < 0 || TextObjectIndex >= TextObjects.size()) {
        TextObjects.add(TextObjectIndex, lv_label_create(lv_scr_act()));
        lv_label_set_text(TextObjects[TextObjectIndex], text);
    }
    else 
    {
        lv_label_set_text(TextObjects[TextObjectIndex], text);
    }

    lv_obj_set_pos(TextObjects[TextObjectIndex], x, y);
}

void ViewInterface::RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex)
{

    if (ImageObjectIndex < 0 || ImageObjectIndex >= ImageObjects.size()) 
    {
        ImageObjects.add(ImageObjectIndex, lv_img_create(lv_scr_act()));
        lv_img_set_src(ImageObjects[ImageObjectIndex], img_src);
    }
    else
    {
        lv_img_set_src(ImageObjects[ImageObjectIndex], img_src);
    }
    lv_obj_set_pos(ImageObjects[ImageObjectIndex], x, y);
}

void ViewInterface::printTouchToSerial(TS_Point p)
{
    Serial.print("Pressure = ");
    Serial.print(p.z);
    Serial.print(", x = ");
    Serial.print(p.x);
    Serial.print(", y = ");
    Serial.print(p.y);
    Serial.println();
}   