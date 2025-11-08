cmake_minimum_required(VERSION 3.10)
project(yyaml VERSION 1.0.0 LANGUAGES C)

option(BUILD_EXAMPLES "Build yyaml example programs" OFF)
option(BUILD_PYTHON "Build yyaml example programs" OFF)

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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


if(${BUILD_PYTHON} OR DEFINED SKBUILD)

    # Find Python and Cython
    find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(Cython MODULE REQUIRED VERSION 3.0)
    include(UseCython)


    # Set Python module output directory
    # set(PYTHON_MODULE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/yyaml)

    # Transpile Cython to C++
    cython_transpile(src/python/yyaml.pyx LANGUAGE CXX OUTPUT_VARIABLE yyaml_cpp_file)

    # Create the Python extension module
    python_add_library(yyaml_python MODULE "${yyaml_cpp_file}" WITH_SOABI)


    # Link pugixml to our module
    target_link_libraries(yyaml_python PRIVATE yyaml)

    set_target_properties(yyaml_python NAME yyaml)



    if (DEFINED SKBUILD)
        # If using scikit-build, set the output directory for the module
        # set_target_properties(yyaml_python PROPERTIES
        #     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${SKBUILD_PROJECT_NAME}
        # )

        install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/python/yyaml
            DESTINATION ${SKBUILD_PROJECT_NAME}
            FILES_MATCHING PATTERN "*.py"
        )

        # Install the module
        install(TARGETS yyaml_python
            DESTINATION ${SKBUILD_PROJECT_NAME}
        )

        RETURN()
    else()
        # Install the module to the standard Python site-packages directory
        install(TARGETS yyaml_python
            LIBRARY DESTINATION ${Python_SITEARCH}
        )
    endif()
endif()


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
