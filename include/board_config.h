// File: board_config.h

#pragma once

#if defined(ESP32_DEV_MODULE)
    #include "config/esp32_dev_module.h"
#elif defined(flipper_dev_board)
    #include "config/flipper_dev_board.h"
#elif defined(DEV_BOARD_PRO)
    #include "config/marauder_dev_board_pro.h"
#elif defined(MINION_BOARD)
    #include "config/rabbit_labs_minion.h"
#elif defined(NUGGET_BOARD)
    #include "config/esp32_s2_nugget.h"
#else
    #error "Board type is not defined or unsupported!"
#endif