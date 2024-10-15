#include "managers/display_manager.h"
#include <stdlib.h>
#include "lvgl_helpers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LVGL_TASK_PERIOD_MS 5

static DisplayManager dm = { .current_view = NULL, .previous_view = NULL };

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
    printf("created Object");
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