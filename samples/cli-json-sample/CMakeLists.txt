cmake_minimum_required(VERSION 3.0.2)

project(cli-json-sample)

set(IOTC_USE_PAHO OFF CACHE BOOL "")
set(IOTC_TPM_SUPPORT OFF CACHE BOOL "") # NOTE: IOTC_USE_PAHO needs to be set to OFF for this
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)


set(X509_CONF x509_config/)
set(SYMMKEY_CONF symmkey/)

set(SYMMKEY_APP main.c ${SYMMKEY_CONF}/app_config.h)
set(X509_APP main.c ${X509_CONF}/app_config.h)

add_executable(cli-json-sample ${SYMMKEY_APP})
add_executable(cli-json-sample-x509 ${X509_APP})

target_include_directories(cli-json-sample PRIVATE ${SYMMKEY_CONF})
target_include_directories(cli-json-sample-x509 PRIVATE ${X509_CONF})

target_link_libraries(cli-json-sample iotc-c-generic-sdk)
target_link_libraries(cli-json-sample-x509 iotc-c-generic-sdk)
