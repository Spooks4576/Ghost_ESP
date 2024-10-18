#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include <stdlib.h>
#include <string.h>

lv_obj_t *terminal_label = NULL;


void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
    }

    
    terminal_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(terminal_view.root, LV_SCROLLBAR_MODE_AUTO);

    
    terminal_label = lv_label_create(terminal_view.root);
    lv_label_set_recolor(terminal_label, true);
    lv_obj_set_style_text_color(terminal_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_width(terminal_label, lv_pct(100));
    lv_label_set_long_mode(terminal_label, LV_LABEL_LONG_WRAP);

    
    lv_obj_align(terminal_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(terminal_label, "");
}


void terminal_view_destroy(void) {
    if (terminal_view.root != NULL) {
        lv_obj_del(terminal_view.root);
        terminal_view.root = NULL;
        terminal_label = NULL;
    }
}


void terminal_view_add_text(const char *text) {
    if (terminal_label == NULL) {
        return;
    }

    
    const char *current_text = lv_label_get_text(terminal_label);
    size_t new_length = strlen(current_text) + strlen(text) + 2;
    char *new_text = malloc(new_length);
    if (new_text == NULL) {
        return;
    }

    snprintf(new_text, new_length, "%s\n%s", current_text, text);


    lv_label_set_text(terminal_label, new_text);


    free(new_text);


    lv_obj_scroll_to_y(terminal_label, lv_obj_get_scroll_bottom(terminal_label), LV_ANIM_ON);
}


void terminal_view_hardwareinput_callback(int input) {
    if (input == 1)
    {
        display_manager_switch_view(&main_menu_view);
    }
}


void terminal_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)terminal_view_hardwareinput_callback;
    }
}

View terminal_view = {
    .root = NULL,
    .create = terminal_view_create,
    .destroy = terminal_view_destroy,
    .hardwareinput_callback = terminal_view_hardwareinput_callback,
    .name = "TerminalView",
    .get_hardwareinput_callback = terminal_view_get_hardwareinput_callback
};