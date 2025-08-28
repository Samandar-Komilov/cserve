cmake_minimum_required(VERSION 3.10)
project(Cserve VERSION 0.2.0 LANGUAGES C)

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure the header file (generates in build directory)
configure_file(
    "${CMAKE_SOURCE_DIR}/cserve_config.h.in"
    "${CMAKE_BINARY_DIR}/cserve_config.h"
)

add_subdirectory(src)
add_subdirectory(tests)

# Doxygen Config
find_package(Doxygen)
if (DOXYGEN_FOUND)
    message(STATUS "Doxygen found, generating documentation...")
    
    configure_file(
        "${CMAKE_SOURCE_DIR}/Doxyfile.in"
        "${CMAKE_BINARY_DIR}/Doxyfile"
        @ONLY
    )

    add_custom_target(doxygen_doc
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found. Documentation will not be generated.")
endif()