#include "managers/views/music_visualizer.h"



MusicVisualizerView view;


View music_visualizer_view = {
    .root = NULL,
    .create = music_visualizer_view_create,
    .destroy = music_visualizer_destroy
};


void music_visualizer_view_create() {
    view.screen = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(view.screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_size(view.screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(view.screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(view.screen, 10, 0);
    lv_obj_set_style_bg_opa(view.screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view.screen, 0, 0);
    lv_obj_set_style_pad_all(view.screen, 0, 0);
    lv_obj_set_style_radius(view.screen, 0, 0);

    
    view.track_label = lv_label_create(view.screen);
    lv_label_set_text(view.track_label, "Ghost ESP");
    lv_obj_set_style_text_font(view.track_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(view.track_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(view.track_label, LV_ALIGN_BOTTOM_LEFT, 20, -30);

    
    view.artist_label = lv_label_create(view.screen);
    lv_label_set_text(view.artist_label, "Spooky");
    lv_obj_set_style_text_font(view.artist_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(view.artist_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align_to(view.artist_label, view.track_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);


    for (int i = 0; i < NUM_BARS; i++) {
        view.bars[i] = lv_obj_create(view.screen);
        lv_obj_set_size(view.bars[i], 10, 1);
        lv_obj_align(view.bars[i], LV_ALIGN_BOTTOM_LEFT, 10 + (15 * i), -100);

        
        lv_obj_set_style_radius(view.bars[i], 0, LV_PART_MAIN); 
        lv_obj_set_style_bg_opa(view.bars[i], LV_OPA_COVER, LV_PART_MAIN); 
        lv_obj_set_style_bg_color(view.bars[i], lv_color_make(255, 215, 0), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(view.bars[i], lv_color_make(255, 255, 0), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(view.bars[i], LV_GRAD_DIR_VER, LV_PART_MAIN);
    }
}

void music_visualizer_view_update(const uint8_t *amplitudes, const char *track_name, const char *artist_name) {
    if (strcmp(lv_label_get_text(view.track_label), track_name) != 0) {
        lv_label_set_text(view.track_label, track_name);
    }
    if (strcmp(lv_label_get_text(view.artist_label), artist_name) != 0) {
        lv_label_set_text(view.artist_label, artist_name);
    }

    for (int i = 0; i < NUM_BARS; i++) {
        int new_height = amplitudes[i];
        
        lv_obj_set_height(view.bars[i], new_height);

        
        lv_obj_align(view.bars[i], LV_ALIGN_BOTTOM_LEFT, 10 + (15 * i), -100);
        lv_obj_set_height(view.bars[0], 0); // Attempt To 0 out deadzone
    }
}



void music_visualizer_destroy(void) {
    if (view.screen) {
        lv_obj_del(view.screen);
        view.screen = NULL;
    }
}