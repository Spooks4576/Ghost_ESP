# Ensure only C source files are included
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c)
idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "." REQUIRES lvgl)