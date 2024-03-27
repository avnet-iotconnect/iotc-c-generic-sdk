cmake_minimum_required(VERSION 3.0.2)

project(basic-sample)

# don't know how to best do this. The intent is to add our include path and the define in order to pick up iotcl_config.h
add_compile_options(-DIOTCL_USER_CONFIG_FILE=\"iotcl_config.h\" -I${CMAKE_CURRENT_SOURCE_DIR}/config)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)

# adding this for cleanness but the add_compile_options above already took care of it
include_directories(config)

add_executable(basic-sample main.c)

if(CMAKE_COMPILER_IS_GNUCXX)
    # expect warnings about usleep and unused arguments from paho
    target_compile_options(basic-sample PRIVATE -std=c99 -Wall -Wextra)
    target_compile_options(iotc-c-generic-sdk PRIVATE -std=c99 -Wall -Wextra)
endif(CMAKE_COMPILER_IS_GNUCXX)


if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # We should be always checking if sprintf, strcat and whatnot will fit, and/or allocating adequate buffer sizes,
    # so disable these types of warnings for MSVC compiler
    target_compile_definitions(iotc-c-generic-sdk PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()


target_link_libraries(basic-sample iotc-c-generic-sdk)
