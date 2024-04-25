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
    lv_obj_set_style_text_font(TextObjects[TextObjectIndex], &Juma, LV_PART_MAIN);

    static lv_style_t style_bright_text;
    lv_style_init(&style_bright_text);
    lv_style_set_text_color(&style_bright_text, lv_palette_main(LV_PALETTE_LIGHT_GREEN));

    lv_obj_add_style(TextObjects[TextObjectIndex], &style_bright_text, 0);

    lv_obj_set_pos(TextObjects[TextObjectIndex], x, y);

    
    lv_obj_set_style_transform_angle(TextObjects[TextObjectIndex], angle * 100, LV_PART_MAIN);

    lv_obj_set_scrollbar_mode(ImageObjects[TextObjectIndex], LV_SCROLLBAR_MODE_OFF);
}

lv_obj_t* ViewInterface::add_version_module(lv_obj_t * status_bar)
{
    lv_obj_t * version = lv_label_create(status_bar);
    lv_label_set_text(version, "V-1.5A");
    lv_obj_set_style_transform_angle(version, 900, 0);
    lv_obj_set_style_transform_pivot_x(version, lv_obj_get_width(version) / 2, 0);
    lv_obj_set_style_transform_pivot_y(version, lv_obj_get_height(version) / 2, 0);   
    lv_obj_set_pos(version, 8, 0);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(version, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    return version;
}

lv_obj_t * ViewInterface::add_battery_module(lv_obj_t * status_bar) {
    lv_obj_t * battery_icon = lv_label_create(status_bar);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_transform_angle(battery_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(battery_icon, lv_obj_get_width(battery_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(battery_icon, lv_obj_get_height(battery_icon) / 2, 0);   
    lv_obj_set_pos(battery_icon, 8, 200);
    lv_obj_set_style_text_color(battery_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    flashIcon = lv_label_create(status_bar);
    lv_label_set_text(flashIcon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_transform_angle(flashIcon, 900, 0);
    lv_obj_set_style_transform_pivot_x(flashIcon, lv_obj_get_width(flashIcon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(flashIcon, lv_obj_get_height(flashIcon) / 2, 0);   
    lv_obj_set_pos(flashIcon, 8, 185);
    lv_obj_set_style_text_color(flashIcon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(flashIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(battery_icon, LV_SCROLLBAR_MODE_OFF);
    return battery_icon;
}

lv_obj_t * ViewInterface::create_status_bar(lv_obj_t * parent) {
    lv_obj_t * status_bar = lv_obj_create(parent);
    lv_obj_set_height(status_bar, LV_PCT(100));
    lv_obj_set_width(status_bar, LV_DPX(35));
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_0, 0);
    lv_obj_set_layout(status_bar, LV_LAYOUT_NONE);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_align(status_bar, LV_ALIGN_RIGHT_MID, 0, 0);

    versionlabel = add_version_module(status_bar);
    batteryversion = add_battery_module(status_bar);

    int xOffset = 185;

#ifdef HAS_BT
    lv_obj_t* bt_icon = lv_label_create(status_bar);
    SBIcons.add(bt_icon);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_transform_angle(bt_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(bt_icon, lv_obj_get_width(bt_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(bt_icon, lv_obj_get_height(bt_icon) / 2, 0);   
    lv_obj_set_pos(bt_icon, 8, xOffset -= 16);
    lv_obj_set_style_text_color(bt_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(bt_icon, LV_SCROLLBAR_MODE_OFF);
#endif

#ifdef SD_CARD_CS_PIN
    lv_obj_t* sd_icon = lv_label_create(status_bar);
    SBIcons.add(sd_icon);
    lv_label_set_text(sd_icon, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_transform_angle(sd_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(sd_icon, lv_obj_get_width(sd_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(sd_icon, lv_obj_get_height(sd_icon) / 2, 0);   
    lv_obj_set_pos(sd_icon, 8, xOffset -= 20);
    lv_obj_set_style_text_color(sd_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(sd_icon, LV_SCROLLBAR_MODE_OFF);
#endif

    lv_obj_t* wifi_icon = lv_label_create(status_bar);
    SBIcons.add(wifi_icon);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_transform_angle(wifi_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(wifi_icon, lv_obj_get_width(wifi_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(wifi_icon, lv_obj_get_height(wifi_icon) / 2, 0);   
    lv_obj_set_pos(wifi_icon, 8, xOffset -= 23);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(wifi_icon, LV_SCROLLBAR_MODE_OFF);
    return status_bar;
}

lv_obj_t * ViewInterface::RenderImageToButton(lv_obj_t *parent, const lv_img_dsc_t *img_src, int angle, int x, int y, int width, int height) {
    lv_obj_t *btn = lv_imagebutton_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, width, height);

    // Set image for the released and pressed state
    lv_imagebutton_set_src(btn, LV_IMAGEBUTTON_STATE_RELEASED, NULL, img_src, NULL);
    lv_imagebutton_set_src(btn, LV_IMAGEBUTTON_STATE_PRESSED, NULL, img_src, NULL);

    // Handle angle
    if (angle != 0) {
        lv_obj_set_style_transform_angle(btn, angle * 10, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_x(btn, width / 2, LV_PART_MAIN); 
        lv_obj_set_style_transform_pivot_y(btn, height / 2, LV_PART_MAIN);
    }

    return btn;
}

void ViewInterface::RenderJpg(const lv_img_dsc_t *img_src, lv_coord_t x, lv_coord_t y, int ImageObjectIndex, int angle, bool ScaleUp)
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

    lv_obj_set_style_transform_pivot_x(ImageObjects[ImageObjectIndex], lv_obj_get_width(ImageObjects[ImageObjectIndex]) / 2, 0);
    lv_obj_set_style_transform_pivot_y(ImageObjects[ImageObjectIndex], lv_obj_get_height(ImageObjects[ImageObjectIndex]) / 2, 0);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 10);
    lv_obj_add_style(ImageObjects[ImageObjectIndex], &style_btn, 0);

    lv_obj_set_style_transform_angle(ImageObjects[ImageObjectIndex], angle * 100, LV_PART_MAIN);

    lv_obj_set_scrollbar_mode(ImageObjects[ImageObjectIndex], LV_SCROLLBAR_MODE_OFF);
    
    if (ScaleUp)
    {
        lv_img_set_zoom(ImageObjects[ImageObjectIndex], 504);
    }
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