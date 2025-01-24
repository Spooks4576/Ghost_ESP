#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "lvgl.h"
#include "managers/display_manager.h"

/**
 * @brief Creates the splash screen view.
 */
void splash_create(void);

/**
 * @brief Destroys the splash screen view.
 */
void splash_destroy(void);

/**
 * @brief Splash screen view object.
 */
extern View splash_view;

#endif /* SPLASH_SCREEN_H */