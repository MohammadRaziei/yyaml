cmake_minimum_required(VERSION 3.10)

add_executable(yyaml_cpp_tests
    test_document.cpp
)

target_include_directories(yyaml_cpp_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(yyaml_cpp_tests PRIVATE yyaml)

target_compile_features(yyaml_cpp_tests PRIVATE cxx_std_17)

add_test(NAME yyaml_cpp_tests COMMAND yyaml_cpp_tests)
