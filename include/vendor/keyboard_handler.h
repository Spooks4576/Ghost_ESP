/**
 * @file keyboard.h
 * @brief Keyboard handling for ESP-IDF in C.
 * @version 0.1
 * @date 2023-09-22
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "m5_keyboard_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x;
    int y;
} Point2D_t;

typedef struct {
    bool tab;
    bool fn;
    bool shift;
    bool ctrl;
    bool opt;
    bool alt;
    bool del;
    bool enter;
    bool space;
    uint8_t modifiers;

    char* word;
    size_t word_len;
    uint8_t* hid_keys;
    size_t hid_keys_len;
    uint8_t* modifier_keys;
    size_t modifier_keys_len;

    void (*reset)(struct KeysState_t* self);
} KeysState_t;

typedef struct {
    Point2D_t* key_list_buffer;
    size_t key_list_buffer_len;
    Point2D_t* key_pos_print_keys;
    size_t key_pos_print_keys_len;
    Point2D_t* key_pos_hid_keys;
    size_t key_pos_hid_keys_len;
    Point2D_t* key_pos_modifier_keys;
    size_t key_pos_modifier_keys_len;
    KeysState_t keys_state_buffer;
    bool is_caps_locked;
    uint8_t last_key_size;
} Keyboard_t;

// Function declarations
void keyboard_init(Keyboard_t* keyboard);
void keyboard_begin(Keyboard_t* keyboard);
void keyboard_set_output(const int* pinList, size_t pinCount, uint8_t output);
uint8_t keyboard_get_input(const int* pinList, size_t pinCount);
uint8_t keyboard_get_key(const Keyboard_t* keyboard, Point2D_t keyCoor);
void keyboard_update_key_list(Keyboard_t* keyboard);
uint8_t keyboard_is_pressed(const Keyboard_t* keyboard);
bool keyboard_is_change(Keyboard_t* keyboard);
bool keyboard_is_key_pressed(const Keyboard_t* keyboard, char c);
void keyboard_update_keys_state(Keyboard_t* keyboard);

#ifdef __cplusplus
}
#endif