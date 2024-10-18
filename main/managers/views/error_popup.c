#include "managers/views/error_popup.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>


lv_obj_t *error_popup_root = NULL;
lv_obj_t *error_popup_label = NULL;


void error_popup_create(const char *message) {
    if (error_popup_root != NULL) {
        return;
    }

    
    error_popup_root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(error_popup_root, LV_HOR_RES * 0.8, LV_VER_RES * 0.5);
    lv_obj_align(error_popup_root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(error_popup_root, lv_color_make(50, 50, 50), 0);
    lv_obj_set_style_radius(error_popup_root, 10, 0);
    lv_obj_set_style_border_width(error_popup_root, 0, 0);
    lv_obj_set_style_shadow_color(error_popup_root, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_shadow_width(error_popup_root, 10, 0);
    lv_obj_set_style_shadow_opa(error_popup_root, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(error_popup_root, 20, 0);
    lv_obj_set_scrollbar_mode(error_popup_root, LV_SCROLLBAR_MODE_OFF);

    
    error_popup_label = lv_label_create(error_popup_root);
    lv_label_set_long_mode(error_popup_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(error_popup_label, lv_pct(100));
    lv_obj_set_style_text_color(error_popup_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(error_popup_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(error_popup_label, message);
    lv_obj_align(error_popup_label, LV_ALIGN_CENTER, 0, 0);


    vTaskDelay(pdMS_TO_TICKS(1000));

    error_popup_destroy();
}


void error_popup_destroy(void) {
    if (error_popup_root == NULL) {
        return;
    }
    lv_obj_del(error_popup_root);
    error_popup_root = NULL;
    error_popup_label = NULL;
}

bool is_error_popup_rendered()
{
    return error_popup_root;
}