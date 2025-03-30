#include "managers/views/number_pad_screen.h"
#include "core/serial_manager.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "managers/views/terminal_screen.h"
#include "managers/views/options_screen.h"
#include <stdio.h>
#include <string.h>

static ENumberPadMode current_mode = NP_MODE_AP;
static lv_obj_t *root = NULL;
static lv_obj_t *number_display = NULL;
static char input_buffer[32] = {0};
static int input_pos = 0;

static void number_pad_create();
static void number_pad_destroy();
static void handle_hardware_button_press_number_pad(InputEvent *event);
static void get_number_pad_callback(void **callback);

static void update_display() {
    lv_label_set_text(number_display, input_buffer);
}

static void add_digit(char digit) {
    if (input_pos < sizeof(input_buffer) - 1) {
        input_buffer[input_pos++] = digit;
        input_buffer[input_pos] = '\0';
        update_display();
    }
}

static void remove_digit() {
    if (input_pos > 0) {
        input_buffer[--input_pos] = '\0';
        update_display();
    }
}

static void submit_number() {
    if (input_pos > 0) {
        display_manager_switch_view(&terminal_view);
        vTaskDelay(pdMS_TO_TICKS(10));
        
        char command[64];
        if (current_mode == NP_MODE_AP) {
            snprintf(command, sizeof(command), "select -a %s", input_buffer);
        } else {
            snprintf(command, sizeof(command), "select -a %s", input_buffer);
        }
        simulateCommand(command);
        
        // Reset buffer
        input_buffer[0] = '\0';
        input_pos = 0;
    }
}

static void number_button_cb(lv_event_t *e) {
    const char *digit = (const char *)lv_event_get_user_data(e);
    
    if (strcmp(digit, "DEL") == 0) {
        remove_digit();
    } else if (strcmp(digit, "OK") == 0) {
        submit_number();
    } else if (strcmp(digit, "BACK") == 0) {
        display_manager_switch_view(&options_menu_view);
    } else {
        add_digit(digit[0]);
    }
}

static void number_pad_create() {
    int screen_width = LV_HOR_RES;
    int screen_height = LV_VER_RES;
    
    // Calculate button sizes based on screen resolution
    int btn_width = screen_width / 4 - 10;
    int btn_height = (screen_height - 100) / 5 - 10;
    
    display_manager_fill_screen(lv_color_hex(0x121212));
    
    root = lv_obj_create(lv_scr_act());
    number_pad_view.root = root;
    lv_obj_set_size(root, screen_width, screen_height);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x121212), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_align(root, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    
    // Number display area
    number_display = lv_label_create(root);
    lv_obj_set_width(number_display, screen_width - 20);
    lv_obj_set_style_bg_color(number_display, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_text_color(number_display, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(number_display, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_all(number_display, 10, 0);
    lv_obj_align(number_display, LV_ALIGN_TOP_MID, 0, 10);
    update_display();
    
    // Number pad container
    lv_obj_t *pad_container = lv_obj_create(root);
    lv_obj_set_size(pad_container, screen_width, screen_height - 70);
    lv_obj_align(pad_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(pad_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(pad_container, 5, 0);
    
    // Button labels
    const char *buttons[] = {
        "1", "2", "3", "DEL",
        "4", "5", "6", "OK",
        "7", "8", "9", "BACK",
        "0", "", "", "",
        NULL
    };
    
    // Create number pad buttons
    int x = 0, y = 0;
    for (int i = 0; buttons[i] != NULL; i++) {
        if (strlen(buttons[i]) == 0) {
            x++;
            continue;
        }
        
        lv_obj_t *btn = lv_btn_create(pad_container);
        lv_obj_set_size(btn, btn_width, btn_height);
        lv_obj_set_pos(btn, x * (btn_width + 5), y * (btn_height + 5));
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, buttons[i]);
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        
        lv_obj_add_event_cb(btn, number_button_cb, LV_EVENT_CLICKED, (void *)buttons[i]);
        
        x++;
        if (x >= 4) {
            x = 0;
            y++;
        }
    }
    
    // Add status bar
    const char *title = (current_mode == NP_MODE_AP) ? "Select AP Num" : "Select LAN Num";
    display_manager_add_status_bar(title);
}

static void number_pad_destroy() {
    if (number_pad_view.root) {
        lv_obj_clean(number_pad_view.root);
        lv_obj_del(number_pad_view.root);
        number_pad_view.root = NULL;
        root = NULL;
        number_display = NULL;
        input_buffer[0] = '\0';
        input_pos = 0;
    }
}

static void handle_hardware_button_press_number_pad(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        lv_indev_data_t *data = &event->data.touch_data;
        
        int screen_width = LV_HOR_RES;
        int screen_height = LV_VER_RES;
        int btn_width = screen_width / 4 - 10;
        int btn_height = (screen_height - 100) / 5 - 10;
        
        int x = data->point.x / (btn_width + 5);
        int y = (data->point.y - 70) / (btn_height + 5);
        
        if (x >= 0 && x < 4 && y >= 0 && y < 4) {
            int index = y * 4 + x;
            const char *buttons[] = {"1", "2", "3", "DEL",
                                   "4", "5", "6", "OK",
                                   "7", "8", "9", "BACK",
                                   "0", "", "", ""};
            if (index < 12 && strlen(buttons[index]) > 0) {
                number_button_cb(&(lv_event_t){.user_data = (void *)buttons[index]});
            }
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 2) {  // Up
            remove_digit();
        } else if (button == 4) {  // Down
            add_digit('0');
        } else if (button == 1) {  // Select
            submit_number();
        } else if (button == 3) {  // Back
            display_manager_switch_view(&options_menu_view);
        }
    }
}

static void get_number_pad_callback(void **callback) {
    *callback = number_pad_view.input_callback;
}

View number_pad_view = {
    .root = NULL,
    .create = number_pad_create,
    .destroy = number_pad_destroy,
    .input_callback = handle_hardware_button_press_number_pad,
    .name = "Number Pad Screen",
    .get_hardwareinput_callback = get_number_pad_callback
};


void set_number_pad_mode(ENumberPadMode mode) {
    current_mode = mode;
}