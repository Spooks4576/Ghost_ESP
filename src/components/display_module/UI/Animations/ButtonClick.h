#include "lvgl.h"


static void set_button_opacity(void *btn, int32_t v) 
{
    lv_obj_set_style_opa((lv_obj_t *)btn, v, 0);
}


static void button_anim_ready_cb(lv_anim_t *a) {
    lv_obj_t *btn = (lv_obj_t *)a->var;
    lv_obj_set_style_opa(btn, LV_OPA_COVER, 0);  
    lv_anim_del(a, NULL);
}


lv_anim_t *animate_button_click(lv_obj_t *btn) 
{
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);

    lv_anim_set_exec_cb(&a, set_button_opacity);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_50); 
    lv_anim_set_values(&a, LV_OPA_50, LV_OPA_COVER); 

    lv_anim_set_time(&a, 200); 
    lv_anim_set_ready_cb(&a, button_anim_ready_cb);

    lv_anim_start(&a);
    return &a;
}