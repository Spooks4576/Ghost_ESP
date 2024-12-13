#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "core/serial_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Terminal";
static lv_obj_t *terminal_textarea = NULL;
static SemaphoreHandle_t terminal_mutex = NULL;
#define MAX_TEXT_LENGTH 4096
#define CLEANUP_THRESHOLD (MAX_TEXT_LENGTH * 3 / 4)
#define CLEANUP_AMOUNT (MAX_TEXT_LENGTH / 2)

int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
    }
   
    // Create mutex if it doesn't exist
    if (!terminal_mutex) {
        terminal_mutex = xSemaphoreCreateMutex();
        if (!terminal_mutex) {
            ESP_LOGE(TAG, "Failed to create terminal mutex");
            return;
        }
    }

    terminal_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(terminal_view.root, LV_SCROLLBAR_MODE_OFF);

    terminal_textarea = lv_textarea_create(terminal_view.root);
    lv_obj_set_style_bg_color(terminal_textarea, lv_color_black(), 0);
    lv_textarea_set_one_line(terminal_textarea, false);
    lv_textarea_set_text(terminal_textarea, "");
    lv_obj_set_size(terminal_textarea, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(terminal_textarea, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_color(terminal_textarea, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(terminal_textarea, &lv_font_montserrat_10, 0);
    lv_obj_set_style_border_width(terminal_textarea, 0, 0);

    display_manager_add_status_bar("Terminal");
}

void terminal_view_destroy(void) {
    if (terminal_mutex) {
        vSemaphoreDelete(terminal_mutex);
        terminal_mutex = NULL;
    }

    if (terminal_view.root != NULL) {
        lv_obj_del(terminal_view.root);
        terminal_view.root = NULL;
        terminal_textarea = NULL;
    }
}

void terminal_view_add_text(const char *text) {
    if (!terminal_textarea || !text) {
        return;
    }

    // Try to take mutex with timeout
    if (xSemaphoreTake(terminal_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire terminal mutex");
        return;
    }

    // Get current text length
    const char *current_text = lv_textarea_get_text(terminal_textarea);
    size_t current_length = strlen(current_text);
    size_t new_text_length = strlen(text);

    // Check if we need to clean up
    if (current_length + new_text_length > CLEANUP_THRESHOLD) {
        // Find the position to start keeping text
        const char *keep_pos = current_text + CLEANUP_AMOUNT;
        
        // Find the start of the next line to maintain clean output
        while (*keep_pos && *keep_pos != '\n') {
            keep_pos++;
        }
        if (*keep_pos == '\n') {
            keep_pos++; // Skip the newline
        }

        // Set the remaining text
        lv_textarea_set_text(terminal_textarea, keep_pos);
    }

    // Add new text and scroll to end
    lv_textarea_add_text(terminal_textarea, text);
    lv_textarea_set_cursor_pos(terminal_textarea, LV_TEXTAREA_CURSOR_LAST);

    xSemaphoreGive(terminal_mutex);
}

void terminal_view_hardwareinput_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        int touch_y = event->data.touch_data.point.y;

        if (touch_y < LV_VER_RES / 3) {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_up(terminal_textarea);
            }
        } else if (touch_y > (LV_VER_RES * 2) / 3) {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_down(terminal_textarea);
            }
        } else {
            display_manager_switch_view(&options_menu_view);
            simulateCommand("stop");
            simulateCommand("stopspam");
            simulateCommand("stopdeauth");
            simulateCommand("capture -stop");
            simulateCommand("stopportal");
            simulateCommand("gpsinfo -s");
            simulateCommand("blewardriving -s");
            return;
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            display_manager_switch_view(&options_menu_view);
            simulateCommand("stop");
            simulateCommand("stopspam");
            simulateCommand("stopdeauth");
            simulateCommand("capture -stop");
            simulateCommand("stopportal");
            simulateCommand("gpsinfo -s");
            simulateCommand("blewardriving -s");
            return;
        }

        if (button == 2)
        {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_up(terminal_textarea);
            }
        }

        if (button == 4)
        {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_down(terminal_textarea);
            }
        }
    }
}

void terminal_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)terminal_view_hardwareinput_callback;
    }
}

int custom_log_vprintf(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len < 0) {
        return len;
    }

    terminal_view_add_text(buf);

    return len;
}

View terminal_view = {
    .root = NULL,
    .create = terminal_view_create,
    .destroy = terminal_view_destroy,
    .input_callback = terminal_view_hardwareinput_callback,
    .name = "TerminalView",
    .get_hardwareinput_callback = terminal_view_get_hardwareinput_callback
};