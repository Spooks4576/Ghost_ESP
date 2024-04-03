#include "lvgl.h"

static void set_zoom(void* img, int32_t v)
{
    lv_img_set_zoom((lv_obj_t*)img, v);
}

void animate_image_scale(lv_obj_t* Obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, Obj);
    lv_anim_set_exec_cb(&a, set_zoom);

    
    lv_anim_set_values(&a, 128, 256); 

    lv_anim_set_time(&a, 2000);
    lv_anim_set_playback_time(&a, 2000);

    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    lv_anim_start(&a);
}