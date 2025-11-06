cmake_minimum_required(VERSION 3.10)
project(yyaml VERSION 1.0.0 LANGUAGES C)

option(BUILD_EXAMPLES "Build yyaml example programs" OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Create library target
add_library(yyaml STATIC src/yyaml.c)

# Include directories
target_include_directories(yyaml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

# Compiler options
target_compile_options(yyaml PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Install targets
install(TARGETS yyaml
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY includes/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Enable testing
enable_testing()

# Add tests directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples/c)
endif()
