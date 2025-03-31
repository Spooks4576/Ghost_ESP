#ifndef NUMBER_PAD_SCREEN_H
#define NUMBER_PAD_SCREEN_H

#include "managers/display_manager.h"

extern View number_pad_view;

typedef enum {
    NP_MODE_AP,
    NP_MODE_LAN
} ENumberPadMode;

void set_number_pad_mode(ENumberPadMode mode);

#endif // NUMBER_PAD_SCREEN_H