cmake_minimum_required(VERSION 3.17)

project(VolumeReader CXX)

option(BUILD_TEST "whether build test" ON)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)

if(BUILD_TEST)
    add_subdirectory(test)
endif()