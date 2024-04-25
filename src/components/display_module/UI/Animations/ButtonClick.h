#include "lvgl.h"

static void set_button_size(void *btn, int32_t v) 
{
    lv_obj_set_size((lv_obj_t *)btn, v, v);
}

static void button_anim_ready_cb(lv_anim_t *a) {
    lv_anim_del(a, NULL);
}

lv_anim_t *animate_button_click(lv_obj_t *btn) 
{
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);

    // Key Change: Set pivot point to center
    lv_obj_set_style_transform_pivot_x(btn, lv_obj_get_width(btn) / 2, 0);
    lv_obj_set_style_transform_pivot_y(btn, lv_obj_get_height(btn) / 2, 0); 

    lv_anim_set_exec_cb(&a, set_button_size);

    // Adjust Values for Centered Shrinking
    lv_anim_set_values(&a, lv_obj_get_width(btn), lv_obj_get_width(btn) - 20);
    lv_anim_set_values(&a, lv_obj_get_width(btn) - 20, lv_obj_get_width(btn));

    lv_anim_set_time(&a, 200); 
    lv_anim_set_ready_cb(&a, button_anim_ready_cb);

    lv_anim_start(&a);
    return &a;
}