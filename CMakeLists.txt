cmake_minimum_required(VERSION 3.17)

project(VolumeReader CXX)

option(BUILD_TEST "whether build test" ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(VolumeReader INTERFACE)
target_include_directories(
        VolumeReader
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(VolumeReader INTERFACE cxx_std_14)

if(BUILD_TEST)
    add_subdirectory(test)
endif()