#include "view_scrollbar_i.h"

void ScrollableMenu::Render() {
    status_bar = create_status_bar(lv_scr_act());
    List = lv_list_create(lv_scr_act());
    lv_obj_set_style_opa(List, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(List, lv_color_hex(0x000), 0);
    //lv_obj_set_style_border_color(List, lv_color_hex(0x000), 0);
    lv_obj_set_size(List, 270, 240);
    lv_obj_set_scrollbar_mode(List, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_flex_flow(List, LV_FLEX_FLOW_COLUMN_REVERSE, 0);
    lv_obj_clear_flag(List, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_fade_in(status_bar, 300, 0);
    lv_obj_fade_in(List, 300, 0);
    lv_obj_set_style_transform_pivot_x(List, lv_obj_get_width(List) / 2, 0);
    lv_obj_set_style_transform_pivot_y(List, lv_obj_get_height(List) / 2, 0);
    addItem("test 1");
    addItem("test 2");
    addItem("test 3");
    addItem("test 4");
}


void ScrollableMenu::HandleTouch(TS_Point P)  {

}


void ScrollableMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)  {
    
}

// Add item to menu
void ScrollableMenu::addItem(const char *text) {
    SubMenuItem newitem;
    newitem.cont = lv_obj_create(List);
    lv_obj_set_size(newitem.cont, 30, 150);
    newitem.label = lv_label_create(newitem.cont);
    lv_label_set_text(newitem.label, text);
    SubMenuItems.add(newitem);
}

// Item selection callback
void ScrollableMenu::selectItemCallback(lv_obj_t *obj, lv_event_t event) {
}