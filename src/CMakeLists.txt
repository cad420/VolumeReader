add_library(VolumeReader STATIC volume_reader.cpp)
target_include_directories(VolumeReader PRIVATE ${PROJECT_SOURCE_DIR}/include)