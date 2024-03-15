#pragma once

#include "board_config.h"

#if DISPLAY_SUPPORT



class DisplayModule {
public:
    void init();
    void update();
    // Other display-related methods
};

#endif