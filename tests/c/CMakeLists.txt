# CMake configuration for tests directory
cmake_minimum_required(VERSION 3.10)

# Add bundled Unity as a subdirectory
add_subdirectory(bundled)

# Create test executables
add_executable(yyaml_tests
    test_yyaml.c
)

add_executable(yyaml_file_tests
    test_yaml_files.c
)

# Create debug executable
add_executable(debug_yaml
    debug_yaml.c
)

# Link with Unity and yyaml libraries
target_link_libraries(yyaml_tests
    Unity
    yyaml
)

target_link_libraries(yyaml_file_tests
    Unity
    yyaml
)

target_link_libraries(debug_yaml
    yyaml
)

# Include directories
target_include_directories(yyaml_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bundled
    ${CMAKE_SOURCE_DIR}/includes
)

target_include_directories(yyaml_file_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bundled
    ${CMAKE_SOURCE_DIR}/includes
)

target_include_directories(debug_yaml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bundled
    ${CMAKE_SOURCE_DIR}/includes
)

target_compile_definitions(yyaml_file_tests PRIVATE
    YYAML_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
)

# Optionally enable testing
enable_testing()
add_test(NAME yyaml_tests COMMAND yyaml_tests)
add_test(NAME yyaml_file_tests COMMAND yyaml_file_tests)
