#include "managers/views/options_screen.h"
#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/error_popup.h"
#include "managers/wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/serial_manager.h"
#include "esp_wifi_types.h"
#include "esp_timer.h"
#include <stdio.h>

EOptionsMenuType SelectedMenuType = OT_Wifi;
int selected_item_index = 0;
lv_obj_t *root = NULL;
lv_obj_t *menu_container = NULL;
int num_items = 0;
unsigned long createdTimeInMs = 0;

static void select_menu_item(int index); // Forward Declaration

const char* options_menu_type_to_string(EOptionsMenuType menuType) {
    switch (menuType) {
        case OT_Wifi:
            return "Wi-Fi";
        case OT_Bluetooth:
            return "BLE";
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
    "Scan LAN Devices",
    "Start Deauth Attack",
    "Beacon Spam - Random",
    "Beacon Spam - Rickroll",
    "Beacon Spam - List",
    "Start Evil Portal",
    "Capture Probe",
    "Capture Deauth",
    "Capture Beacon",
    "Capture Raw",
    "Capture Eapol",
    "Capture WPS",
    "Capture PWN",
    "TV Cast (Dial Connect)",
    "Power Printer",
    "TP Link Test",
    "Go Back",
    "    ",
    NULL
};

static const char *bluetooth_options[] = {
    "Find Flippers",
    "Start AirTag Scanner",
    "Raw BLE Scanner",
    "BLE Skimmer Detect",
    "Go Back",
    NULL
};

static const char *gps_options[] = {
    "Start Wardriving",
    "Stop Wardriving",
    "GPS Info",
    "BLE Wardriving",
    "Go Back",
    NULL
};

static const char *settings_options[] = {
    "Set RGB Mode - Stealth",
    "Set RGB Mode - Normal",
    "Set RGB Mode - Rainbow",
    "Go Back",
    NULL
};

void options_menu_create() {
    int screen_width = LV_HOR_RES;
    int screen_height = LV_VER_RES;

    display_manager_fill_screen(lv_color_black());


    root = lv_obj_create(lv_scr_act());
    options_menu_view.root = root;
    lv_obj_set_size(root, screen_width, screen_height);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_align(root, LV_ALIGN_BOTTOM_MID, -12, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

#ifdef CONFIG_JC3248W535EN_LCD
    screen_width -= 50;
#endif
    
    lv_obj_t *list = lv_list_create(root);
    lv_obj_set_size(list, screen_width, screen_height);
    lv_obj_set_style_pad_all(list, screen_width < 240 ? 2 : 4, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(list, lv_color_black(), 0);

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

    createdTimeInMs = (unsigned long) (esp_timer_get_time() / 1000ULL);
}

static void select_menu_item(int index) {
    printf("select_menu_item called with index: %d, num_items: %d\n", index, num_items);

    if (index < 0) index = num_items - 1;
    if (index >= num_items) index = 0;

    printf("Adjusted index: %d\n", index);

    int previous_index = selected_item_index;

    selected_item_index = index;

    printf("Previous index: %d, New selected index: %d\n", previous_index, selected_item_index);

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


    lv_obj_t *current_item = lv_obj_get_child(menu_container, selected_item_index);
    if (current_item) {
        printf("Current item found for index %d\n", selected_item_index);

        lv_obj_set_style_bg_color(current_item, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(current_item, LV_OPA_COVER, LV_PART_MAIN);

        printf("Scrolling to view item at index %d\n", selected_item_index);
        lv_obj_scroll_to_view(current_item, LV_ANIM_OFF);
    } else {
        printf("Error: Current item not found for index %d\n", selected_item_index);
    }
}

void handle_hardware_button_press_options(InputEvent *event) {
#ifdef CONFIG_JC3248W535EN_LCD
    // lvgl on the JC3248W535EN doesn't require custom input handling
    return;
#endif

    if (event->type == INPUT_TYPE_TOUCH) {
        lv_indev_data_t *data = &event->data.touch_data;

        int screen_height = CONFIG_TFT_HEIGHT;
        int third_height = screen_height / 3;

        if (data->point.y < third_height) {
            select_menu_item(selected_item_index - 1);
        } else if (data->point.y > 2 * third_height) {
            select_menu_item(selected_item_index + 1);
        } else {
            const char *selected_option = (const char *)lv_label_get_text(
                lv_obj_get_child(lv_obj_get_child(menu_container, selected_item_index), 0)
            );
            option_event_cb(selected_option);
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;

        if (button == 2) {
            select_menu_item(selected_item_index - 1);
        } else if (button == 4) {
            select_menu_item(selected_item_index + 1);
        } else if (button == 1) {
            const char *selected_option = (const char *)lv_label_get_text(
                lv_obj_get_child(lv_obj_get_child(menu_container, selected_item_index), 0)
            );
            option_event_cb(selected_option);
        }
    }
}

void option_event_cb(lv_event_t * e) {

    // when moving to the options screen ignore any click events for 1s to make
    // sure user input destined for the previosu is not accidentally used here
    if ((esp_timer_get_time() / 1000ULL) - createdTimeInMs <= 500)
    {
        return;
    }

    char* Selected_Option = (char*) lv_event_get_user_data(e);

    if (strcmp(Selected_Option, "Scan Access Points") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("scanap");
    }

    if (strcmp(Selected_Option, "Start Deauth Attack") == 0) {
        if (scanned_aps)
        {
            display_manager_switch_view(&terminal_view);
            vTaskDelay(pdMS_TO_TICKS(10));
            simulateCommand("attack -d");
        }
        else 
        {
            error_popup_create("You Need to Scan AP's First...");
        }
    }

    if (strcmp(Selected_Option, "Scan Stations") == 0) {
        if (strlen((const char*)selected_ap.ssid) > 0)
        {
            display_manager_switch_view(&terminal_view);
            vTaskDelay(pdMS_TO_TICKS(10));
            simulateCommand("scansta");
        }
        else 
        {
            error_popup_create("You Need to Select a Scanned AP First...");
        }
    }
    

    if (strcmp(Selected_Option, "Beacon Spam - Random") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("beaconspam -r");
    }


    if (strcmp(Selected_Option, "Beacon Spam - Rickroll") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("beaconspam -rr");
    }

    if (strcmp(Selected_Option, "Scan LAN Devices") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("scanlocal");
    }


    if (strcmp(Selected_Option, "Beacon Spam - List") == 0) {
        if (scanned_aps)
        {
            display_manager_switch_view(&terminal_view);
            vTaskDelay(pdMS_TO_TICKS(10));
            simulateCommand("beaconspam -l");
        }
        else 
        {
            error_popup_create("You Need to Scan AP's First...");
        }
    }


    if (strcmp(Selected_Option, "Capture Deauth") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -deauth");
    }

    if (strcmp(Selected_Option, "Capture Probe") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -probe");
    }

    if (strcmp(Selected_Option, "Capture Beacon") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -beacon");
    }

    if (strcmp(Selected_Option, "Capture Raw") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -raw");
    }

    if (strcmp(Selected_Option, "Capture Eapol") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -eapol");
    }

    if (strcmp(Selected_Option, "Capture WPS") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -wps");
    }

    if (strcmp(Selected_Option, "TV Cast (Dial Connect)") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("dialconnect");
    }

    if (strcmp(Selected_Option, "Power Printer") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("powerprinter");
    }

    if (strcmp(Selected_Option, "Start Evil Portal") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("startportal");
    }

    if (strcmp(Selected_Option, "Start Wardriving") == 0)
    {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("startwd");
    }

    if (strcmp(Selected_Option, "Stop Wardriving") == 0)
    {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("startwd -s");
    }

if (strcmp(Selected_Option, "Start AirTag Scanner") == 0) {
#ifndef CONFIG_IDF_TARGET_ESP32S2
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("blescan -a");
#else 
    error_popup_create("Device Does not Support Bluetooth...");
#endif
    }
    

if (strcmp(Selected_Option, "Find Flippers") == 0) {
#ifndef CONFIG_IDF_TARGET_ESP32S2
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("blescan -f");
#else 
    error_popup_create("Device Does not Support Bluetooth...");
#endif
    }


    if (strcmp(Selected_Option, "Set RGB Mode - Stealth") == 0) {
        simulateCommand("setsetting 1 1");
        vTaskDelay(pdMS_TO_TICKS(10));
        error_popup_create("Set RGB Mode Successfully...");
    }

    if (strcmp(Selected_Option, "Set RGB Mode - Normal") == 0) {
        simulateCommand("setsetting 1 2");
        vTaskDelay(pdMS_TO_TICKS(10));
        error_popup_create("Set RGB Mode Successfully...");
    }

    if (strcmp(Selected_Option, "Set RGB Mode - Rainbow") == 0) {
        simulateCommand("setsetting 1 3");
        vTaskDelay(pdMS_TO_TICKS(10));
        error_popup_create("Set RGB Mode Successfully...");
    }



    if (strcmp(Selected_Option, "Go Back") == 0) {
        display_manager_switch_view(&main_menu_view);
    } else {
        printf("Option selected: %s\n", Selected_Option);
    }

    if (strcmp(Selected_Option, "Capture PWN") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -pwn");
    }

    if (strcmp(Selected_Option, "TP Link Test") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("tplinktest");
    }

    if (strcmp(Selected_Option, "Raw BLE Scanner") == 0) {
#ifndef CONFIG_IDF_TARGET_ESP32S2
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("blescan -r");
#else 
    error_popup_create("Device Does not Support Bluetooth...");
#endif
    }

    if (strcmp(Selected_Option, "BLE Skimmer Detect") == 0) {
#ifndef CONFIG_IDF_TARGET_ESP32S2
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("capture -skimmer");
#else 
    error_popup_create("Device Does not Support Bluetooth...");
#endif
    }

    if (strcmp(Selected_Option, "GPS Info") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("gpsinfo");
    }

    if (strcmp(Selected_Option, "BLE Wardriving") == 0) {
#ifndef CONFIG_IDF_TARGET_ESP32S2
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("blewardriving");
#else 
    error_popup_create("Device Does not Support Bluetooth...");
#endif
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
    *callback = options_menu_view.input_callback;
}

View options_menu_view = {
    .root = NULL,
    .create = options_menu_create,
    .destroy = options_menu_destroy,
    .input_callback = handle_hardware_button_press_options,
    .name = "Options Screen",
    .get_hardwareinput_callback = get_options_menu_callback
};