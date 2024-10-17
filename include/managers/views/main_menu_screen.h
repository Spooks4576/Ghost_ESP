#ifndef MAIN_MENU_SCREEN_H
#define MAIN_MENU_SCREEN_H

#include "managers/display_manager.h"
#include "lvgl.h"

/**
 * @brief Creates the main menu screen view.
 */
void main_menu_create(void);

/**
 * @brief Destroys the main menu screen view.
 */
void main_menu_destroy(void);

/**
 * @brief Main menu screen view object.
 */

static void menu_item_event_handler(lv_event_t *e);


extern View main_menu_view;

#endif /* MAIN_MENU_SCREEN_H */