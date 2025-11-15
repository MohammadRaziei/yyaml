cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB EXAMPLE_CPP_SAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cpp_sample*.cpp")
list(SORT EXAMPLE_CPP_SAMPLE_SOURCES)

foreach(sample_source IN LISTS EXAMPLE_CPP_SAMPLE_SOURCES)
    get_filename_component(sample_name "${sample_source}" NAME_WE)

    add_executable(${sample_name}
        ${sample_source}
        ${EXAMPLE_COMMON_SOURCES}
    )
    target_include_directories(${sample_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${sample_name} PRIVATE yyaml)
endforeach()