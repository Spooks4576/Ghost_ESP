#include "managers/views/splash_screen.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/music_visualizer.h"
#include <stdio.h>

lv_obj_t *splash_screen;
lv_obj_t *img;


static void zoom_anim_cb(void *var, int32_t zoom);
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
    
    lv_anim_t zoom_anim;
    lv_anim_init(&zoom_anim);

    
    lv_disp_t * disp = lv_disp_get_default();
    int hor_res = lv_disp_get_hor_res(disp);
    int ver_res = lv_disp_get_ver_res(disp);


    lv_anim_set_var(&zoom_anim, img);
    lv_anim_set_values(&zoom_anim, hor_res / 2, hor_res);
    lv_anim_set_time(&zoom_anim, 100);
    lv_anim_set_playback_time(&zoom_anim, 100);
    lv_anim_set_repeat_count(&zoom_anim, 1);
    lv_anim_set_exec_cb(&zoom_anim, zoom_anim_cb);
    lv_anim_set_ready_cb(&zoom_anim, fade_out_cb);
    lv_anim_start(&zoom_anim);
}


static void zoom_anim_cb(void *var, int32_t zoom) {
    lv_img_set_zoom((lv_obj_t *)var, zoom);
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