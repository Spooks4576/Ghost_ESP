cmake_minimum_required(VERSION 3.16.0)

# Set IDF_TARGET from environment variable if defined
if(DEFINED ENV{IDF_TARGET})
    set(IDF_TARGET $ENV{IDF_TARGET})
    message(STATUS "Setting IDF_TARGET to ${IDF_TARGET} from environment variable")
endif()

# Partition table settings
set(PARTITION_TABLE_CUSTOM_FILENAME "partitions/min_spiffs.csv")
set(PARTITION_TABLE_CUSTOM_PARTITION_FILENAME "partitions/min_spiffs.csv")
add_link_options("-Wl,-z,muldefs")

if(DEFINED ENV{FLAPPY_GHOST_WEB_HOOK})
    set(FLAPPY_GHOST_WEB_HOOK "$ENV{FLAPPY_GHOST_WEB_HOOK}")
    add_compile_definitions(FLAPPY_GHOST_WEB_HOOK=\"${FLAPPY_GHOST_WEB_HOOK}\")
endif()

# LED configuration
if(DEFINED ENV{LED_DATA_PIN})
    add_compile_definitions(LED_DATA_PIN=$ENV{LED_DATA_PIN})
endif()
if(DEFINED ENV{NUM_LEDS})
    add_compile_definitions(NUM_LEDS=$ENV{NUM_LEDS})
endif()

# Screen support and resolution
if(DEFINED ENV{WITH_SCREEN} AND "$ENV{WITH_SCREEN}" STREQUAL "1")
    add_compile_definitions(WITH_SCREEN=1)
endif()

if(DEFINED ENV{TFT_WIDTH})
    add_compile_definitions(TFT_WIDTH=$ENV{TFT_WIDTH})
else()
    add_compile_definitions(TFT_WIDTH=128)
endif()

if(DEFINED ENV{TFT_HEIGHT})
    add_compile_definitions(TFT_HEIGHT=$ENV{TFT_HEIGHT})
else()
    add_compile_definitions(TFT_HEIGHT=128)
endif()

# Input type
if(DEFINED ENV{USE_TOUCHSCREEN} AND "$ENV{USE_TOUCHSCREEN}" STREQUAL "1")
    add_compile_definitions(USE_TOUCHSCREEN=1)
endif()
if(DEFINED ENV{USE_JOYSTICK} AND "$ENV{USE_JOYSTICK}" STREQUAL "1")
    add_compile_definitions(USE_JOYSTICK=1)
endif()
if(DEFINED ENV{USE_CARDPUTER} AND "$ENV{USE_CARDPUTER}" STREQUAL "1")
    add_compile_definitions(USE_CARDPUTER=1)
endif()

# Ghost board configuration
if(DEFINED ENV{IS_GHOST_BOARD} AND "$ENV{IS_GHOST_BOARD}" STREQUAL "1")
    add_compile_definitions(IS_GHOST_BOARD=1)
endif()

# 7-inch display support
if(DEFINED ENV{USE_7_INCHER} AND "$ENV{USE_7_INCHER}" STREQUAL "0")
    add_compile_definitions(USE_7_INCHER=0)
endif()
if(DEFINED ENV{Waveshare_LCD} AND "$ENV{Waveshare_LCD}" STREQUAL "1")
    add_compile_definitions(Waveshare_LCD=1)
endif()
if(DEFINED ENV{Crowtech_LCD} AND "$ENV{Crowtech_LCD}" STREQUAL "1")
    add_compile_definitions(Crowtech_LCD=1)
endif()
if(DEFINED ENV{Sunton_LCD} AND "$ENV{Sunton_LCD}" STREQUAL "1")
    add_compile_definitions(Sunton_LCD=1)
endif()

# Werid Display Module
if(DEFINED ENV{USE_ILI9341_2} AND "$ENV{USE_ILI9341_2}" STREQUAL "1")
    add_compile_definitions(USE_ILI9341_2=1)
endif()

# GPS
if(DEFINED ENV{HAS_GPS} AND "$ENV{HAS_GPS}" STREQUAL "1")
    add_compile_definitions(HAS_GPS=1)
endif()
if(DEFINED ENV{GPS_UART_RX_PIN})
    add_compile_definitions(GPS_UART_RX_PIN=$ENV{GPS_UART_RX_PIN})
endif()
if(DEFINED ENV{GPS_UART_TX_PIN})
    add_compile_definitions(GPS_UART_TX_PIN=$ENV{GPS_UART_TX_PIN})
endif()

# SPI and MMC configuration
if(DEFINED ENV{USING_SPI} AND "$ENV{USING_SPI}" STREQUAL "1")
    add_compile_definitions(USING_SPI=1)
endif()
if(DEFINED ENV{USING_MMC} AND "$ENV{USING_MMC}" STREQUAL "1")
    add_compile_definitions(USING_MMC=1)
endif()

# SD Card SPI Pins configuration
if(DEFINED ENV{SD_SPI_CS_PIN})
    add_compile_definitions(SD_SPI_CS_PIN=$ENV{SD_SPI_CS_PIN})
else()
    add_compile_definitions(SD_SPI_CS_PIN=4)
endif()

if(DEFINED ENV{SD_SPI_CLK_PIN})
    add_compile_definitions(SD_SPI_CLK_PIN=$ENV{SD_SPI_CLK_PIN})
else()
    add_compile_definitions(SD_SPI_CLK_PIN=18)
endif()

if(DEFINED ENV{SD_SPI_MISO_PIN})
    add_compile_definitions(SD_SPI_MISO_PIN=$ENV{SD_SPI_MISO_PIN})
else()
    add_compile_definitions(SD_SPI_MISO_PIN=19)
endif()

if(DEFINED ENV{SD_SPI_MOSI_PIN})
    add_compile_definitions(SD_SPI_MOSI_PIN=$ENV{SD_SPI_MOSI_PIN})
else()
    add_compile_definitions(SD_SPI_MOSI_PIN=23)
endif()

if(DEFINED ENV{RED_RGB_PIN})
    add_compile_definitions(RED_RGB_PIN=$ENV{RED_RGB_PIN})
endif()

if(DEFINED ENV{GREEN_RGB_PIN})
    add_compile_definitions(GREEN_RGB_PIN=$ENV{GREEN_RGB_PIN})
endif()

if(DEFINED ENV{BLUE_RGB_PIN})
    add_compile_definitions(BLUE_RGB_PIN=$ENV{BLUE_RGB_PIN})
endif()

# Other default configurations
add_compile_definitions(SDSPIHOST=3)
add_compile_definitions(HOLD_LIMIT=1000) # For joystick hold duration
add_compile_definitions(LED_ORDER=0)     # 2 for RGB, 0 for GRB
add_compile_definitions(DNS_SERVER_MAX_ITEMS=1)
add_compile_definitions(MAX_WPS_NETWORKS=15)

# Include project settings
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Ghost_ESP_IDF)