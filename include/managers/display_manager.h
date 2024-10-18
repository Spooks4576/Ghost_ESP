#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "lvgl.h"
#include <stdbool.h>
#include "managers/joystick_manager.h"


typedef void* SemaphoreHandle_tt; // Because Circular Includes are fun :)

typedef struct {
    lv_obj_t *root;
    void (*create)(void);  
    void (*destroy)(void);
    void (*hardwareinput_callback)(int);
    const char* name;
} View;


typedef struct {
    View *current_view; 
    View *previous_view; 
    SemaphoreHandle_tt mutex;
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

// Status Bar Functions

void update_status_bar(bool wifi_enabled, bool bt_enabled, bool sd_card_mounted, int batteryPercentage);

void display_manager_add_status_bar(const char* CurrentMenuName);


LV_IMG_DECLARE(Ghost_ESP);
LV_IMG_DECLARE(Map);
LV_IMG_DECLARE(bluetooth);
LV_IMG_DECLARE(Settings);
LV_IMG_DECLARE(wifi);

joystick_t joysticks[5];


#endif /* DISPLAY_MANAGER_H */