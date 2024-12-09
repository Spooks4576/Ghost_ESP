# Gather all .c files recursively from the src/ folder
file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/main/*.c")
file(GLOB_RECURSE axs15231b_sources "${CMAKE_SOURCE_DIR}/components/axs15231b/*.c")

# Gather all include directories within the include/ folder
file(GLOB_RECURSE inc_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/include/*")
file(GLOB_RECURSE axs15231b_inc_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/components/*")

list(APPEND app_sources ${axs15231b_sources})

# Filter out only directories (not files)
set(filtered_inc_dirs "")
foreach(inc_dir ${inc_dirs})
    if(IS_DIRECTORY ${inc_dir})
        list(APPEND filtered_inc_dirs ${inc_dir})
    endif()
endforeach()

# Base required components
set(required_components bt nvs_flash driver esp_http_server mdns json esp_http_client mbedtls fatfs sdmmc wpa_supplicant lvgl lvgl_esp32_drivers freertos esp_lcd M5GFX)

# Register the component with the dynamically collected source files and include directories
idf_component_register(SRCS ${app_sources} "vendor/m5/m5gfx_wrapper.cpp"
                       INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"
                       REQUIRES ${required_components})