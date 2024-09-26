# Gather all .c files recursively from the src/ folder
file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/main/*.c")

# Gather all include directories within the include/ folder
file(GLOB_RECURSE inc_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/include/*")

# Filter out only directories (not files)
set(filtered_inc_dirs "")
foreach(inc_dir ${inc_dirs})
    if(IS_DIRECTORY ${inc_dir})
        list(APPEND filtered_inc_dirs ${inc_dir})
    endif()
endforeach()

# Register the component with the dynamically collected source files and include directories
idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"
                       REQUIRES bt nvs_flash driver esp_http_server mdns json)