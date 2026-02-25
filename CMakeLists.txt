cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PyProject)
include(DynamicVersion)

extract_version_string(
    HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/yyaml/yyaml.h"
    VERSION_PREFIX "YYAML_"
    OUTPUT_VAR PROJECT_VERSION
)

project(yyaml VERSION ${PROJECT_VERSION} LANGUAGES C CXX)
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")

include(Startup)

option(BUILD_TESTS "Build yyaml test suites" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_PYTHON "Build yyaml Python bindings" OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create library target
add_library(yyaml STATIC yyaml/yyaml.c)
target_include_directories(yyaml PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/yyaml
)
target_compile_options(yyaml PRIVATE -Wall -Wextra -Wpedantic)
add_library(yyaml::yyaml ALIAS yyaml)


add_library(yyaml_cpp INTERFACE)
target_link_libraries(yyaml_cpp INTERFACE yyaml::yyaml)
target_include_directories(yyaml_cpp INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/cpp
)
add_library(yyaml::yyamlcpp ALIAS yyaml_cpp)


# Optimize for performance
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# Additional optimization flags for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SAFE_FLAGS "-fomit-frame-pointer -fno-stack-protector")
    if(NOT APPLE)
        set(SAFE_FLAGS "${SAFE_FLAGS} -march=native -mtune=native")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SAFE_FLAGS}")
endif()

# Set build type to Release for better performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(BUILD_PYTHON OR DEFINED SKBUILD)

    # Find Python and Cython
    find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(Cython MODULE REQUIRED VERSION 3.0)
    include(UseCython)


    # Transpile Cython to C
    cython_transpile(${PROJECT_SOURCE_DIR}/bindings/python/yyaml_python.pyx LANGUAGE CXX OUTPUT_VARIABLE yyaml_c_file)

    # Create the Python extension module
    python_add_library(yyaml_python MODULE "${yyaml_c_file}" WITH_SOABI)


    # Link pugixml to our module
    target_link_libraries(yyaml_python PRIVATE yyaml)


    if (DEFINED SKBUILD)
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/bindings/python/yyaml
            DESTINATION .
            FILES_MATCHING PATTERN "*.py"
        )

        # Install the module
        install(TARGETS yyaml_python
            DESTINATION yyaml
        )

        RETURN()
    else()
        # Install the module to the standard Python site-packages directory
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/bindings/python/yyaml
            DESTINATION ${CMAKE_BINARY_DIR}
            FILES_MATCHING PATTERN "*.py"
        )
        install(TARGETS yyaml_python
            LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/yyaml
        )
    endif()
endif()


# Install targets
install(TARGETS yyaml
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY yyaml/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(FILES bindings/cpp/yyaml.hpp
    DESTINATION include
)

if(BUILD_TESTS)
    enable_testing()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

