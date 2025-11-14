cmake_minimum_required(VERSION 3.10)

function(add_yyaml_cpp_example target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/includes)
    target_link_libraries(${target} PRIVATE yyaml)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_compile_definitions(${target} PRIVATE
        YYAML_EXAMPLE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data")
endfunction()

add_yyaml_cpp_example(yyaml_cpp_inspect
    inspect_config.cpp)

add_yyaml_cpp_example(yyaml_cpp_roundtrip
    roundtrip_complex.cpp)

add_yyaml_cpp_example(yyaml_cpp_dump_json
    dumps_as_json.cpp)
