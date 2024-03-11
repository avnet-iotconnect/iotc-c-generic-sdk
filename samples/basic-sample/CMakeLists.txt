cmake_minimum_required(VERSION 3.0.2)

project(basic-sample)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)

include_directories(config PRIVATE)
add_executable(basic-sample main.c)
target_link_libraries(basic-sample iotc-c-generic-sdk)
