#ifndef OPTIONS_MENU_VIEW_H
#define OPTIONS_MENU_VIEW_H

#include "lvgl/lvgl.h"
#include "managers/display_manager.h"

typedef enum
{
    OT_Wifi,
    OT_Bluetooth,
    OT_GPS,
    OT_Settings
} EOptionsMenuType;

extern View options_menu_view;
extern EOptionsMenuType SelectedMenuType;

void options_menu_create(void);
void options_menu_destroy(void);
void option_event_cb(lv_event_t * e);
void handle_option_directly(const char* Selected_Option);

#endif // OPTIONS_MENU_VIEW_H