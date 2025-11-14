if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/c/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c ${CMAKE_CURRENT_BINARY_DIR}/c)
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/cpp/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/cpp ${CMAKE_CURRENT_BINARY_DIR}/cpp)
endif()
