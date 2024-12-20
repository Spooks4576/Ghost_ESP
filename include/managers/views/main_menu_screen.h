#ifndef MAIN_MENU_SCREEN_H
#define MAIN_MENU_SCREEN_H

#include "managers/display_manager.h"
#include "managers/views/options_screen.h"
#include "lvgl.h"

/**
 * @brief Creates the main menu screen view.
 */
void main_menu_create(void);

/**
 * @brief Destroys the main menu screen view.
 */
void main_menu_destroy(void);


static void select_menu_item(int index);

static void handle_menu_item_selection(int item_index);

void handle_hardware_button_press(int ButtonPressed);


extern View main_menu_view;

extern lv_timer_t *time_update_timer;

#endif /* MAIN_MENU_SCREEN_H */