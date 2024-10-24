cmake_minimum_required(VERSION 3.16.0)
set(IDF_TARGET "esp32")
set(PARTITION_TABLE_CUSTOM_FILENAME "partitions/min_spiffs.csv")
set(PARTITION_TABLE_CUSTOM_PARTITION_FILENAME "partitions/min_spiffs.csv")
add_link_options("-Wl,-z,muldefs")
#add_compile_definitions(LED_DATA_PIN=10)
#add_compile_definitions(USB_MODULE=1)
#add_compile_definitions(DEBUG=1)
add_compile_definitions(USE_TOUCHSCREEN)
add_compile_definitions(HOLD_LIMIT=1000) # for joystick
#add_compile_definitions(USE_JOYSTICK)
#add_compile_definitions(USE_CARDPUTER)
add_compile_definitions(WITH_SCREEN=1)
add_compile_definitions(TFT_WIDTH=240) # TFT Width TFT_WIDTH=240
add_compile_definitions(TFT_HEIGHT=320) # TFT Height TFT_WIDTH=320
add_compile_definitions(LED_ORDER=0) # 2 For RGB 0 for GRB
add_compile_definitions(NUM_LEDS=1)
add_compile_definitions(USING_SPI=1)
add_compile_definitions(USING_MMC=0)
add_compile_definitions(DNS_SERVER_MAX_ITEMS=1)
add_compile_definitions(MAX_WPS_NETWORKS=15)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Ghost_ESP_IDF)
