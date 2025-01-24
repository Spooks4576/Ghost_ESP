#include "managers/joystick_manager.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void joystick_init(joystick_t *joystick, int pin, uint32_t hold_lim,
                   bool pullup) {
  joystick->pin = pin;
  joystick->pullup = pullup;
  joystick->pressed = false;
  joystick->hold_lim = hold_lim;
  joystick->cur_hold = 0;
  joystick->isheld = false;
  joystick->hold_init = 0;

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
      .pull_down_en = pullup ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
      .intr_type = GPIO_INTR_DISABLE};

  gpio_config(&io_conf);
}

bool joystick_is_held(joystick_t *joystick) { return joystick->isheld; }

bool joystick_get_button_state(joystick_t *joystick) {
  int button_state = gpio_get_level(joystick->pin);

  if ((joystick->pullup && button_state == 0) ||
      (!joystick->pullup && button_state == 1)) {
    return true;
  }
  return false;
}

bool joystick_just_pressed(joystick_t *joystick) {
  bool btn_state = joystick_get_button_state(joystick);

  if (btn_state && !joystick->pressed) {
    joystick->hold_init =
        esp_timer_get_time() / 1000; // Get time in milliseconds
    joystick->pressed = true;
    return true;
  } else if (btn_state) {
    uint32_t elapsed = (esp_timer_get_time() / 1000) - joystick->hold_init;
    if (elapsed < joystick->hold_lim) {
      joystick->isheld = false;
    } else {
      joystick->isheld = true;
    }
    return false;
  } else {
    joystick->pressed = false;
    joystick->isheld = false;
    return false;
  }
}

bool joystick_just_released(joystick_t *joystick) {
  bool btn_state = joystick_get_button_state(joystick);

  if (!btn_state && joystick->pressed) {
    joystick->isheld = false;
    joystick->pressed = false;
    return true;
  } else {
    return false;
  }
}