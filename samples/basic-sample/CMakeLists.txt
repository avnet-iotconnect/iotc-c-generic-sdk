cmake_minimum_required(VERSION 3.0.2)

project(basic-sample)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIOTCL_USER_CONFIG_FILE='\"iotcl_config.h\"' -Wall -Wextra -std=c99")

# workaround for clion?
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENSSL")
include_directories(config)
add_executable(basic-sample main.c)
target_link_libraries(basic-sample iotc-c-generic-sdk)
