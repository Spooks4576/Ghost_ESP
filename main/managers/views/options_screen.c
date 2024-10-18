#include "managers/views/options_screen.h"
#include "managers/views/main_menu_screen.h"
#include <stdio.h>

EOptionsMenuType SelectedMenuType = OT_Wifi;
int selected_item_index = 0;
lv_obj_t *menu_container = NULL;
int num_items = 0;

static void select_menu_item(int index); // Forward Declaration

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
    "Go Back",
    NULL
};

static const char *bluetooth_options[] = {
    "Scan BLE Devices",
    "Find Flippers",
    "Stop BLE Scan",
    "Start AirTag Scanner",
    "Go Back",
    NULL
};

static const char *gps_options[] = {
    "Start GPS Tracking",
    "Stop GPS Tracking",
    "Show GPS Info",
    "Go Back",
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
    "Go Back",
    NULL
};

void options_menu_create() {
    int screen_width = LV_HOR_RES;
    int screen_height = LV_VER_RES;


    display_manager_fill_screen(lv_color_black());


    lv_obj_t *root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_align(root, LV_ALIGN_BOTTOM_MID, -12, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    options_menu_view.root = root;

    
    lv_obj_t *list = lv_list_create(root);
    lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_pad_all(list, screen_width < 240 ? 2 : 4, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    menu_container = list;

    
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
        display_manager_switch_view(&main_menu_view);
        return;
    }

   
    num_items = 0;
    for (int i = 0; options[i] != NULL; i++) {

        lv_obj_t *btn = lv_list_add_btn(list, NULL, options[i]);

        
        if (screen_height < 200) {
            lv_obj_set_style_radius(btn, 2, 0);
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, 0);
        } else {
            lv_obj_set_style_radius(btn, 8, 0); 
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_16, 0);
        }

        
        lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);     
        lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN);

        lv_obj_set_user_data(btn, (void *)options[i]);

        
        lv_obj_add_event_cb(btn, option_event_cb, LV_EVENT_CLICKED, (void *)options[i]);

        num_items++;
    }

    select_menu_item(0);

    display_manager_add_status_bar(options_menu_type_to_string(SelectedMenuType));
}

static void select_menu_item(int index) {
    printf("select_menu_item called with index: %d, num_items: %d\n", index, num_items);

    if (index < 0) index = num_items - 1;
    if (index >= num_items) index = 0;

    printf("Adjusted index: %d\n", index);

    int previous_index = selected_item_index;

    selected_item_index = index;

    printf("Previous index: %d, New selected index: %d\n", previous_index, selected_item_index);

    lv_obj_t *current_item = lv_obj_get_child(menu_container, selected_item_index);
    if (current_item) {
        printf("Current item found for index %d\n", selected_item_index);

        lv_obj_set_style_bg_color(current_item, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(current_item, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(current_item, lv_color_make(255, 255, 0), LV_PART_MAIN);
        lv_obj_set_style_border_width(current_item, 4, LV_PART_MAIN);

        printf("Scrolling to view item at index %d\n", selected_item_index);
        lv_obj_scroll_to_view(current_item, LV_ANIM_OFF);
    } else {
        printf("Error: Current item not found for index %d\n", selected_item_index);
    }

    if (previous_index != selected_item_index) {
        lv_obj_t *previous_item = lv_obj_get_child(menu_container, previous_index);
        if (previous_item) {
            printf("Resetting style for previous item at index %d\n", previous_index);

            lv_obj_set_style_bg_color(previous_item, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_style_border_width(previous_item, 0, LV_PART_MAIN);
        } else {
            printf("Error: Previous item not found for index %d\n", previous_index);
        }
    }
}

void handle_hardware_button_press_options(int ButtonPressed) {
    if (ButtonPressed == 2) {
        select_menu_item(selected_item_index - 1);
    } else if (ButtonPressed == 4) {
        select_menu_item(selected_item_index + 1);
    } else if (ButtonPressed == 1) {
        const char *selected_option = (const char *)lv_label_get_text(lv_obj_get_child(lv_obj_get_child(menu_container, selected_item_index), 0));
        option_event_cb(selected_option);
    }
}

void option_event_cb(const char* Selected_Option) {

    if (strcmp(Selected_Option, "Go Back") == 0) {
        display_manager_switch_view(&main_menu_view);
    } else {
        printf("Option selected: %s\n", Selected_Option);
    }
}

void options_menu_destroy() {
    if (options_menu_view.root) {
        lv_obj_clean(options_menu_view.root);
        lv_obj_del(options_menu_view.root);
        options_menu_view.root = NULL;
    }
}

void get_options_menu_callback(void **callback) {
    *callback = options_menu_view.hardwareinput_callback;
}

View options_menu_view = {
    .root = NULL,
    .create = options_menu_create,
    .destroy = options_menu_destroy,
    .hardwareinput_callback = handle_hardware_button_press_options,
    .name = "Options Screen",
    .get_hardwareinput_callback = get_options_menu_callback
};