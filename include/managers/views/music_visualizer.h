#include "lvgl.h"
#include <string.h>
#include <stdio.h>

#include "managers/display_manager.h"

#define NUM_BARS 15

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *track_label;
    lv_obj_t *artist_label;
    lv_obj_t *bars[NUM_BARS];
} MusicVisualizerView;


void music_visualizer_view_create();

void music_visualizer_view_update(const uint8_t *amplitudes, const char *track_name, const char *artist_name);

void music_visualizer_destroy();

extern View music_visualizer_view;