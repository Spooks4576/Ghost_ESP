// File: board_config.h

#pragma once

#if defined(ESP32_DEV_MODULE)
    #include "config/esp32_dev_module.h"
#elif defined(ESP32_S2_DEV_MODULE)
    #include "config/esp32_s2_dev_module.h"
#elif defined(ESP32_S3_DEV_MODULE)
    #include "config/esp32_s3_dev_module.h"
#elif defined(ESP32_C6_DEV_MODULE)
    #include "config/esp32_c6_dev_module.h"
#elif defined(ESP32_C3_DEV_MODULE)
    #include "config/esp32_c3_dev_module.h"
#elif defined(DEV_BOARD_PRO)
    #include "config/marauder_dev_board_pro.h"
#elif defined(MINION_BOARD)
    #include "config/rabbit_labs_minion.h"    
#elif defined(rek5rek5_BOARD)
    #include "config/rek5rek5_board_module.h"
#elif defined(NUGGET_BOARD)
    #include "config/esp32_s2_nugget.h"
#else
    #error "Board type is not defined or unsupported!"
#endif