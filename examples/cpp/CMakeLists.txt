cmake_minimum_required(VERSION 3.10)

project(yyaml_cpp_examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB EXAMPLE_CPP_SAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cpp_sample*.cpp")
list(SORT EXAMPLE_CPP_SAMPLE_SOURCES)

include(FetchContent)
# cmake_policy(SET CMP0002 NEW)

set(FETCHCONTENT_QUIET OFF)


FetchContent_Declare(
    yyaml
    GIT_REPOSITORY https://github.com/mohammadraziei/yyaml.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(yyaml)

foreach(sample_source IN LISTS EXAMPLE_CPP_SAMPLE_SOURCES)
    get_filename_component(sample_name "${sample_source}" NAME_WE)

    add_executable(${sample_name}
        ${sample_source}
        ${EXAMPLE_COMMON_SOURCES}
    )
    target_include_directories(${sample_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${sample_name} PRIVATE yyaml::yyamlcpp)
endforeach()