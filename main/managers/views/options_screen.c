#include "managers/views/options_screen.h"
#include <stdio.h>


View options_menu_view = {
    .root = NULL,
    .create = options_menu_create,
    .destroy = options_menu_destroy
};


EOptionsMenuType SelectedMenuType = OT_Wifi;

const char* options_menu_type_to_string(EOptionsMenuType menuType) {
    switch (menuType) {
        case OT_Wifi:
            return "Wi-Fi";
        case OT_Bluetooth:
            return "Bluetooth";
        case OT_GPS:
            return "GPS";
        case OT_Settings:
            return "Settings";
        default:
            return "Unknown";
    }
}

static const char *wifi_options[] = {
    "Scan Access Points",
    "Scan Stations",
    "Stop Scan",
    "List Access Points",
    "List Stations",
    "Start Deauth Attack",
    "Beacon Spam - Random",
    "Beacon Spam - Rickroll",
    "Stop Beacon Spam",
    "Stop Deauth",
    "Start Evil Portal",
    "Capture / Sniff Packets",
    "Connect",
    "TV Cast (Dial Connect)",
    "Power Printer",
    NULL
};

static const char *bluetooth_options[] = {
    "Scan BLE Devices",
    "Find Flippers",
    "Stop BLE Scan",
    "Start AirTag Scanner",
    NULL
};

static const char *gps_options[] = {
    "Start GPS Tracking",
    "Stop GPS Tracking",
    "Show GPS Info",
    NULL
};

static const char *settings_options[] = {
    "Set RGB Mode - Stealth",
    "Set RGB Mode - Normal",
    "Set RGB Mode - Rainbow",
    "Set Channel Switch Delay - 0.5s",
    "Enable Channel Hopping",
    "Disable Channel Hopping",
    "Enable Random BLE MAC",
    "Disable Random BLE MAC",
    NULL
};




void options_menu_create() {
    lv_obj_t * root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_pad_all(root, 10, 0);


    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);

   const char **options = NULL;

    switch (SelectedMenuType) {
        case OT_Wifi:
            options = wifi_options;
            break;
        case OT_Bluetooth:
            options = bluetooth_options;
            break;
        case OT_GPS:
            options = gps_options;
            break;
        case OT_Settings:
            options = settings_options;
            break;
        default:
            options = NULL;
            break;
    }

    if (options == NULL) {
        return;  // If no options are available, do nothing.
    }

    
    for (int i = 0; options[i] != NULL; i++) {
        lv_obj_t *btn = lv_btn_create(root);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_height(btn, lv_pct(10));
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);
        lv_obj_set_style_pad_all(btn, 15, 0);

        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, options[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, -10);

        
        lv_obj_add_event_cb(btn, option_event_cb, LV_EVENT_CLICKED, (void *)options[i]);
    }

    display_manager_add_status_bar(options_menu_type_to_string(SelectedMenuType));
}


void option_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *option = (const char *)lv_event_get_user_data(e);


    printf("Option selected: %s\n", option);


}


void options_menu_destroy() {
    if (options_menu_view.root) {
        lv_obj_clean(options_menu_view.root);
        lv_obj_del(options_menu_view.root);
        options_menu_view.root = NULL;
    }
}