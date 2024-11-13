#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "core/serial_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include "esp_log.h"
#include <string.h>

// Terminal text area object
lv_obj_t *terminal_textarea = NULL;

// Variables for touch handling
static bool touch_active = false;
static lv_point_t last_touch_point;

// Custom log function declaration
int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

// Create the terminal view
void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
    }

    // Create the root object for the terminal view
    terminal_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(terminal_view.root, LV_SCROLLBAR_MODE_OFF);

    // Create the text area for displaying logs
    terminal_textarea = lv_textarea_create(terminal_view.root);
    lv_obj_set_style_bg_color(terminal_textarea, lv_color_black(), 0);
    lv_textarea_set_one_line(terminal_textarea, false); // Allow multiple lines
    lv_textarea_set_text(terminal_textarea, "");
    lv_obj_set_size(terminal_textarea, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(terminal_textarea, LV_SCROLLBAR_MODE_AUTO); // Enable scrollbar
    lv_obj_add_flag(terminal_textarea, LV_OBJ_FLAG_SCROLLABLE); // Ensure the text area is scrollable
    lv_obj_clear_flag(terminal_textarea, LV_OBJ_FLAG_CLICKABLE); // Prevent text area from capturing clicks
    lv_obj_set_style_text_color(terminal_textarea, lv_color_hex(0x00FF00), 0); // Set text color to green
    lv_obj_set_style_text_font(terminal_textarea, &lv_font_montserrat_10, 0); // Set font size
    lv_obj_set_style_border_width(terminal_textarea, 0, 0); // Remove border
    // Removed maximum text length setting

    // Add a status bar to the terminal view
    display_manager_add_status_bar("Terminal");
}

// Destroy the terminal view
void terminal_view_destroy(void) {
    // Restore the default log function if modified
    // esp_log_set_vprintf(default_log_vprintf);
    default_log_vprintf = NULL;
    if (terminal_view.root != NULL) {
        lv_obj_del(terminal_view.root);
        terminal_view.root = NULL;
        terminal_textarea = NULL;
    }
}

// Add text to the terminal view
void terminal_view_add_text(const char *text) {
    if (terminal_textarea == NULL) return;

    // Append new text directly to the textarea
    lv_textarea_add_text(terminal_textarea, text);
    lv_textarea_add_text(terminal_textarea, "\n");

    // Removed cursor set to bottom calls
}

// Handle touch input for scrolling
void terminal_view_hardwareinput_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        // Extract touch point and state
        lv_point_t current_touch_point;
        current_touch_point.x = event->data.touch_data.point.x;
        current_touch_point.y = event->data.touch_data.point.y;
        lv_indev_state_t touch_state = event->data.touch_data.state;

        if (touch_state == LV_INDEV_STATE_PR && !touch_active) {
            // Touch started
            touch_active = true;
            last_touch_point = current_touch_point;
        } else if (touch_state == LV_INDEV_STATE_PR && touch_active) {
            // Touch moved
            int16_t delta_x = current_touch_point.x - last_touch_point.x;
            int16_t delta_y = current_touch_point.y - last_touch_point.y;

            // Scroll the text area based on the touch movement
            // Negate delta_y because moving finger up should scroll down
            lv_obj_scroll_by(terminal_textarea, 0, -delta_y, LV_ANIM_OFF);

            // Update the last touch point for the next calculation
            last_touch_point = current_touch_point;
        } else if (touch_state == LV_INDEV_STATE_REL && touch_active) {
            // Touch ended
            touch_active = false;
            // No momentum scrolling
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            // Handle joystick button press
            handle_serial_command("stop");
            handle_serial_command("stopspam");
            handle_serial_command("stopdeauth");
            handle_serial_command("capture -stop");
            display_manager_switch_view(&options_menu_view);
        }
    }
}

// Get the hardware input callback function
void terminal_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)terminal_view_hardwareinput_callback;
    }
}

// Custom log function to capture logs and display them in the terminal view
int custom_log_vprintf(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len < 0) {
        return len;
    }

    // Add the formatted log message to the terminal view
    terminal_view_add_text(buf);

    return len;
}

// Terminal view structure
View terminal_view = {
    .root = NULL,
    .create = terminal_view_create,
    .destroy = terminal_view_destroy,
    .input_callback = terminal_view_hardwareinput_callback,
    .name = "TerminalView",
    .get_hardwareinput_callback = terminal_view_get_hardwareinput_callback
};
