#include "managers/display_manager.h"
#include <stdlib.h>
#include "lvgl_helpers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "managers/sd_card_manager.h"

#define LVGL_TASK_PERIOD_MS 5

static DisplayManager dm = { .current_view = NULL, .previous_view = NULL };

static lv_obj_t *status_bar;
lv_obj_t *wifi_label = NULL;
lv_obj_t *bt_label = NULL;
lv_obj_t *sd_label = NULL;
lv_obj_t *battery_label = NULL;

void update_status_bar(bool wifi_enabled, bool bt_enabled, bool sd_card_mounted, int batteryPercentage) {
    if (wifi_enabled) {
        if (wifi_label == NULL) {
            wifi_label = lv_label_create(status_bar);
            lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
            lv_obj_align(wifi_label, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_style_text_color(wifi_label, lv_color_white(), 0);
        }
    } else if (wifi_label != NULL) {
        lv_obj_del(wifi_label);
        wifi_label = NULL;
    }

    if (bt_enabled) {
        if (bt_label == NULL) {
            bt_label = lv_label_create(status_bar);
            lv_label_set_text(bt_label, LV_SYMBOL_BLUETOOTH);
            lv_obj_align(bt_label, LV_ALIGN_LEFT_MID, 25, 0);
            lv_obj_set_style_text_color(bt_label, lv_color_white(), 0);
        }
    } else if (bt_label != NULL) {
        lv_obj_del(bt_label);
        bt_label = NULL;
    }


    if (sd_card_mounted) {
        if (sd_label == NULL) {
            sd_label = lv_label_create(status_bar);
            lv_label_set_text(sd_label, LV_SYMBOL_SD_CARD);
            lv_obj_align(sd_label, LV_ALIGN_LEFT_MID, 50, 0);
            lv_obj_set_style_text_color(sd_label, lv_color_white(), 0);
        }
    } else if (sd_label != NULL) {
        lv_obj_del(sd_label);
        sd_label = NULL;
    }

    const char* battery_symbol;


    if (batteryPercentage < 10) {
        battery_symbol = LV_SYMBOL_BATTERY_EMPTY;
    } else if (batteryPercentage < 30) {
        battery_symbol = LV_SYMBOL_BATTERY_1;
    } else if (batteryPercentage < 60) {
        battery_symbol = LV_SYMBOL_BATTERY_2;
    } else if (batteryPercentage < 80) {
        battery_symbol = LV_SYMBOL_BATTERY_3;
    } else {
        battery_symbol = LV_SYMBOL_BATTERY_FULL;
    }

    if (batteryPercentage == 1000) // Marker to signal charging / No Battery
    {
        battery_symbol = LV_SYMBOL_CHARGE;
        batteryPercentage = 100;
    }

    if (battery_label == NULL) {
        battery_label = lv_label_create(status_bar);
        lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -5, 0);
        lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
    }

    lv_label_set_text_fmt(battery_label, "%s %d%%", battery_symbol, batteryPercentage);


    lv_obj_invalidate(status_bar);
}

void display_manager_add_status_bar(const char* CurrentMenuName)
{
    status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(status_bar, LV_HOR_RES, 20);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);

    
    lv_obj_set_style_bg_color(status_bar, lv_color_black(), LV_PART_MAIN);
    
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(status_bar, lv_color_white(), LV_PART_MAIN); // Previous color lv_color_hex(0x393939)
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mainlabel = lv_label_create(status_bar);
    lv_label_set_text(mainlabel, CurrentMenuName);
    lv_obj_align(mainlabel, LV_ALIGN_CENTER, -5, 0);
    lv_obj_set_style_text_color(mainlabel, lv_color_white(), 0);

    bool HasBluetooth;

#ifndef CONFIG_IDF_TARGET_ESP32S2
    HasBluetooth = true;
#else
    HasBluetooth = false;
#endif

    update_status_bar(true, HasBluetooth, sd_card_exists("/mnt/ghostesp"), 1000);
}

void display_manager_init(void) {
    lv_init();
    lvgl_driver_init();

    static lv_color_t buf1[240 * 20] __attribute__((aligned(4)));
    static lv_color_t buf2[240 * 20] __attribute__((aligned(4)));
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, 240 * 20);

    /* Initialize the display */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register(&indev_drv);

    xTaskCreate(lvgl_tick_task, "LVGL Tick Task", 4096, NULL, 5, NULL);
}

bool display_manager_register_view(View *view) {
    if (view == NULL || view->create == NULL || view->destroy == NULL) {
        return false;
    }
    return true;
}

void display_manager_switch_view(View *view) {
    if (view == NULL) return;
    display_manager_destroy_current_view();


    dm.previous_view = dm.current_view;
    dm.current_view = view;
    view->create();
}

void display_manager_destroy_current_view(void) {
    if (dm.current_view && dm.current_view->destroy) {
        dm.current_view->destroy();
        dm.current_view = NULL;
    }
}

View *display_manager_get_current_view(void) {
    return dm.current_view;
}

void display_manager_fill_screen(lv_color_t color)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, color); 
    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(lv_scr_act(), &style, LV_PART_MAIN | LV_STATE_DEFAULT);
}


void lvgl_tick_task(void *arg) {
    const TickType_t tick_interval = pdMS_TO_TICKS(10);

    while (1) {
        lv_timer_handler();
        lv_tick_inc(LVGL_TASK_PERIOD_MS);
        vTaskDelay(tick_interval);
    }
}