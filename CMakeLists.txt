cmake_minimum_required(VERSION 3.20)

project(CServe
    VERSION 0.1.0
    DESCRIPTION "A lightweight HTTP server with reverse proxy support"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GNU extensions required: memmem(), asprintf(), epoll
add_compile_options(-Wall -Wextra -D_GNU_SOURCE)

# ---------------------------------------------------------------
# Core library (shared by server binary and test runner)
# ---------------------------------------------------------------
add_library(cserve_core STATIC
    src/sock/server.c
    src/http/request.c
    src/http/response.c
    src/http/parsers.c
    src/http/server.c
    src/utils/config.c
    src/utils/logger.c
)
target_include_directories(cserve_core PUBLIC src)

# ---------------------------------------------------------------
# Server binary
# ---------------------------------------------------------------
add_executable(cserve src/main.c)
target_link_libraries(cserve PRIVATE cserve_core)

# ---------------------------------------------------------------
# Tests
# ---------------------------------------------------------------
enable_testing()

add_executable(test_runner tests/test_parsers.c)
target_include_directories(test_runner PRIVATE src)
target_link_libraries(test_runner PRIVATE cserve_core)

add_test(NAME unit_tests COMMAND test_runner)

# ---------------------------------------------------------------
# Doxygen (optional)
# ---------------------------------------------------------------
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    message(STATUS "Doxygen found — run 'make docs' to generate docs in build/doc/html/")
else()
    message(STATUS "Doxygen not found — install doxygen to enable 'make docs'")
endif()
