#include "managers/display_manager.h"
#include <stdlib.h>
#include "lvgl_helpers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "managers/sd_card_manager.h"
#include "freertos/semphr.h"
#include "managers/views/error_popup.h"
#include "managers/views/options_screen.h"
#include "managers/views/main_menu_screen.h"

#define LVGL_TASK_PERIOD_MS 5

DisplayManager dm = { .current_view = NULL, .previous_view = NULL };

lv_obj_t *status_bar;
lv_obj_t *wifi_label = NULL;
lv_obj_t *bt_label = NULL;
lv_obj_t *sd_label = NULL;
lv_obj_t *battery_label = NULL;



#define FADE_DURATION_MS 50


void fade_out_cb(void *obj, int32_t v) {
    if (obj)
    {
        lv_obj_set_style_opa(obj, v, LV_PART_MAIN);
    }
}


void fade_in_cb(void *obj, int32_t v) {
    if (obj)
    {
        lv_obj_set_style_opa(obj, v, LV_PART_MAIN);
    }
}


void display_manager_fade_out(lv_obj_t *obj, lv_anim_ready_cb_t ready_cb, View* view) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&anim, FADE_DURATION_MS);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)fade_out_cb);
    lv_anim_set_ready_cb(&anim, ready_cb);
    lv_anim_set_user_data(&anim, view);
    lv_anim_start(&anim);
}


void display_manager_fade_in(lv_obj_t *obj) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim, FADE_DURATION_MS);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)fade_in_cb);
    lv_anim_start(&anim);
}


void fade_out_ready_cb(lv_anim_t *anim) {
    display_manager_destroy_current_view();

    View *new_view = (View *)anim->user_data;
    if (new_view) {
        dm.previous_view = dm.current_view;
        dm.current_view = new_view;
        
        if (new_view->get_hardwareinput_callback) {
            new_view->input_callback(&dm.current_view->input_callback);
        }

        new_view->create();
        display_manager_fade_in(new_view->root);
        display_manager_fade_in(status_bar);
    }
}

void update_status_bar(bool wifi_enabled, bool bt_enabled, bool sd_card_mounted, int batteryPercentage) {
    lv_disp_t *disp = lv_disp_get_default();
    int hor_res = lv_disp_get_hor_res(disp);


    int wifi_pos_x = hor_res / 50;
    int bt_pos_x = hor_res / 10;
    int sd_pos_x = hor_res / 6;
    int battery_pos_x = hor_res - (hor_res / 20) - 5;

    // Only render Wi-Fi, Bluetooth, and SD icons if the width is greater than 128
    bool render_icons = (hor_res > 128);

   
    if (render_icons && wifi_enabled) {
        if (wifi_label == NULL) {
            wifi_label = lv_label_create(status_bar);
            lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
            lv_obj_align(wifi_label, LV_ALIGN_LEFT_MID, wifi_pos_x + -5, 0);
            lv_obj_set_style_text_color(wifi_label, lv_color_white(), 0);
        }
    } else if (wifi_label != NULL) {
        lv_obj_del(wifi_label);
        wifi_label = NULL;
    }

    
    if (render_icons && bt_enabled) {
        if (bt_label == NULL) {
            bt_label = lv_label_create(status_bar);
            lv_label_set_text(bt_label, LV_SYMBOL_BLUETOOTH);
            lv_obj_align(bt_label, LV_ALIGN_LEFT_MID, bt_pos_x, 0);
            lv_obj_set_style_text_color(bt_label, lv_color_white(), 0);
        }
    } else if (bt_label != NULL) {
        lv_obj_del(bt_label);
        bt_label = NULL;
    }

   
    if (render_icons && sd_card_mounted) {
        if (sd_label == NULL) {
            sd_label = lv_label_create(status_bar);
            lv_label_set_text(sd_label, LV_SYMBOL_SD_CARD);
            lv_obj_align(sd_label, LV_ALIGN_LEFT_MID, sd_pos_x, 0);
            lv_obj_set_style_text_color(sd_label, lv_color_white(), 0);
        }
    } else if (sd_label != NULL) {
        lv_obj_del(sd_label);
        sd_label = NULL;
    }

    
    const char *battery_symbol;
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

    if (batteryPercentage == 1000) { // Marker to signal charging / No Battery
        battery_symbol = LV_SYMBOL_CHARGE;
        batteryPercentage = 100;
    }


    if (battery_label == NULL) {
        battery_label = lv_label_create(status_bar);
        lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -5, 0); 
        lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(battery_label, render_icons ? &lv_font_montserrat_16 : &lv_font_montserrat_10, 0);
    }
    else 
    {
        lv_obj_del(battery_label); // Fix Battery Not Showing up on other menus
        battery_label = lv_label_create(status_bar);
        lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -5, 0); 
        lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(battery_label, render_icons ? &lv_font_montserrat_16 : &lv_font_montserrat_10, 0);
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

    lv_disp_t *disp = lv_disp_get_default();
    int hor_res = lv_disp_get_hor_res(disp);

    lv_obj_t* mainlabel = lv_label_create(status_bar);
    lv_label_set_text(mainlabel, CurrentMenuName);
    lv_obj_align(mainlabel, hor_res > 128 ? LV_ALIGN_CENTER : LV_ALIGN_LEFT_MID, hor_res > 128 ? -15 : -5, 0);
    lv_obj_set_style_text_color(mainlabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(mainlabel, hor_res > 128 ? &lv_font_montserrat_16 : &lv_font_montserrat_10, 0);

    bool HasBluetooth;

#ifndef CONFIG_IDF_TARGET_ESP32S2
    HasBluetooth = true;
#else
    HasBluetooth = false;
#endif

    update_status_bar(true, HasBluetooth, sd_card_exists("/mnt/ghostesp"), 1000);
}


void apply_calibration_to_point(lv_point_t *point, uint16_t *calData, int screen_width, int screen_height) {
    uint16_t x_min = calData[0];
    uint16_t x_max = calData[1];
    uint16_t y_min = calData[2];
    uint16_t y_max = calData[3];
    uint8_t invert_xy = calData[4];

    int32_t x = point->x;
    int32_t y = point->y;

    
    x = (x - x_min) * (screen_width - 1) / (x_max - x_min);
    y = (y - y_min) * (screen_height - 1) / (y_max - y_min);

    
    if (invert_xy & 0x01) {
        int32_t temp = x;
        x = y;
        y = temp;
    }
    if (invert_xy & 0x02) {
        x = screen_width - 1 - x;
    }
    if (invert_xy & 0x04) {
        y = screen_height - 1 - y;
    }

    
    point->x = x;
    point->y = y;
}

void display_manager_init(void) {
    lv_init();
    lvgl_driver_init();

    static lv_color_t buf1[TFT_WIDTH * 20] __attribute__((aligned(4)));
    static lv_color_t buf2[TFT_WIDTH * 20] __attribute__((aligned(4)));
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, TFT_WIDTH * 20);

    /* Initialize the display */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    dm.mutex = xSemaphoreCreateMutex();
    if (dm.mutex == NULL) {
        printf("Failed to create mutex\n");
        return;
    }

    input_queue = xQueueCreate(10, sizeof(InputEvent));
    if (input_queue == NULL) {
        printf("Failed to create input queue\n");
        return;
    }

    xTaskCreate(lvgl_tick_task, "LVGL Tick Task", 4096, NULL, RENDERING_TASK_PRIORITY, NULL);
    xTaskCreate(&input_processing_task, "InputProcessing", 4096, NULL, INPUT_PROCESSING_TASK_PRIORITY, NULL);
    xTaskCreate(&hardware_input_task, "RawInput", 4096, NULL, HARDWARE_INPUT_TASK_PRIORITY, NULL);
}

bool display_manager_register_view(View *view) {
    if (view == NULL || view->create == NULL || view->destroy == NULL) {
        return false;
    }
    return true;
}

void display_manager_switch_view(View *view) {
    if (view == NULL) return;

    if (xSemaphoreTake(dm.mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
        printf("Switching view from %s to %s\n", 
            dm.current_view ? dm.current_view->name : "NULL", 
            view->name);

        if (dm.current_view && dm.current_view->root) {
            if (status_bar)
            {
                lv_obj_del(status_bar);
                wifi_label = NULL;
                bt_label = NULL;
                sd_label = NULL;
                battery_label = NULL;
                status_bar = NULL;
            }
            display_manager_fade_out(dm.current_view->root, fade_out_ready_cb, view);
        } else {
            dm.previous_view = dm.current_view;
            dm.current_view = view;

            if (view->get_hardwareinput_callback) {
                view->input_callback(&dm.current_view->input_callback);
            }

            view->create();
            display_manager_fade_in(view->root);
        }

        xSemaphoreGive(dm.mutex);
    } else {
        printf("Failed to acquire mutex for switching view\n");
    }
}

void display_manager_destroy_current_view(void) {
    if (dm.current_view) {
        if (dm.current_view->destroy) {
            dm.current_view->destroy();
        }

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

void delete_circle_callback(lv_timer_t *timer) {
    lv_obj_t *circle = (lv_obj_t *)timer->user_data;  // Access the user_data directly
    lv_obj_del(circle);  // Delete the circle object
    lv_timer_del(timer); // Delete the timer to avoid memory leaks
}

void hardware_input_task(void *pvParameters) {
    const TickType_t tick_interval = pdMS_TO_TICKS(10);

    lv_indev_drv_t touch_driver;
    lv_indev_data_t touch_data;
    uint16_t calData[5] = { 339, 3470, 237, 3438, 2 };
    bool touch_active = false;
    int screen_width = LV_HOR_RES;
    int screen_height = LV_VER_RES;

    while (1) {
        #ifdef USE_JOYSTICK
            for (int i = 0; i < 5; i++) {
                if (joysticks[i].pin >= 0) {
                    if (joystick_just_pressed(&joysticks[i])) {
                        InputEvent event;
                        event.type = INPUT_TYPE_JOYSTICK;
                        event.data.joystick_index = i;

                        if (xQueueSend(input_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
                            printf("Failed to send joystick input to queue\n");
                        }
                    }
                }
            }
        #endif

        #ifdef USE_TOUCHSCREEN
            touch_driver_read(&touch_driver, &touch_data);

            if (touch_data.state == LV_INDEV_STATE_PR && !touch_active) {
                touch_active = true;

                InputEvent event;
                event.type = INPUT_TYPE_TOUCH;
                event.data.touch_data.point.x = touch_data.point.x;
                event.data.touch_data.point.y = touch_data.point.y;
                event.data.touch_data.state = touch_data.state;

                if (xQueueSend(input_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
                    printf("Failed to send touch input to queue\n");
                }

                
                lv_obj_t *circle = lv_obj_create(lv_scr_act()); 
                lv_obj_set_size(circle, 10, 10);
                lv_obj_set_style_bg_color(circle, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);  // Set the circle color to red
                lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);  // Make the object circular
                lv_obj_align_to(circle, lv_scr_act(), LV_ALIGN_TOP_LEFT, touch_data.point.x - 5, touch_data.point.y - 5);  // Position the circle at the touch point

                // Set a timer to delete the circle after 500 ms
                lv_timer_t *timer = lv_timer_create(delete_circle_callback, 500, circle);
                timer->user_data = circle;  // Assign the circle object to the timer's user data
            }
            else if (touch_data.state == LV_INDEV_STATE_REL && touch_active) {
                touch_active = false;
            }
        #endif

        vTaskDelay(tick_interval);
    }

    vTaskDelete(NULL);
}


void input_processing_task(void *pvParameters) {
    InputEvent event;

    while (1) {
        if (xQueueReceive(input_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (xSemaphoreTake(dm.mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
                View *current = dm.current_view;
                void (*input_callback)(InputEvent*) = NULL;
                const char* view_name = "NULL";

                if (current) {
                    view_name = current->name;
                    input_callback = current->input_callback;
                } else {
                    printf("[WARNING] Current view is NULL in input_processing_task\n");
                }

                xSemaphoreGive(dm.mutex);

                printf("[INFO] Input event type: %d, Current view: %s\n", event.type, view_name);

                if (input_callback) {
                    input_callback(&event);
                }
            }
        }
    }

    vTaskDelete(NULL);
}


void lvgl_tick_task(void *arg) {
    const TickType_t tick_interval = pdMS_TO_TICKS(10);

    while (1) 
    {
        lv_timer_handler();
        lv_tick_inc(LVGL_TASK_PERIOD_MS);
        vTaskDelay(tick_interval);
    }

    vTaskDelete(NULL);
}