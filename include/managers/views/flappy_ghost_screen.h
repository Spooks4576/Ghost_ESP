#ifndef FLAPPY_BIRD_SCREEN_H
#define FLAPPY_BIRD_SCREEN_H

#include "managers/display_manager.h"

void flappy_bird_view_create(void);
void flappy_bird_view_destroy(void);
void flappy_bird_view_hardwareinput_callback(InputEvent *event);
void flappy_bird_view_get_hardwareinput_callback(void **callback);
void flappy_bird_game_loop(lv_timer_t *timer);
int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe);
void flappy_bird_game_over();
void flappy_bird_restart();


extern View flappy_bird_view;

#endif // FLAPPY_BIRD_SCREEN_H