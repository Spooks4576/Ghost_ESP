# Automatically collect all source files from the src directory
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Register the component and link LVGL and drivers
idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS "."
                       REQUIRES lvgl lvgl_esp32_drivers)