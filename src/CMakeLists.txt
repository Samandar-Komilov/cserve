cmake_minimum_required(VERSION 3.10)
project(Cserve VERSION 0.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

configure_file(cserve_config.h.in cserve_config.h)

add_executable(cserve main.c)

target_link_libraries(cserve m)

target_include_directories(cserve PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
