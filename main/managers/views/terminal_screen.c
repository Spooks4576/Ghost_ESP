#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/options_screen.h" // Include options screen header
#include "core/serial_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include "esp_log.h"
#include <string.h>

// Terminal text area object
lv_obj_t *terminal_textarea = NULL;

// Buffer for storing text before updating the textarea
char text_buffer[1024] = "";
uint32_t last_update = 0;
#define MAX_TEXT_LENGTH 4096

// Custom log function declaration
int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

// Create the terminal view
void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
    }

    terminal_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);

    // Create the text area for displaying logs
    terminal_textarea = lv_textarea_create(terminal_view.root);
    lv_obj_set_style_bg_color(terminal_textarea, lv_color_black(), 0);
    lv_textarea_set_one_line(terminal_textarea, false); // Allow multiple lines
    lv_textarea_set_text(terminal_textarea, "");
    lv_obj_set_size(terminal_textarea, LV_HOR_RES, LV_VER_RES);

    // Enable scrollbar and ensure the text area is scrollable
    lv_obj_set_scrollbar_mode(terminal_textarea, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(terminal_textarea, LV_OBJ_FLAG_SCROLLABLE);
    // Prevent text area from capturing clicks
    lv_obj_clear_flag(terminal_textarea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(terminal_textarea, lv_color_hex(0x00FF00), 0); // Set text color to green
    lv_obj_set_style_text_font(terminal_textarea, &lv_font_montserrat_10, 0); // Set font size
    lv_obj_set_style_border_width(terminal_textarea, 0, 0); // Remove border

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

    // Append new text to the buffer
    strcat(text_buffer, text);
    strcat(text_buffer, "\n");

    // Update the textarea periodically to prevent performance issues
    TickType_t now = xTaskGetTickCount();
    if ((now - last_update) * portTICK_PERIOD_MS > 100) {
        lv_textarea_set_text(terminal_textarea, text_buffer);

        // Scroll to the bottom to show the new text
        lv_textarea_set_cursor_pos(terminal_textarea, LV_TEXTAREA_CURSOR_LAST);
        lv_textarea_scroll_to_cursor(terminal_textarea);

        last_update = now;
        text_buffer[0] = '\0';
    }
}

// Handle touch input for scrolling and exiting
void terminal_view_hardwareinput_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        // Handle touch release events
        if (event->data.touch_data.state == LV_INDEV_STATE_REL) {
            // Extract touch Y coordinate
            int y = event->data.touch_data.point.y;

            int screen_height = LV_VER_RES;
            int third_height = screen_height / 3;

            if (y < third_height) {
                // Touch is in the top third - scroll up
                lv_obj_scroll_by(terminal_textarea, 0, -50, LV_ANIM_OFF); // Adjust scroll amount as needed
            } else if (y > 2 * third_height) {
                // Touch is in the bottom third - scroll down
                lv_obj_scroll_by(terminal_textarea, 0, 50, LV_ANIM_OFF); // Adjust scroll amount as needed
            } else {
                // Touch is in the middle third - exit terminal view
                handle_serial_command("stop");
                handle_serial_command("stopspam");
                handle_serial_command("stopdeauth");
                handle_serial_command("capture -stop");
                // Switch back to options menu view
                display_manager_switch_view(&options_menu_view);
            }
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            handle_serial_command("stop");
            handle_serial_command("stopspam");
            handle_serial_command("stopdeauth");
            handle_serial_command("capture -stop");
            // Switch back to options menu view
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
