#include "managers/views/splash_screen.h"
#include <stdio.h>

static lv_obj_t *splash_screen;
static lv_obj_t *img;
LV_IMG_DECLARE(Spooky_Splash);


static void zoom_anim_cb(void *var, int32_t zoom);
static void fade_out_cb(void *var);
static void transition_to_next_screen(lv_anim_t *a);


void splash_create(void) {

    lv_obj_t *black_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(black_screen, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(black_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(black_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(black_screen, 0, LV_PART_MAIN);
    lv_obj_set_size(black_screen, LV_HOR_RES, LV_VER_RES);

   
    splash_screen = lv_obj_create(black_screen);
    lv_obj_set_size(splash_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(splash_screen, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);


    img = lv_img_create(splash_screen);
    lv_img_set_src(img, &Spooky_Splash);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    lv_anim_t zoom_anim;
    lv_anim_init(&zoom_anim);
    lv_anim_set_var(&zoom_anim, img);
    lv_anim_set_values(&zoom_anim, 256, 512);
    lv_anim_set_time(&zoom_anim, 500);
    lv_anim_set_playback_time(&zoom_anim, 500);
    lv_anim_set_repeat_count(&zoom_anim, 1);
    lv_anim_set_exec_cb(&zoom_anim, zoom_anim_cb);
    lv_anim_set_ready_cb(&zoom_anim, fade_out_cb);
    lv_anim_start(&zoom_anim);

    
     lv_obj_t *label = lv_label_create(splash_screen);
    lv_label_set_text(label, "Ghost\nESP");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 80);
    printf("Created Splash Screen\n");
}


static void zoom_anim_cb(void *var, int32_t zoom) {
    lv_img_set_zoom((lv_obj_t *)var, zoom);
}


static void fade_out_cb(void *var) {
    lv_anim_t fade_anim;
    lv_anim_init(&fade_anim);
    lv_anim_set_var(&fade_anim, splash_screen);
    lv_anim_set_values(&fade_anim, 255, 0);
    lv_anim_set_time(&fade_anim, 500);
    lv_anim_set_exec_cb(&fade_anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_ready_cb(&fade_anim, transition_to_next_screen);
    lv_anim_start(&fade_anim);
}


static void transition_to_next_screen(lv_anim_t *a) {
    printf("Transitioning to next screen...\n");
    splash_destroy();
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
    .destroy = splash_destroy
};