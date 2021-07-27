## Introduction

This IoTConnect C SDK is intended for standard operating systems Linux/Windows/MacOS.

The SDK can be integrated with Azure IoT C SDK or Paho MQTT C Client.

The Paho MQTT implementation does not support TPM authentication.

## Building

It is recommended to use JetBrains CLion IDE with the sample projects, but you can also build the projects
with cmake command line tool manually.

Note that the following build options are configured by default and cna be togled with cmake flags:
* IOTC_USE_PAHO=OFF - builds with Azure IoT C SDK by default.
* IOTC_TPM_SUPPORT=OFF - no TPM support by default

#### Dependencies

The project depends on curl and openssl libraries.

Both the shared libraries and the C source headers are required to be present on the build host for building. 
Curl and openssl shared libraries must be present on the device when running the project. 

On Ubuntu, you can run the following command to set up for development: 

```shell script
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

The project uses the following dependent projects as git submodules:

* [cJSON](https://github.com/DaveGamble/cJSON.git) v1.7.13
* [iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib.git) v2.0.2
* [azure-iot-sdk-c](https://github.com/Azure/azure-iot-sdk-c.git) lts_01_2021
* [paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c.git) v1.3.9

#### Initializing/Downloading The Project

This project has git submodules that need to be pulled before building. You can either:

* Download this project from the GitHub Releases or Actions
* or Clone this repo and perform the following steps: 
  * Initialize the project using the script on Linux/MacOS with ```scripts/setup-project.sh```
  * or examine the script and perform the actions for your OS.
* Edit samples/basic-sample/config/app_config.h to reflect your account and device's configuration.
* If using CA Certificate based authentication, follow the instructions in the 
[iotc-c-lib/tools/ecc-certs](https://github.com/avnet-iotconnect/iotc-c-lib/tree/master/tools/ecc-certs) 
repo and create the identify for your device. 
Place the device certificate and private key into *certs/client-crt.pem* and *certs/client-key.pem* in the basic-sample project.

#### Building and Running with CLion

* In CLion, open the *basic-sample* project from the *samples* directory of this repo
* In the top right of of the IDE next to the hammer icon, select *basic-sample*
* If you wish to build with the Paho MQTT client insted of Azure SDK, Select File->Settings->Build,Execution,Deployment->CMake 
and enter ```-DIOTC_USE_PAHO=ON``` in the "CMake options" entry box. 
Similarly, you can enable TPM support with ```-DIOTC_TPM_SUPPORT=ON```.
* Click the build, execute or debug icon.

#### Building and Running with CMake Command Line

```shell script
cd samples/basic-sample
mkdir build
cd build
cmake ..
cmake --build . --target basic-sample
./basic-sample
```

* If you wish to build with the Paho MQTT client, append ```-DIOTC_USE_PAHO=ON``` to the ```cmake ..``` command line.
* If you wish to build with TPM support, append ```-DIOTC_TPM_SUPPORT=ON``` (with azure SDK) to the ```cmake ..``` command line.

