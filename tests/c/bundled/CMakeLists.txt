# CMake configuration for bundled Unity test framework
cmake_minimum_required(VERSION 3.10)

# Create Unity library
add_library(Unity STATIC
    unity.c
)

# Include directories
target_include_directories(Unity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler options
target_compile_options(Unity PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Set C standard
set_target_properties(Unity PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)
