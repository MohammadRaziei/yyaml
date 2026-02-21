# Tests CMake configuration
cmake_minimum_required(VERSION 3.10)

# C tests - compile all test_*.c files into one binary using GLOB
file(GLOB C_TEST_SOURCES "c/test_*.c")

# C test executable
add_executable(yyaml_c_tests
    ${C_TEST_SOURCES}
)

target_include_directories(yyaml_c_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/tests/utest
    ${CMAKE_CURRENT_SOURCE_DIR}/c
)

target_link_libraries(yyaml_c_tests PRIVATE yyaml)

target_compile_definitions(yyaml_c_tests PRIVATE
    YYAML_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data"
)

# C++ tests - compile all test_*.cpp files into one binary using GLOB
file(GLOB CPP_TEST_SOURCES "cpp/test_*.cpp")

# C++ test executable
add_executable(yyaml_cpp_tests
    ${CPP_TEST_SOURCES}
)

target_include_directories(yyaml_cpp_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/tests/utest
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

target_link_libraries(yyaml_cpp_tests PRIVATE yyaml)

target_compile_features(yyaml_cpp_tests PRIVATE cxx_std_17)

# Enable testing
enable_testing()
add_test(NAME yyaml_c_tests COMMAND yyaml_c_tests)
add_test(NAME yyaml_cpp_tests COMMAND yyaml_cpp_tests)