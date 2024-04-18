#include "lvgl.h"

static void set_zoom(void* img, int32_t v)
{
    lv_img_set_zoom((lv_obj_t*)img, v);
}

static void anim_ready_cb(lv_anim_t * a) {
}

lv_anim_t* animate_image_scale(lv_obj_t* obj) {
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, set_zoom);
    lv_anim_set_values(&a, 256, 128); 
    lv_anim_set_time(&a, 1000); 
    lv_anim_set_playback_time(&a, 1000); 
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_ready_cb(&a, anim_ready_cb);

    lv_anim_start(&a);
    return &a;
}