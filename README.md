## Introduction

This IoTConnect C SDK is intended for standard operating systems Linux/Windows/MacOS.

The SDK can be integrated with Azure IoT C SDK or Paho MQTT C Client.

The Paho MQTT implementation does not support TPM authentication.

To get started quickly, see the [IoTConnect Generic C SDK Windows](https://www.youtube.com/watch?v=cvP3zmcs8JA) and [SmartEdge Industrial IoT Gateway](https://www.youtube.com/watch?v=j6AC95nz7IY) demo videos on YouTube.
 
#### CMake Build Options

Note that the following build options are configured by default and can be toggled with cmake flags:
* IOTC_USE_PAHO=ON - Builds with Paho MQTT Client instead of Azure IoT C SDK by default.
* IOTC_TPM_SUPPORT=ON -  Adds TPM Support for Azure IoT C SDK. Requires IOTC_USE_PAHO=OFF.

#### Dependencies

The project depends on curl openssl libraries and uuid library (uuid required for for Azure C SDK flavor onl) .

Both the shared libraries and the C source headers are required to be present on the build host for building. 
Curl and openssl shared libraries must be present on the device when running the project. 



The project uses the following dependent projects as git submodules:

* [cJSON](https://github.com/DaveGamble/cJSON.git) v1.7.13
* [iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib.git) v2.0.2
* [azure-iot-sdk-c](https://github.com/Azure/azure-iot-sdk-c.git) lts_01_2021
* [paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c.git) v1.3.9

#### Initializing/Downloading The Project

This project has git submodules that need to be pulled before building. You can either:

* Download this project from the GitHub Releases or Actions
* or Clone this repo or download and perform the following steps:
* Follow the instructions for your OS:
  * [Linux Instructions](doc/Linux.md)
  * [Windows Instructions](doc/Windows.md) 
* Edit samples/basic-sample/config/app_config.h to reflect your account and device's configuration.
* If using CA Certificate based authentication, follow the instructions in the 
[iotc-c-lib/tools/ecc-certs](https://github.com/avnet-iotconnect/iotc-c-lib/tree/master/tools/ecc-certs) 
repo and create the identify for your device.
Place the device certificate and private key into *certs/client-crt.pem* and *certs/client-key.pem* in the basic-sample project.
* Build or re-build the project after editing the *app_config.h* file.  

#### JSON config

You can also pass a json file parameter to `basic-sample` - for example `basic-sample ../my_config.json` which would be used instad of `app_config.h` file.

Current expected JSON format is:
```json
{
    "duid": "azRTOS-demo-blueboard",
    "cpid": "av",
    "env": "avn",
    "auth_type": "IOTC_AT_SYMMETRIC_KEY",
    "symmkey": "UN0TwIaNf4LOOQVt79t3rw==",
    "sensors": [{
        "name": "light_sensor",
        "path": "/home/some_file_to_read"
    },
    {
        "name": "test_sensor",
        "path": "/home/some_file_to_read_v3"
    },
    {
        "name": "nonsense-sensor",
        "path": "/home/some_file_to_read_v2"
    }]
}
```

Where `duid`, `cpid`, `env` and `auth_type` fields are mandatory.
Depending on `auth_type` some other fields become mandatory too.
In example above, because `auth_type` is set to `IOTC_AT_SYMMETRIC_KEY`, `symmkey` field is required too.
If `auth_type` is set to `IOTC_AT_X509` a `x509_certs` object is required. See example below.

```json
{
    "duid": "CSDKSelfSigned",
    "cpid": "av",
    "env": "avn",
    "auth_type": "IOTC_AT_X509",
    "x509_certs": {
        "client_key": "/home/client1-key.pem",
        "client_cert": "/home/client1.pem"
    },
    "sensors": [{
        "name": "light_sensor",
        "path": "/home/some_file_to_read"
    },
    {
        "name": "test_sensor",
        "path": "/home/some_file_to_read_v3"
    }]
}
```

Currently the only types implemented with proper parsing are `IOTC_AT_X509` and `IOTC_AT_SYMMETRIC_KEY`. (`IOTC_AT_TOKEN` will probably also work without changing anything, but it needs to be tested).
`sensors` object is always optional, but, if present, it must be a JSON array.
***NOTE:*** spaces in `name` fields for `sensors` array are causing problems at the moment (data is being sent, but doesn’t appear on IoTC dashboard). So, avoid using spaces.