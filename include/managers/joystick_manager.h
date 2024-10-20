#ifndef JOYSTICK_MANAGER_H
#define JOYSTICK_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

// Define a struct for joystick management
typedef struct {
    int pin;
    bool pullup;
    bool pressed;
    bool isheld;
    uint32_t hold_lim;
    uint32_t cur_hold;
    uint32_t hold_init;
} joystick_t;

/**
 * @brief Initializes a joystick object.
 * 
 * @param joystick Pointer to the joystick structure.
 * @param pin GPIO pin used for the joystick.
 * @param hold_lim Time in milliseconds to consider the button as held.
 * @param pullup True if pull-up resistor is used, false if pull-down resistor is used.
 */
void joystick_init(joystick_t *joystick, int pin, uint32_t hold_lim, bool pullup);

/**
 * @brief Checks if the joystick is being held.
 * 
 * @param joystick Pointer to the joystick structure.
 * @return True if held, false otherwise.
 */
bool joystick_is_held(joystick_t *joystick);

/**
 * @brief Checks if the button is currently pressed.
 * 
 * @param joystick Pointer to the joystick structure.
 * @return True if the button is pressed, false otherwise.
 */
bool joystick_get_button_state(joystick_t *joystick);

/**
 * @brief Checks if the button was just pressed.
 * 
 * @param joystick Pointer to the joystick structure.
 * @return True if the button was just pressed, false otherwise.
 */
bool joystick_just_pressed(joystick_t *joystick);

/**
 * @brief Checks if the button was just released.
 * 
 * @param joystick Pointer to the joystick structure.
 * @return True if the button was just released, false otherwise.
 */
bool joystick_just_released(joystick_t *joystick);

#endif // JOYSTICK_MANAGER_H