#include "managers/views/options_screen.h"
#include "core/serial_manager.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "managers/views/error_popup.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/terminal_screen.h"
#include "managers/views/number_pad_screen.h"
#include "managers/wifi_manager.h"
#include <stdio.h>

EOptionsMenuType SelectedMenuType = OT_Wifi;
int selected_item_index = 0;
lv_obj_t *root = NULL;
lv_obj_t *menu_container = NULL;
int num_items = 0;
unsigned long createdTimeInMs = 0;

static void select_menu_item(int index); // Forward Declaration

const char *options_menu_type_to_string(EOptionsMenuType menuType) {
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

static const char *wifi_options[] = {"Scan Access Points",
                                     "Select AP",
                                     "Scan LAN Devices",
                                     "Select LAN",
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
                                     "PineAP Detection",
                                     "Scan Open Ports",
                                     "Reset AP Credentials",
                                     "Go Back",
                                     NULL};

static const char *bluetooth_options[] = {"Find Flippers",   "Start AirTag Scanner",
                                          "Raw BLE Scanner", "BLE Skimmer Detect",
                                          "Go Back",         NULL};

static const char *gps_options[] = {"Start Wardriving", "Stop Wardriving", "GPS Info",
                                    "BLE Wardriving",   "Go Back",         NULL};

static const char *settings_options[] = {"Set RGB Mode - Stealth", "Set RGB Mode - Normal",
                                         "Set RGB Mode - Rainbow", "Go Back", NULL};


static void up_down_event_cb(lv_event_t *e) {
int direction = (int)(intptr_t)lv_event_get_user_data(e);
select_menu_item(selected_item_index + direction);
}


void options_menu_create() {
    int screen_width = LV_HOR_RES;
    int screen_height = LV_VER_RES;

    display_manager_fill_screen(lv_color_hex(0x121212));

    root = lv_obj_create(lv_scr_act());
    options_menu_view.root = root;
    lv_obj_set_size(root, screen_width, screen_height);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x121212), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_align(root, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);

    menu_container = lv_obj_create(root);
    lv_obj_set_size(menu_container, screen_width, screen_height - 20);
    lv_obj_set_layout(menu_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(menu_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(menu_container, 10, 0);
    lv_obj_set_style_pad_row(menu_container, 5, 0);
    lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);

    const char **options = NULL;
    switch (SelectedMenuType) {
    case OT_Wifi: options = wifi_options; break;
    case OT_Bluetooth: options = bluetooth_options; break;
    case OT_GPS: options = gps_options; break;
    case OT_Settings: options = settings_options; break;
    default: options = NULL; break;
    }

    if (options == NULL) {
        display_manager_switch_view(&main_menu_view);
        return;
    }

    num_items = 0;
    int button_height = 60;
    for (int i = 0; options[i] != NULL; i++) {
        lv_obj_t *btn = lv_btn_create(menu_container);
        lv_obj_set_size(btn, screen_width - 20, button_height);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(btn, 10, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, options[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

        if (options[i + 1] != NULL) {
            lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), LV_PART_MAIN);
        }

        lv_obj_add_event_cb(btn, option_event_cb, LV_EVENT_CLICKED, (void *)options[i]);
        lv_obj_set_user_data(btn, (void *)options[i]);

        num_items++;
    }

    if (num_items * (button_height + 5) > screen_height - 20) {
        lv_obj_t *up_btn = lv_btn_create(root);
        lv_obj_set_size(up_btn, 40, 40);
        lv_obj_align(up_btn, LV_ALIGN_BOTTOM_RIGHT, -50, -10);
        lv_obj_set_style_bg_color(up_btn, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(up_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_t *up_label = lv_label_create(up_btn);
        lv_label_set_text(up_label, LV_SYMBOL_UP);
        lv_obj_center(up_label);
        lv_obj_add_event_cb(up_btn, up_down_event_cb, LV_EVENT_CLICKED, (void *)-1);

        lv_obj_t *down_btn = lv_btn_create(root);
        lv_obj_set_size(down_btn, 40, 40);
        lv_obj_align(down_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_set_style_bg_color(down_btn, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(down_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_t *down_label = lv_label_create(down_btn);
        lv_label_set_text(down_label, LV_SYMBOL_DOWN);
        lv_obj_center(down_label);
        lv_obj_add_event_cb(down_btn, up_down_event_cb, LV_EVENT_CLICKED, (void *)1);
    }

    select_menu_item(0);
    display_manager_add_status_bar(options_menu_type_to_string(SelectedMenuType));

    createdTimeInMs = (unsigned long)(esp_timer_get_time() / 1000ULL);
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
            lv_obj_set_style_bg_color(previous_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_color(previous_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_dir(previous_item, LV_GRAD_DIR_NONE, LV_PART_MAIN);
        }
    }

    lv_obj_t *current_item = lv_obj_get_child(menu_container, selected_item_index);
    if (current_item) {
        lv_color_t orange_start = lv_color_hex(0xFF5722); // Deep orange
        lv_color_t orange_end = lv_color_hex(0xD81B60);   // Darker orange-pink
        lv_obj_set_style_bg_color(current_item, orange_start, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(current_item, orange_end, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(current_item, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(current_item, LV_OPA_COVER, LV_PART_MAIN);

        // Scroll to view if needed
        lv_obj_scroll_to_view(current_item, LV_ANIM_OFF);
    } else {
        printf("Error: Current item not found for index %d\n", selected_item_index);
    }
}

void handle_hardware_button_press_options(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        lv_indev_data_t *data = &event->data.touch_data;
        printf("Touch at x=%d, y=%d\n", data->point.x, data->point.y);

        // Check if touch is on up/down buttons (bottom 50px)
        if (data->point.y > LV_VER_RES - 50) {
            if (data->point.x > LV_HOR_RES - 100 && data->point.x < LV_HOR_RES - 50) {
                select_menu_item(selected_item_index - 1); // Up
            } else if (data->point.x > LV_HOR_RES - 50) {
                select_menu_item(selected_item_index + 1); // Down
            }
            return;
        }

        // Check touch on buttons
        for (int i = 0; i < num_items; i++) {
            lv_obj_t *btn = lv_obj_get_child(menu_container, i);
            lv_area_t btn_area;
            lv_obj_get_coords(btn, &btn_area);
            if (data->point.x >= btn_area.x1 && data->point.x <= btn_area.x2 &&
                data->point.y >= btn_area.y1 && data->point.y <= btn_area.y2) {
                select_menu_item(i);
                handle_option_directly((const char *)lv_obj_get_user_data(btn));
                break;
            }
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 2) {
            select_menu_item(selected_item_index - 1); // Up
        } else if (button == 4) {
            select_menu_item(selected_item_index + 1); // Down
        } else if (button == 1) {
            lv_obj_t *selected_obj = lv_obj_get_child(menu_container, selected_item_index);
            if (selected_obj) {
                const char *selected_option = (const char *)lv_obj_get_user_data(selected_obj);
                if (selected_option) {
                    handle_option_directly(selected_option);
                }
            }
        }
    }
}

void option_event_cb(lv_event_t *e) {
    // when moving to the options screen ignore any click events for 1s
    if ((esp_timer_get_time() / 1000ULL) - createdTimeInMs <= 500) {
        return;
    }

    const char *Selected_Option = (const char *)lv_event_get_user_data(e);

    if (strcmp(Selected_Option, "Scan Access Points") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("scanap");
    }

    if (strcmp(Selected_Option, "Start Deauth Attack") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        if (!scanned_aps) {
            TERMINAL_VIEW_ADD_TEXT("No APs scanned. Please run 'Scan Access Points' first.\n");
        } else {
            simulateCommand("attack -d");
        }
    }

    if (strcmp(Selected_Option, "Scan Stations") == 0) {
        if (strlen((const char *)selected_ap.ssid) > 0) {
            display_manager_switch_view(&terminal_view);
            vTaskDelay(pdMS_TO_TICKS(10));
            simulateCommand("scansta");
        } else {
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
        if (scanned_aps) {
            display_manager_switch_view(&terminal_view);
            vTaskDelay(pdMS_TO_TICKS(10));
            simulateCommand("beaconspam -l");
        } else {
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

    if (strcmp(Selected_Option, "Start Wardriving") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("startwd");
    }

    if (strcmp(Selected_Option, "Stop Wardriving") == 0) {
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
        // Clear any state before switching views
        selected_item_index = 0;
        num_items = 0;
        menu_container = NULL;
        root = NULL;

        display_manager_switch_view(&main_menu_view);
        return; // Important: return immediately after initiating view switch
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

    if (strcmp(Selected_Option, "PineAP Detection") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("pineap");
    }

    if (strcmp(Selected_Option, "Scan Open Ports") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("scanports local -C");
    }

    if (strcmp(Selected_Option, "Reset AP Credentials") == 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        simulateCommand("apcred -r");
    }

    if (strcmp(Selected_Option, "Select AP") == 0) {
        if (scanned_aps) {
            set_number_pad_mode(NP_MODE_AP);
            display_manager_switch_view(&number_pad_view);
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            error_popup_create("You Need to Scan APs First...");
        }
    }

    if (strcmp(Selected_Option, "Select LAN") == 0) {
        set_number_pad_mode(NP_MODE_LAN);
        display_manager_switch_view(&number_pad_view);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void handle_option_directly(const char *Selected_Option) {
    lv_event_t e;
    e.user_data = (void *)Selected_Option;
    option_event_cb(&e);
}

void options_menu_destroy(void) {
    if (options_menu_view.root) {
        // First clean up any child objects
        if (menu_container) {
            lv_obj_clean(menu_container);
            menu_container = NULL;
        }

        // Then clean up the root object
        lv_obj_clean(options_menu_view.root);
        lv_obj_del(options_menu_view.root);
        options_menu_view.root = NULL;

        // Reset state
        selected_item_index = 0;
        num_items = 0;
    }
}

void get_options_menu_callback(void **callback) { *callback = options_menu_view.input_callback; }

View options_menu_view = {.root = NULL,
                          .create = options_menu_create,
                          .destroy = options_menu_destroy,
                          .input_callback = handle_hardware_button_press_options,
                          .name = "Options Screen",
                          .get_hardwareinput_callback = get_options_menu_callback};