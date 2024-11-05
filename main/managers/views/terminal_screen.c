#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "core/serial_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include "esp_log.h"
#include <string.h>

lv_obj_t *terminal_textarea = NULL;
char text_buffer[1024] = "";
uint32_t last_update = 0;
#define MAX_TEXT_LENGTH 4096

int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
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

    
    //default_log_vprintf = esp_log_set_vprintf(custom_log_vprintf); // This is very slow might have to find a alternative

    
    display_manager_add_status_bar("Terminal");
}

void terminal_view_destroy(void) {
    //esp_log_set_vprintf(default_log_vprintf);
    default_log_vprintf = NULL;
    if (terminal_view.root != NULL) {
        lv_obj_del(terminal_view.root);
        terminal_view.root = NULL;
        terminal_textarea = NULL;
    }
}

void terminal_view_add_text(const char *text) {
    if (terminal_textarea == NULL) return;

    // Append new text to the buffer
    strcat(text_buffer, text);
    strcat(text_buffer, "\n");

    // Get the current tick count
    TickType_t now = xTaskGetTickCount();
    if ((now - last_update) * portTICK_PERIOD_MS > 100) {
        lv_textarea_set_text(terminal_textarea, text_buffer);
        lv_textarea_set_cursor_pos(terminal_textarea, LV_TEXTAREA_CURSOR_LAST);
        last_update = now;

        text_buffer[0] = '\0';
    }
}

void terminal_view_hardwareinput_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        handle_serial_command("stop");
        handle_serial_command("stopspam");
        handle_serial_command("stopdeauth");
        handle_serial_command("capture -stop");
        display_manager_switch_view(&options_menu_view);
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            handle_serial_command("stop");
            handle_serial_command("stopspam");
            handle_serial_command("stopdeauth");
            handle_serial_command("capture -stop");
            display_manager_switch_view(&options_menu_view);
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