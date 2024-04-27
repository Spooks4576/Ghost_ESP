#include "view_scrollbar_i.h"

void ScrollableMenu::Render() {
    status_bar = create_status_bar(lv_scr_act());
    List = lv_list_create(lv_scr_act());
    lv_obj_set_style_opa(List, LV_OPA_0, 0);
    lv_obj_set_size(List, 250, 200);
    lv_obj_center(List);
    lv_obj_set_scrollbar_mode(List, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(List, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_fade_in(status_bar, 300, 0);
    lv_obj_fade_in(List, 300, 0);
}


void ScrollableMenu::HandleTouch(TS_Point P)  {

}


void ScrollableMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)  {
    
}

// Add item to menu
void ScrollableMenu::addItem(const char *text) {

}

// Item selection callback
void ScrollableMenu::selectItemCallback(lv_obj_t *obj, lv_event_t event) {
}