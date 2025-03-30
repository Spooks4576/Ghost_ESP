#include "managers/views/error_popup.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static lv_obj_t *error_popup_root = NULL;
static lv_obj_t *error_popup_label = NULL;
static SemaphoreHandle_t popup_mutex = NULL;

#define DISPLAY_DURATION_MS 1500  // Duration popup stays visible
#define ANIMATION_TIME_MS 150     // Faster animation duration for fade in/out

static void error_popup_destroy_task(void *param);

// Animation callback for fade in/out
static void fade_anim_cb(void *obj, int32_t value) {
    lv_obj_set_style_opa(obj, value, 0);
}

void error_popup_destroy(void) {
    if (error_popup_root == NULL) {
        return;
    }
    
    if (xSemaphoreTake(popup_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (error_popup_root) {
            lv_obj_del(error_popup_root);
            error_popup_root = NULL;
            error_popup_label = NULL;
        }
        xSemaphoreGive(popup_mutex);
    }
}

void error_popup_create(const char *message) {
    // Initialize mutex if not already done
    if (!popup_mutex) {
        popup_mutex = xSemaphoreCreateMutex();
        if (!popup_mutex) {
            return;
        }
    }

    // If a popup already exists, destroy it first
    if (error_popup_root != NULL) {
        error_popup_destroy();
    }

    if (xSemaphoreTake(popup_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    // Create popup as an overlay
    error_popup_root = lv_obj_create(lv_layer_top());
    lv_obj_clear_flag(error_popup_root, LV_OBJ_FLAG_SCROLLABLE);

    // Smaller size and top-aligned based on screen resolution
    int popup_width, popup_height, padding, font_size;
    if (LV_HOR_RES == 240 && LV_VER_RES == 320) {
        popup_width = LV_HOR_RES * 0.7;    // 70% width
        popup_height = LV_VER_RES * 0.2;   // 20% height
        padding = 10;
        font_size = 12;
    } else if (LV_HOR_RES == 128 && LV_VER_RES == 128) {
        popup_width = LV_HOR_RES * 0.8;    // 80% width
        popup_height = LV_VER_RES * 0.25;  // 25% height
        padding = 5;
        font_size = 8;
    } else {
        popup_width = LV_HOR_RES * 0.6;    // 60% width
        popup_height = LV_VER_RES * 0.15;  // 15% height
        padding = 10;
        font_size = 12;
    }

    lv_obj_set_size(error_popup_root, popup_width, popup_height);
    lv_obj_align(error_popup_root, LV_ALIGN_TOP_MID, 0, 20);  // 20px from top

    // Improved styling
    lv_obj_set_style_bg_color(error_popup_root, lv_color_hex(0x323232), 0);  // Darker gray
    lv_obj_set_style_radius(error_popup_root, 8, 0);                          // Softer corners
    lv_obj_set_style_border_width(error_popup_root, 1, 0);
    lv_obj_set_style_border_color(error_popup_root, lv_color_hex(0x555555), 0);
    lv_obj_set_style_shadow_color(error_popup_root, lv_color_black(), 0);
    lv_obj_set_style_shadow_width(error_popup_root, 5, 0);
    lv_obj_set_style_shadow_opa(error_popup_root, LV_OPA_60, 0);
    lv_obj_set_style_opa(error_popup_root, LV_OPA_0, 0);  // Start transparent
    lv_obj_set_style_pad_all(error_popup_root, padding, 0);

    // Create label with better alignment
    error_popup_label = lv_label_create(error_popup_root);
    lv_label_set_long_mode(error_popup_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(error_popup_label, popup_width - 2 * padding, popup_height - 2 * padding);
    lv_obj_set_style_text_color(error_popup_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(error_popup_label, 
        font_size == 8 ? &lv_font_montserrat_8 : &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(error_popup_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(error_popup_label, message);
    lv_obj_align(error_popup_label, LV_ALIGN_CENTER, 0, 0);

    // Fade in animation
    lv_anim_t fade_in;
    lv_anim_init(&fade_in);
    lv_anim_set_var(&fade_in, error_popup_root);
    lv_anim_set_values(&fade_in, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_time(&fade_in, ANIMATION_TIME_MS);
    lv_anim_set_exec_cb(&fade_in, fade_anim_cb);
    lv_anim_start(&fade_in);

    // Create task to handle fade out and destruction
    xTaskCreate(error_popup_destroy_task, "error_popup_destroy", 2048, NULL, 5, NULL);

    xSemaphoreGive(popup_mutex);
}

static void error_popup_destroy_task(void *param) {
    // Wait for display duration
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_DURATION_MS));

    if (error_popup_root && xSemaphoreTake(popup_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Fade out animation
        lv_anim_t fade_out;
        lv_anim_init(&fade_out);
        lv_anim_set_var(&fade_out, error_popup_root);
        lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_0);
        lv_anim_set_time(&fade_out, ANIMATION_TIME_MS);
        lv_anim_set_exec_cb(&fade_out, fade_anim_cb);
        lv_anim_start(&fade_out);

        // Wait for fade out to complete
        vTaskDelay(pdMS_TO_TICKS(ANIMATION_TIME_MS));

        // Destroy the popup
        error_popup_destroy();
        xSemaphoreGive(popup_mutex);
    }

    vTaskDelete(NULL);
}

bool is_error_popup_rendered(void) {
    return error_popup_root != NULL;
}