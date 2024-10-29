#include "managers/views/music_visualizer.h"
#include "managers/views/main_menu_screen.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <lvgl.h>
#include <math.h> 

#define NUM_PARTICLES 0
#define ANIMATION_INTERVAL_MS 5  // Approximately 30 FPS

typedef struct {
    lv_obj_t *obj;
    int x;  // Current x position
    int y;  // Current y position
    int velocity;  // Horizontal velocity
} Particle;

typedef struct {
    int bars[NUM_BARS];  // Amplitude data for each bar
} AmplitudeData;

Particle particles[NUM_PARTICLES];
MusicVisualizerView view;
lv_obj_t *root;
QueueHandle_t amplitudeQueue;

int target_amplitudes[NUM_BARS] = {0};
int current_amplitudes[NUM_BARS] = {0};

void handle_hardware_input_music_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        display_manager_switch_view(&main_menu_view);
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            display_manager_switch_view(&main_menu_view);
        }
    }
}

void get_music_visualizer_callback(void **callback) {
    *callback = music_visualizer_view.input_callback;
}

View music_visualizer_view = {
    .root = NULL,
    .create = music_visualizer_view_create,
    .destroy = music_visualizer_destroy,
    .input_callback = handle_hardware_input_music_callback,
    .name = "Music Visualizer",
    .get_hardwareinput_callback = get_music_visualizer_callback
};

void animation_timer_callback(lv_timer_t *timer);

void music_visualizer_view_create() {
    display_manager_fill_screen(lv_color_black());

   root = lv_obj_create(lv_scr_act());
    music_visualizer_view.root = root;
    lv_obj_set_style_bg_color(music_visualizer_view.root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_size(music_visualizer_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(music_visualizer_view.root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(music_visualizer_view.root, LV_HOR_RES / 24, 0);
    lv_obj_set_style_bg_opa(music_visualizer_view.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(music_visualizer_view.root, 0, 0);
    lv_obj_set_style_pad_all(music_visualizer_view.root, 0, 0);
    lv_obj_set_style_radius(music_visualizer_view.root, 0, 0);

    const lv_font_t *track_label_font;
    const lv_font_t *artist_label_font;

    if (LV_HOR_RES <= 128) {
        track_label_font = &lv_font_montserrat_12;
        artist_label_font = &lv_font_montserrat_10;
    } else if (LV_HOR_RES <= 240) {
        track_label_font = &lv_font_montserrat_16;
        artist_label_font = &lv_font_montserrat_12;
    } else {
        track_label_font = &lv_font_montserrat_24;
        artist_label_font = &lv_font_montserrat_16;
    }

    int label_x_offset = LV_HOR_RES / 12;
    int label_y_offset = LV_VER_RES / 8;
    int bar_width = LV_HOR_RES / (NUM_BARS * 2);
    int bar_spacing = LV_HOR_RES / (NUM_BARS + 2);
    int bar_y_offset = LV_VER_RES / 4;

    view.track_label = lv_label_create(music_visualizer_view.root);
    lv_label_set_text(view.track_label, "Ghost ESP");
    lv_obj_set_style_text_font(view.track_label, track_label_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(view.track_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(view.track_label, LV_ALIGN_BOTTOM_LEFT, label_x_offset, -label_y_offset);

    view.artist_label = lv_label_create(music_visualizer_view.root);
    lv_label_set_text(view.artist_label, "Spooky");
    lv_obj_set_style_text_font(view.artist_label, artist_label_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(view.artist_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align_to(view.artist_label, view.track_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, lv_font_get_line_height(track_label_font) / 4);

    for (int i = 0; i < NUM_BARS; i++) {
        view.bars[i] = lv_obj_create(music_visualizer_view.root);
        lv_obj_set_size(view.bars[i], bar_width, 1);
        lv_obj_align(view.bars[i], LV_ALIGN_BOTTOM_LEFT, label_x_offset + (bar_spacing * i), -bar_y_offset);


        lv_obj_set_style_radius(view.bars[i], 0, LV_PART_MAIN); 
        lv_obj_set_style_bg_opa(view.bars[i], LV_OPA_COVER, LV_PART_MAIN); 
        lv_obj_set_style_bg_color(view.bars[i], lv_color_make(147, 112, 219), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(view.bars[i], lv_color_make(147, 112, 219), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(view.bars[i], LV_GRAD_DIR_VER, LV_PART_MAIN);
    }

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].obj = lv_obj_create(music_visualizer_view.root);
        lv_obj_set_size(particles[i].obj, 1, 1);
        lv_obj_set_style_radius(particles[i].obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(particles[i].obj, lv_color_white(), LV_PART_MAIN);
        particles[i].x = 0;
        particles[i].y = rand() % LV_VER_RES;
        particles[i].velocity = 1 + rand() % 3;
        lv_obj_align(particles[i].obj, LV_ALIGN_TOP_LEFT, particles[i].x, particles[i].y);
    }

    display_manager_add_status_bar("Rave Mode");

    amplitudeQueue = xQueueCreate(10, sizeof(AmplitudeData));
    lv_timer_create(animation_timer_callback, ANIMATION_INTERVAL_MS, NULL);
}

void animation_timer_callback(lv_timer_t *timer) {
    AmplitudeData amplitudeData;
    bool dataAvailable = xQueueReceive(amplitudeQueue, &amplitudeData, 0) == pdTRUE;

    if (dataAvailable) {
        for (int i = 0; i < NUM_BARS; i++) {
            lv_obj_set_height(view.bars[i], amplitudeData.bars[i]);
        }
    }

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].velocity;
        if (particles[i].x > LV_HOR_RES) {
            particles[i].x = 0;
            particles[i].y = rand() % LV_VER_RES;
            particles[i].velocity = 1 + rand() % 3;
        }
        lv_obj_set_pos(particles[i].obj, particles[i].x, particles[i].y);
    }
}

void music_visualizer_view_update(const uint8_t *amplitudes, const char *track_name, const char *artist_name) {

    if (music_visualizer_view.root) {
        if (strcmp(lv_label_get_text(view.track_label), track_name) != 0) {
            lv_label_set_text(view.track_label, track_name);
        }
        if (strcmp(lv_label_get_text(view.artist_label), artist_name) != 0) {
            lv_label_set_text(view.artist_label, artist_name);
        }

        AmplitudeData amplitudeData;
        for (int i = 0; i < NUM_BARS; i++) {
            amplitudeData.bars[i] = amplitudes[i];
        }
        xQueueSend(amplitudeQueue, &amplitudeData, portMAX_DELAY);
    }
}

void music_visualizer_destroy(void) {
    if (root) {
        lv_obj_del(root);
        root = NULL;
        music_visualizer_view.root = NULL;
    }
    if (amplitudeQueue) {
        vQueueDelete(amplitudeQueue);
        amplitudeQueue = NULL;
    }
}