# Create the executable
add_executable(cserve main.c)

# Link libraries
target_link_libraries(cserve m)

# Include directories - point to build dir for generated config
target_include_directories(cserve PUBLIC 
    "${CMAKE_BINARY_DIR}"          # for cserve_config.h in build dir
    "${CMAKE_CURRENT_SOURCE_DIR}"  # for source headers in src dir
)

add_subdirectory(core)
add_subdirectory(http)