cmake_minimum_required(VERSION 2.8.4)
project(string-format)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(LIB_SOURCE_FILES
    utils/format.cpp
    utils/format.h)

set(SAMPLE_SOURCE_FILES
    test/test.cpp)

add_library(utils STATIC ${LIB_SOURCE_FILES})

add_executable(string-format ${SAMPLE_SOURCE_FILES})
target_link_libraries(string-format utils)
