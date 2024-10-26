#include "vendor/keyboard_handler.h"
#include <stdlib.h>
#include <string.h>

#define digitalWrite(pin, level) gpio_set_level((gpio_num_t)pin, level)
#define digitalRead(pin)         gpio_get_level((gpio_num_t)pin)

static const int output_list[] = {8, 9, 11};
static const int input_list[] = {13, 15, 3, 4, 5, 6, 7};

// Forward declarations
static void keys_state_reset(KeysState_t* keys_state);
static uint8_t get_key_value(const Point2D_t* keyCoor, bool shift, bool ctrl, bool is_caps_locked);

void keyboard_init(Keyboard_t* keyboard) {
    keyboard->key_list_buffer = NULL;
    keyboard->key_list_buffer_len = 0;
    keyboard->key_pos_print_keys = NULL;
    keyboard->key_pos_print_keys_len = 0;
    keyboard->key_pos_hid_keys = NULL;
    keyboard->key_pos_hid_keys_len = 0;
    keyboard->key_pos_modifier_keys = NULL;
    keyboard->key_pos_modifier_keys_len = 0;
    keyboard->keys_state_buffer.word = NULL;
    keyboard->keys_state_buffer.word_len = 0;
    keyboard->keys_state_buffer.hid_keys = NULL;
    keyboard->keys_state_buffer.hid_keys_len = 0;
    keyboard->keys_state_buffer.modifier_keys = NULL;
    keyboard->keys_state_buffer.modifier_keys_len = 0;
    keyboard->keys_state_buffer.reset = keys_state_reset;
    keyboard->is_caps_locked = false;
    keyboard->last_key_size = 0;
}

void keyboard_begin(Keyboard_t* keyboard) {
    for (size_t i = 0; i < sizeof(output_list) / sizeof(output_list[0]); ++i) {
        gpio_reset_pin((gpio_num_t)output_list[i]);
        gpio_set_direction((gpio_num_t)output_list[i], GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)output_list[i], GPIO_PULLUP_PULLDOWN);
        digitalWrite(output_list[i], 0);
    }

    for (size_t i = 0; i < sizeof(input_list) / sizeof(input_list[0]); ++i) {
        gpio_reset_pin((gpio_num_t)input_list[i]);
        gpio_set_direction((gpio_num_t)input_list[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)input_list[i], GPIO_PULLUP_ONLY);
    }

    keyboard_set_output(output_list, sizeof(output_list) / sizeof(output_list[0]), 0);
}

void keyboard_set_output(const int* pinList, size_t pinCount, uint8_t output) {
    output &= 0x07;
    for (size_t i = 0; i < pinCount; ++i) {
        digitalWrite(pinList[i], (output >> i) & 0x01);
    }
}

uint8_t keyboard_get_input(const int* pinList, size_t pinCount) {
    uint8_t buffer = 0x00;
    uint8_t pin_value = 0x00;

    for (size_t i = 0; i < pinCount; ++i) {
        pin_value = (digitalRead(pinList[i]) == 1) ? 0x00 : 0x01;
        pin_value <<= i;
        buffer |= pin_value;
    }

    return buffer;
}

uint8_t keyboard_get_key(const Keyboard_t* keyboard, Point2D_t keyCoor) {
    if (keyCoor.x < 0 || keyCoor.y < 0) {
        return 0;
    }
    return get_key_value(&keyCoor, keyboard->keys_state_buffer.shift,
                         keyboard->keys_state_buffer.ctrl,
                         keyboard->is_caps_locked);
}

void keyboard_update_key_list(Keyboard_t* keyboard) {
    // Clear current key list
    free(keyboard->key_list_buffer);
    keyboard->key_list_buffer = NULL;
    keyboard->key_list_buffer_len = 0;

    Point2D_t coor;
    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++) {
        keyboard_set_output(output_list, sizeof(output_list) / sizeof(output_list[0]), i);
        input_value = keyboard_get_input(input_list, sizeof(input_list) / sizeof(input_list[0]));

        if (input_value) {
            for (int j = 0; j < 7; j++) {
                if (input_value & (1 << j)) {
                    coor.x = (i > 3) ? j + 1 : j;
                    coor.y = (i > 3) ? (i - 4) : i;

                    coor.y = -coor.y + 3;  // Adjust Y coordinate to match picture

                    keyboard->key_list_buffer_len++;
                    keyboard->key_list_buffer = realloc(keyboard->key_list_buffer,
                        keyboard->key_list_buffer_len * sizeof(Point2D_t));
                    keyboard->key_list_buffer[keyboard->key_list_buffer_len - 1] = coor;
                }
            }
        }
    }
}

uint8_t keyboard_is_pressed(const Keyboard_t* keyboard) {
    return keyboard->key_list_buffer_len;
}

bool keyboard_is_change(Keyboard_t* keyboard) {
    if (keyboard->last_key_size != keyboard->key_list_buffer_len) {
        keyboard->last_key_size = keyboard->key_list_buffer_len;
        return true;
    }
    return false;
}

bool keyboard_is_key_pressed(const Keyboard_t* keyboard, char c) {
    for (size_t i = 0; i < keyboard->key_list_buffer_len; ++i) {
        if (keyboard_get_key(keyboard, keyboard->key_list_buffer[i]) == c) {
            return true;
        }
    }
    return false;
}

void keyboard_update_keys_state(Keyboard_t* keyboard) {
    keys_state_reset(&keyboard->keys_state_buffer);

    for (size_t i = 0; i < keyboard->key_list_buffer_len; ++i) {
        Point2D_t key_pos = keyboard->key_list_buffer[i];
        uint8_t key_value = keyboard_get_key(keyboard, key_pos);

        switch (key_value) {
            case KEY_FN:
                keyboard->keys_state_buffer.fn = true;
                break;
            case KEY_LEFT_CTRL:
                keyboard->keys_state_buffer.ctrl = true;
                keyboard->keys_state_buffer.modifier_keys_len++;
                keyboard->keys_state_buffer.modifier_keys = realloc(
                    keyboard->keys_state_buffer.modifier_keys,
                    keyboard->keys_state_buffer.modifier_keys_len * sizeof(uint8_t));
                keyboard->keys_state_buffer.modifier_keys[keyboard->keys_state_buffer.modifier_keys_len - 1] = KEY_LEFT_CTRL;
                break;
            case KEY_LEFT_SHIFT:
                keyboard->keys_state_buffer.shift = true;
                keyboard->keys_state_buffer.modifier_keys_len++;
                keyboard->keys_state_buffer.modifier_keys = realloc(
                    keyboard->keys_state_buffer.modifier_keys,
                    keyboard->keys_state_buffer.modifier_keys_len * sizeof(uint8_t));
                keyboard->keys_state_buffer.modifier_keys[keyboard->keys_state_buffer.modifier_keys_len - 1] = KEY_LEFT_SHIFT;
                break;
            case KEY_LEFT_ALT:
                keyboard->keys_state_buffer.alt = true;
                keyboard->keys_state_buffer.modifier_keys_len++;
                keyboard->keys_state_buffer.modifier_keys = realloc(
                    keyboard->keys_state_buffer.modifier_keys,
                    keyboard->keys_state_buffer.modifier_keys_len * sizeof(uint8_t));
                keyboard->keys_state_buffer.modifier_keys[keyboard->keys_state_buffer.modifier_keys_len - 1] = KEY_LEFT_ALT;
                break;
            case KEY_TAB:
                keyboard->keys_state_buffer.tab = true;
                break;
            case KEY_BACKSPACE:
                keyboard->keys_state_buffer.del = true;
                break;
            case KEY_ENTER:
                keyboard->keys_state_buffer.enter = true;
                break;
            case ' ':
                keyboard->keys_state_buffer.space = true;
                break;
            default:
                keyboard->keys_state_buffer.word_len++;
                keyboard->keys_state_buffer.word = realloc(
                    keyboard->keys_state_buffer.word,
                    keyboard->keys_state_buffer.word_len * sizeof(char));
                
                char character = (keyboard->keys_state_buffer.shift || keyboard->is_caps_locked)
                                     ? get_key_value(&key_pos, true, keyboard->keys_state_buffer.ctrl, keyboard->is_caps_locked)
                                     : get_key_value(&key_pos, false, keyboard->keys_state_buffer.ctrl, keyboard->is_caps_locked);
                
                keyboard->keys_state_buffer.word[keyboard->keys_state_buffer.word_len - 1] = character;
                break;
        }
    }


    for (size_t i = 0; i < keyboard->keys_state_buffer.modifier_keys_len; ++i) {
        keyboard->keys_state_buffer.modifiers |= (1 << (keyboard->keys_state_buffer.modifier_keys[i] - 0x80));
    }
}

// Helper function to reset the state
static void keys_state_reset(KeysState_t* keys_state) {
    keys_state->tab = false;
    keys_state->fn = false;
    keys_state->shift = false;
    keys_state->ctrl = false;
    keys_state->opt = false;
    keys_state->alt = false;
    keys_state->del = false;
    keys_state->enter = false;
    keys_state->space = false;
    keys_state->modifiers = 0;
    free(keys_state->word);
    free(keys_state->hid_keys);
    free(keys_state->modifier_keys);
    keys_state->word = NULL;
    keys_state->hid_keys = NULL;
    keys_state->modifier_keys = NULL;
    keys_state->word_len = 0;
    keys_state->hid_keys_len = 0;
    keys_state->modifier_keys_len = 0;
}

static uint8_t get_key_value(const Point2D_t* keyCoor, bool shift, bool ctrl, bool is_caps_locked) {
    uint8_t base_value = _kb_asciimap[keyCoor->x + keyCoor->y * 14];

    
    if (shift || (is_caps_locked && (base_value >= 'a' && base_value <= 'z'))) {
        base_value |= SHIFT;
    }


    return base_value & ~SHIFT;
}