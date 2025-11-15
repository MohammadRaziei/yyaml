set(EXAMPLE_COMMON_SOURCES
    common.c
)

file(GLOB EXAMPLE_C_SAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/c_sample*.c")
list(SORT EXAMPLE_C_SAMPLE_SOURCES)

foreach(sample_source IN LISTS EXAMPLE_C_SAMPLE_SOURCES)
    get_filename_component(sample_name "${sample_source}" NAME_WE)

    add_executable(${sample_name}
        ${sample_source}
        ${EXAMPLE_COMMON_SOURCES}
    )
    target_include_directories(${sample_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${sample_name} PRIVATE yyaml)
endforeach()
