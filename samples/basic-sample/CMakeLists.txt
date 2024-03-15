cmake_minimum_required(VERSION 3.0.2)

project(basic-sample)

# don't know how to best do this. The intent is to add our include path and the define in order to pick up iotcl_config.h
add_compile_options(-DIOTCL_USER_CONFIG_FILE=\"iotcl_config.h\" -I${CMAKE_CURRENT_SOURCE_DIR}/config)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)

# workaround for clion?
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENSSL")

# adding this for cleanness but the add_compile_options above already took care of it
include_directories(config)

add_executable(basic-sample main.c)
target_link_libraries(basic-sample iotc-c-generic-sdk)
