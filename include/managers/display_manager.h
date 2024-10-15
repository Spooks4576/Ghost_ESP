#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "lvgl.h"
#include <stdbool.h>


typedef struct {
    lv_obj_t *root;        // Root object for the view
    void (*create)(void);  // Create function for the view
    void (*destroy)(void); // Destroy function for the view
} View;

/* Display Manager structure to store the current and previous views. */
typedef struct {
    View *current_view;   // Pointer to the active view
    View *previous_view;  // Pointer to the last view
} DisplayManager;

/* Function prototypes */

/**
 * @brief Initialize the Display Manager.
 */
void display_manager_init(void);

/**
 * @brief Register a new view.
 */
bool display_manager_register_view(View *view);

/**
 * @brief Switch to a new view.
 */
void display_manager_switch_view(View *view);

/**
 * @brief Destroy the current view.
 */
void display_manager_destroy_current_view(void);

/**
 * @brief Get the current active view.
 */
View *display_manager_get_current_view(void);


void lvgl_tick_task(void *arg);

void display_manager_fill_screen(lv_color_t color);

#endif /* DISPLAY_MANAGER_H */