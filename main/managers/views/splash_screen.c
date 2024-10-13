#include "managers/views/splash_screen.h"

static lv_obj_t *splash_screen;


void splash_create(void) {
    splash_screen = lv_obj_create(NULL);

    
    lv_obj_t *label = lv_label_create(splash_screen);
    lv_label_set_text(label, "Ghost \n ESP");
    lv_obj_set_style_text_font(label, &ft13, LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
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