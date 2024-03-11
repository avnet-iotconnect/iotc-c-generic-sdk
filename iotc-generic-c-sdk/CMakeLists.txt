cmake_minimum_required(VERSION 3.0.2)

project(iotc-generic-c-sdk)

# Affects all dependencies.
# Building with shared libs can cause problems in Windows
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

#cJSON
set(ENABLE_CJSON_TEST OFF CACHE BOOL "CJson - Build Tests")
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "CJson - Custom Compiler Flags")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/iotc-c-lib/lib/cJSON cJSON EXCLUDE_FROM_ALL)

#paho.mqtt.c
set(PAHO_BUILD_SHARED OFF CACHE BOOL "Paho - Build with Shared Libraries")
set(PAHO_BUILD_STATIC ON CACHE BOOL "Paho - Build with Static Libraries")
set(PAHO_WITH_SSL ON CACHE BOOL "Paho - Use SSL")
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "Paho - Build Samples")
set(PAHO_ENABLE_TESTING OFF CACHE BOOL "Paho - Enable Tesing")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/paho.mqtt.c/src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/paho.mqtt.c paho.mqtt.c EXCLUDE_FROM_ALL)

# iotc-c-lib
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/iotc-c-lib/src CLibSources)

include_directories(curl-http-impl/include)
include_directories(include)

file(GLOB SdkSources src/*.c curl-http-impl/src/*.c)
file(GLOB ImplSources paho-c-impl/src/*.c)

add_library(iotc-c-generic-sdk STATIC ${cJSON} ${CLibSources} ${SdkSources} ${ImplSources})

target_include_directories(iotc-c-generic-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/cJSON)
target_include_directories(iotc-c-generic-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/iotc-c-lib/include)
target_include_directories(iotc-c-generic-sdk PUBLIC include)

IF (PAHO_BUILD_STATIC)
    target_link_libraries(iotc-c-generic-sdk paho-mqtt3cs-static)
ELSE ()
    target_link_libraries(iotc-c-generic-sdk paho-mqtt3cs)
ENDIF ()

IF (CMAKE_TOOLCHAIN_FILE)
    # not the best way to detect VCPKG, but we'll go with that
    find_package(CURL CONFIG REQUIRED)
ELSE ()
    find_package(CURL REQUIRED)
ENDIF()

target_link_libraries(iotc-c-generic-sdk CURL::libcurl)

target_link_libraries(iotc-c-generic-sdk cjson)



