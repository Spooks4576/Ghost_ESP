#include "view_scrollbar_i.h"

void ScrollableMenu::Render() {
    status_bar = create_status_bar(lv_scr_act());
    List = lv_list_create(lv_scr_act());
    lv_obj_set_style_opa(List, LV_OPA_0, 0);
    lv_obj_set_size(List, 280, 100);
    lv_obj_center(List);
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