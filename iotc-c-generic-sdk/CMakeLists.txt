cmake_minimum_required(VERSION 3.0.2)

project(iotc-generic-c-sdk)

option(IOTC_USE_PAHO "Use Paho Client. Otherwise the Azure Device SDK will be used." OFF)
option(IOTC_TPM_SUPPORT "Support TPM devices. Implies compiling support from provisioning client in Azure SDK"  OFF)

IF (IOTC_USE_PAHO AND IOTC_TPM_SUPPORT)
    MESSAGE(FATAL_ERROR "TPM functionality is not supported by Paho")
ENDIF()


#cJSON
set(ENABLE_CJSON_TEST OFF CACHE BOOL "CJson - Build Tests")
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "CJson - Custom Compiler Flags")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/cJSON cJSON EXCLUDE_FROM_ALL)

IF (IOTC_USE_PAHO)

    IF (IOTC_TPM_SUPPORT)
        MESSAGE(FATAL_ERROR "TPM functionality is not supported with the Paho client implementation")
    ENDIF()

    #paho.mqtt.c
    set(PAHO_BUILD_SHARED OFF CACHE BOOL "Paho - Build with Shared Libraries")
    set(PAHO_BUILD_STATIC ON CACHE BOOL "Paho - Build with Static Libraries")
    set(PAHO_WITH_SSL ON CACHE BOOL "Paho - Use SSL")
    set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "Paho - Build Samples")
    set(PAHO_ENABLE_TESTING OFF CACHE BOOL "Paho - Enable Tesing")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/paho.mqtt.c/src)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/paho.mqtt.c paho.mqtt.c EXCLUDE_FROM_ALL)
ELSE ()
    IF (IOTC_TPM_SUPPORT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIOTC_USE_PROVISIONING")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIOTC_USE_PROVISIONING")
        set(use_amqp ON CACHE BOOL "") # we need to use AMQP to because the MQTT client doesn't support TPM HSM
        set(use_prov_client ON CACHE BOOL "") # we need provisioning enabled for TPM
        set(hsm_type_sastoken ON CACHE BOOL "") # TPM uses sas token type HSM
    ELSE()
        set(use_amqp OFF CACHE BOOL "")
    ENDIF()
    set(use_http OFF CACHE BOOL "")
    set(run_e2e_tests OFF CACHE BOOL "")
    set(build_service_client OFF CACHE BOOL "")
    set(skip_samples ON CACHE BOOL "")

    #set(use_tpm_simulator ON CACHE BOOL "")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/azure-iot-sdk-c/deps/umock-c/inc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/azure-iot-sdk-c/deps/azure-macro-utils-c/inc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/azure-iot-sdk-c/iothub_client/inc)
    IF (IOTC_TPM_SUPPORT)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/azure-iot-sdk-c/provisioning_client/inc)
    ENDIF()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/azure-iot-sdk-c azure-iot-sdk-c EXCLUDE_FROM_ALL)
ENDIF ()

# iotc-c-lib
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/iotc-c-lib/src CLibSources)

include_directories(curl-http-impl/include)
include_directories(include)

file(GLOB SdkSources src/*.c curl-http-impl/src/*.c)
IF (IOTC_USE_PAHO)
    file(GLOB ImplSources paho-c-impl/src/*.c)
ELSE ()
    file(GLOB ImplSources azure-sdk-impl/src/*.c)
ENDIF ()

add_library(iotc-c-generic-sdk STATIC ${cJSON} ${CLibSources} ${SdkSources} ${ImplSources})

target_include_directories(iotc-c-generic-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/cJSON)
target_include_directories(iotc-c-generic-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/iotc-c-lib/include)
target_include_directories(iotc-c-generic-sdk PUBLIC include)

IF (IOTC_USE_PAHO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIOTC_USE_PAHO")
    IF (PAHO_BUILD_STATIC)
        target_link_libraries(iotc-c-generic-sdk paho-mqtt3cs-static)
    ELSE ()
        target_link_libraries(iotc-c-generic-sdk paho-mqtt3cs)
    ENDIF ()
ELSE ()
    IF(IOTC_TPM_SUPPORT)
         target_link_libraries(iotc-c-generic-sdk
                 iothub_client
                 iothub_client_mqtt_transport
                 prov_device_client
                 prov_amqp_transport
                 )
    ENDIF()
    target_link_libraries(iotc-c-generic-sdk
            iothub_client
            iothub_client_mqtt_transport
            )
ENDIF ()

target_link_libraries(iotc-c-generic-sdk cjson)
target_link_libraries(iotc-c-generic-sdk curl)

