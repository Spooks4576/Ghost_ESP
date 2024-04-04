#include "mainmenu.h"

void MainMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)
{
    
}

void MainMenu::Render()
{
    status_bar = create_status_bar(lv_scr_act());
    versionlabel = add_version_module(status_bar);
    batteryversion = add_battery_module(status_bar);
}

void MainMenu::HandleTouch(TS_Point P)
{

}

lv_obj_t * MainMenu::add_battery_module(lv_obj_t * status_bar) {
    lv_obj_t * battery_icon = lv_label_create(status_bar);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    return battery_icon;
}

lv_obj_t* MainMenu::add_version_module(lv_obj_t * status_bar)
{
    lv_obj_t * clock = lv_label_create(status_bar);
    lv_label_set_text(clock, "V1.5A");
    return clock;
}

lv_obj_t * MainMenu::create_status_bar(lv_obj_t * parent) {
    lv_obj_t * status_bar = lv_obj_create(parent);
    lv_obj_set_width(status_bar, LV_PCT(100));
    lv_obj_set_height(status_bar, LV_DPX(30));
    lv_obj_set_style_bg_color(status_bar, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);

    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);

    return status_bar;
}