## Introduction

This IoTConnect C SDK is intended for standard operating systems Linux/Windows/MacOS
with OpenSSL support.

Paho C MQTT library is used as an underlying implementation.

Use the main branch for protocol 2.1 devices.

Use the rel-protocol-1.0 branch for protocol 1.0 devices, or if TPM or if 
[Azure IoT C SDK](https://github.com/Azure/azure-iot-sdk-c) integration is required for your project.

To get started quickly, see the [IoTConnect Generic C SDK Windows](https://www.youtube.com/watch?v=cvP3zmcs8JA) and [SmartEdge Industrial IoT Gateway](https://www.youtube.com/watch?v=j6AC95nz7IY) demo videos on YouTube.
The videos use the older protocol 1.0 support version, but most of the concepts can be applied to this implementation. 
 
## Dependencies

The project uses the following dependent projects as git submodules:

* [iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib.git) from source (v3.1.0-proto-2.1)
* [cJSON](https://github.com/DaveGamble/cJSON.git) from source (as iotc-c-lib dependency)
* [paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c.git) from source - v1.3.13
* [libcurl](https://curl.se/libcurl/) as a dynamically linked library.  
* [oenssl](https://www.openssl.org/) as a dynamically linked library, as a dependency from paho and curl.

The project depends on the following linked libraries:
 * [libcurl](https://curl.se/libcurl/)
 * OpenSSL library (reused from Paho dependency).

    
Both the shared libraries and the C source headers are required to be present on the build host for building. 
Curl and openssl runtime shared libraries (so, dll etc.) must be present on the device when running the project. 

## Project Setup

This project has git submodules that need to be pulled before building.
Ensure to pass the **--recurse-submodules** flag to your git clone command 
when cloning the repo.

If the project was already cloned without submodules or if you need to pull 
the necessary modules for a different branch after pulling, run the following command
in the root of this repo: 
```shell script
git submodule update --init --recursive
``` 
or execute *scripts/setup-project.sh* with bash.

* Follow the instructions for your OS to build the project:
  * [Linux Instructions](doc/Linux.md)
  * [Windows Instructions](doc/Windows.md) 
* Edit samples/basic-sample/config/app_config.h to reflect your account and device's configuration.
* If using certificate (X509) based authentication, you can generate your own test certificates
by using the samples in the 
[iotc-c-lib/tools/ecc-certs](https://github.com/avnet-iotconnect/iotc-c-lib/tree/master/tools/) directory. 
repo and create the identify for your device.
Place the device certificate and private key into *certs/client-crt.pem* and *certs/client-key.pem* in the basic-sample project.
* Build or re-build the project after editing the *app_config.h* file.  
