#include "managers/views/splash_screen.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/music_visualizer.h"
#include <stdio.h>

lv_obj_t *splash_screen;
lv_obj_t *img;


static void fade_anim_cb(void *var, int32_t opacity);
static void fade_out_cb(void *var);


void splash_create(void) {

    display_manager_fill_screen(lv_color_black());
    
    splash_screen = lv_obj_create(lv_scr_act());
    splash_view.root = splash_screen;
    lv_obj_set_size(splash_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(splash_screen, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);


    img = lv_img_create(splash_screen);
    lv_img_set_src(img, &Ghost_ESP);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    
    if (LV_VER_RES < 140)
    {
        lv_img_set_zoom(img, 128);
    }

   
    
    lv_anim_t fade_anim;
    lv_anim_init(&fade_anim);

    lv_anim_set_var(&fade_anim, img);
    lv_anim_set_values(&fade_anim, LV_OPA_0, LV_OPA_100);
    lv_anim_set_time(&fade_anim, 100);
    lv_anim_set_playback_delay(&fade_anim, 0);
    lv_anim_set_playback_time(&fade_anim, 100);
    lv_anim_set_repeat_count(&fade_anim, 2);
    lv_anim_set_exec_cb(&fade_anim, fade_anim_cb);
    lv_anim_set_ready_cb(&fade_anim, fade_out_cb);
    lv_anim_start(&fade_anim);
}


static void fade_anim_cb(void *var, int32_t opacity) {
    lv_obj_set_style_img_opa((lv_obj_t *)var, opacity, LV_PART_MAIN);
}


static void fade_out_cb(void *var) {
    display_manager_switch_view(&main_menu_view);
}


void splash_destroy(void) {
    if (splash_screen) {
        lv_obj_del(splash_screen);
        splash_screen = NULL;
    }
}


View splash_view = {
    .root = NULL,
    .create = splash_create,
    .destroy = splash_destroy,
    .input_callback = NULL,
    .name = "Splash Screen"
};