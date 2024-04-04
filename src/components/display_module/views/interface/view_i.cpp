#include "view_i.h"

void ViewInterface::RenderTextBox(const char *text, lv_coord_t x, lv_coord_t y, int TextObjectIndex, int angle)
{
    if (TextObjectIndex < 0 || TextObjectIndex >= TextObjects.size()) {
        TextObjects.add(TextObjectIndex, lv_label_create(lv_scr_act()));
        lv_label_set_text(TextObjects[TextObjectIndex], text);
    }
    else {
        lv_label_set_text(TextObjects[TextObjectIndex], text);
    }

    // Use a bolder font
    lv_obj_set_style_text_font(TextObjects[TextObjectIndex], &strike, LV_PART_MAIN);

    static lv_style_t style_bright_text;
    lv_style_init(&style_bright_text);
    lv_style_set_text_color(&style_bright_text, lv_palette_main(LV_PALETTE_LIGHT_GREEN));

    lv_obj_add_style(TextObjects[TextObjectIndex], &style_bright_text, 0);

    lv_obj_set_pos(TextObjects[TextObjectIndex], x, y);

    
    lv_obj_set_style_transform_angle(TextObjects[TextObjectIndex], angle * 100, LV_PART_MAIN);

    lv_obj_set_scrollbar_mode(ImageObjects[TextObjectIndex], LV_SCROLLBAR_MODE_OFF);
}

void ViewInterface::RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex, int angle)
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

    lv_obj_set_style_transform_angle(ImageObjects[ImageObjectIndex], angle * 100, LV_PART_MAIN);

    lv_obj_set_scrollbar_mode(ImageObjects[ImageObjectIndex], LV_SCROLLBAR_MODE_OFF);
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