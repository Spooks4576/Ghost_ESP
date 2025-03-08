file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/main/*.c")
file(GLOB_RECURSE axs15231b_sources "${CMAKE_SOURCE_DIR}/components/axs15231b/*.c")


list(APPEND app_sources ${axs15231b_sources})


set(required_components bt nvs_flash driver esp_http_server mdns json esp_http_client mbedtls fatfs sdmmc wpa_supplicant lvgl lvgl_esp32_drivers freertos esp_lcd)


set(sources ${app_sources})


if("${IDF_TARGET}" STREQUAL "esp32s3" AND CONFIG_USE_CARDPUTER)
    # Add the M5GFX wrapper source file
    list(APPEND sources "vendor/m5/m5gfx_wrapper.cpp")
    # Add M5GFX to the required components
    list(APPEND required_components M5GFX)
endif()


idf_component_register(SRCS ${sources}
                       INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"
                       REQUIRES ${required_components})
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-incompatible-pointer-types)
