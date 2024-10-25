#include "managers/views/flappy_ghost_screen.h"
#include "core/serial_manager.h"
#include "managers/views/main_menu_screen.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"

lv_obj_t *flappy_bird_canvas = NULL;
lv_obj_t *bird = NULL;
lv_obj_t *pipes[2] = {NULL, NULL};
lv_obj_t *score_label = NULL;

lv_timer_t *game_loop_timer = NULL;
int bird_y_position = 0;
float bird_velocity = 0;
float gravity = 2;
float flap_strength = -10;
int pipe_speed = 3;
int pipe_gap = 80;
int pipe_min_gap_y = 60;
int pipe_max_gap_y = 0;
int pipe_width = 20;
int score = 0;
bool is_game_over = false;

void draw_halloween_night_sky(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D0D40), 0);

    
    lv_obj_t *moon = lv_obj_create(parent);
    lv_obj_set_size(moon, 40, 40);
    lv_obj_set_style_bg_color(moon, lv_color_hex(0xFFFFDD), 0);
    lv_obj_set_style_radius(moon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(moon, LV_HOR_RES - 60, 30);

    
    for (int i = 0; i < 30; i++) {
        lv_obj_t *star = lv_obj_create(parent);
        lv_obj_set_size(star, 2, 2);
        lv_obj_set_style_bg_color(star, lv_color_hex(0xFFFFFF), 0);
        int x_pos = rand() % LV_HOR_RES;
        int y_pos = rand() % (LV_VER_RES / 2);
        lv_obj_set_pos(star, x_pos, y_pos);
    }

    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *twinkling_star = lv_obj_create(parent);
        lv_obj_set_size(twinkling_star, 4, 4);
        lv_obj_set_style_bg_color(twinkling_star, lv_color_hex(0xFFFF88), 0);
        int x_pos = rand() % LV_HOR_RES;
        int y_pos = rand() % (LV_VER_RES / 2);
        lv_obj_set_pos(twinkling_star, x_pos, y_pos);
    }

    
    lv_obj_t *ground = lv_obj_create(parent);
    lv_obj_set_size(ground, LV_HOR_RES, 40);
    lv_obj_set_style_bg_color(ground, lv_color_hex(0x101010), 0);
    lv_obj_set_pos(ground, 0, LV_VER_RES - 40);
}


void flappy_bird_view_create(void) {
    if (flappy_bird_view.root != NULL) {
        return;
    }

    bird_y_position = LV_VER_RES / 2;
    pipe_max_gap_y = LV_VER_RES - pipe_gap - 40;

    flappy_bird_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(flappy_bird_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(flappy_bird_view.root, lv_color_black(), 0);

    draw_halloween_night_sky(flappy_bird_view.root);

    
    flappy_bird_canvas = lv_canvas_create(flappy_bird_view.root);
    lv_obj_set_size(flappy_bird_canvas, LV_HOR_RES, LV_VER_RES);

    lv_obj_set_scrollbar_mode(flappy_bird_view.root, LV_SCROLLBAR_MODE_OFF);

    bird = lv_img_create(flappy_bird_view.root);
    lv_img_set_src(bird, &ghost);
    lv_obj_set_size(bird, 32, 32);
    lv_obj_set_pos(bird, LV_HOR_RES / 4, LV_VER_RES / 2);

    
    for (int i = 0; i < 2; i++) {
        pipes[i] = lv_obj_create(flappy_bird_canvas);
        lv_obj_set_size(pipes[i], pipe_width, LV_VER_RES);
        lv_obj_set_style_bg_color(pipes[i], lv_color_hex(0x00FF00), 0);
        int initial_gap_position = rand() % (pipe_max_gap_y - pipe_min_gap_y) + pipe_min_gap_y;
        lv_obj_set_pos(pipes[i], LV_HOR_RES + i * (LV_HOR_RES / 2), initial_gap_position);
    }

    
    score_label = lv_label_create(flappy_bird_view.root);
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(score_label, lv_color_white(), 0);

    
    display_manager_add_status_bar("Flappy Ghost");

    
    game_loop_timer = lv_timer_create(flappy_bird_game_loop, 20, NULL);
}

void flappy_bird_view_destroy(void) {
    if (flappy_bird_view.root != NULL) {
        is_game_over = false;
        bird_y_position = LV_VER_RES / 2;
        bird_velocity = 0;
        score = 0;
        if (game_loop_timer != NULL) {
            lv_timer_del(game_loop_timer);
            game_loop_timer = NULL;
        }

        lv_obj_del(flappy_bird_view.root);
        flappy_bird_view.root = NULL;
        flappy_bird_canvas = NULL;
        bird = NULL;
        pipes[0] = NULL;
        pipes[1] = NULL;
        score_label = NULL;
    }
}

void flappy_bird_view_hardwareinput_callback(InputEvent *event) {
    if (is_game_over) {
        if (event->type == INPUT_TYPE_JOYSTICK && event->data.joystick_index == 1) {
            flappy_bird_restart();
        }
        else if (event->type == INPUT_TYPE_JOYSTICK && event->data.joystick_index == 0)
        {
            display_manager_switch_view(&main_menu_view);
        }
        return;
    }

    if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;

        if (button == 1) {
            bird_velocity = flap_strength;
        }
    }
}

void flappy_bird_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)flappy_bird_view_hardwareinput_callback;
    }
}

void flappy_bird_game_loop(lv_timer_t *timer) {
    if (is_game_over) {
        return;
    }

    
    bird_velocity += gravity;
    bird_y_position += bird_velocity;
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    int angle = (int)(bird_velocity * 5); 
    if (angle > 45) angle = 45;
    if (angle < -45) angle = -45;

    
    lv_img_set_angle(bird, angle * 10);

  
    if (bird_y_position >= LV_VER_RES - 10 || bird_y_position <= 0) {
        flappy_bird_game_over();
    }

    
    for (int i = 0; i < 2; i++) {
        int pipe_x = lv_obj_get_x(pipes[i]);
        pipe_x -= pipe_speed;

       
        if (pipe_x < -pipe_width) {
            pipe_x = LV_HOR_RES;
            int initial_gap_position = rand() % (pipe_max_gap_y - pipe_min_gap_y) + pipe_min_gap_y;
            lv_obj_set_pos(pipes[i], LV_HOR_RES + i * (LV_HOR_RES / 2), initial_gap_position);
            score++;
            lv_label_set_text_fmt(score_label, "Score: %d", score);
        }

        lv_obj_set_x(pipes[i], pipe_x);

       
        if (flappy_bird_check_collision(bird, pipes[i])) {
            flappy_bird_game_over();
        }
    }
}

int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe) {
    int padding = 2;
    int bird_x = lv_obj_get_x(bird);
    int bird_y = lv_obj_get_y(bird);
    int bird_width = 10; 
    int bird_height = 10;

    int pipe_x = lv_obj_get_x(pipe);
    int pipe_gap_y = lv_obj_get_y(pipe);

    if (bird_x + bird_width > pipe_x - padding && bird_x < pipe_x + pipe_width + padding) {

        
        if (bird_y > pipe_gap_y + padding || bird_y + bird_height < pipe_gap_y - pipe_gap - padding) {
            return 1;
        }
    }

    return 0;
}

void flappy_bird_game_over() {
    is_game_over = true;

    
    lv_obj_t *game_over_container = lv_obj_create(flappy_bird_view.root);
    lv_obj_set_size(game_over_container, LV_HOR_RES - 40, 60);
    lv_obj_align(game_over_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(game_over_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(game_over_container, LV_OPA_70, 0);
    lv_obj_set_style_border_width(game_over_container, 2, 0);
    lv_obj_set_style_border_color(game_over_container, lv_color_hex(0xFFFFFF), 0);

    
    lv_obj_t *game_over_label = lv_label_create(game_over_container);
    lv_label_set_text(game_over_label, "Game Over!");
    lv_obj_set_style_text_color(game_over_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(game_over_label, LV_ALIGN_CENTER, 0, 0);
}

void flappy_bird_restart() {
    is_game_over = false;
    bird_y_position = LV_VER_RES / 2;
    bird_velocity = 0;
    score = 0;
    lv_label_set_text_fmt(score_label, "Score: %d", score);

    
    for (int i = 0; i < 2; i++) {
        lv_obj_set_pos(pipes[i], LV_HOR_RES + i * (LV_HOR_RES / 2), rand() % (pipe_max_gap_y - pipe_min_gap_y) + pipe_min_gap_y);
    }

    
    lv_obj_t *game_over_label = lv_obj_get_child(flappy_bird_view.root, -1);
    if (game_over_label) {
        lv_obj_del(game_over_label);
    }
}

View flappy_bird_view = {
    .root = NULL,
    .create = flappy_bird_view_create,
    .destroy = flappy_bird_view_destroy,
    .input_callback = flappy_bird_view_hardwareinput_callback,
    .name = "FlappyBirdView",
    .get_hardwareinput_callback = flappy_bird_view_get_hardwareinput_callback
};