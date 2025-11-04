# CMake configuration for tests directory
cmake_minimum_required(VERSION 3.10)

# Add bundled Unity as a subdirectory
add_subdirectory(bundled)

# Create a test executable (placeholder - you can add your test files here)
# add_executable(yyaml_tests
#     # Add your test source files here
# )

# Link with Unity and yyaml libraries
# target_link_libraries(yyaml_tests
#     Unity
#     yyaml
# )

# Include directories
# target_include_directories(yyaml_tests PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/bundled
#     ${CMAKE_SOURCE_DIR}/includes
# )

# Optionally enable testing
# enable_testing()
# add_test(NAME yyaml_tests COMMAND yyaml_tests)
