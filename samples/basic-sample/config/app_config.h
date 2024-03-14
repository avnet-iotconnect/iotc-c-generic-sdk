/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

// from iotconnect.h IotConnectConnectionType
#define IOTCONNECT_CONNECTION_TYPE IOTC_CT_AZURE

#define IOTCONNECT_CPID "avtds"
#define IOTCONNECT_ENV  "Avnet"
#define IOTCONNECT_DUID "nik-deleteme"

#if 0
#define IOTCONNECT_CPID "your-cpid"
#define IOTCONNECT_ENV  "your-enviroment"

// Device Unique ID
// If using TPM, and this value is a blank string, Registration ID will be used from output of tpm_device_provision. Otherwise, the provide Device Uinque ID will be used.
#define IOTCONNECT_DUID "your-device-unique-id"
#endif
// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_X509

// if using Symmetric Key based authentication, provide the primary or secondary key here:
#define IOTCONNECT_SYMMETRIC_KEY ""

// The server CA Certificate used to validate the Azure IoTHub or AWS IoT Core TLS Connection
// and it is required for all authentication types:
// #define IOTCONNECT_MQTT_SERVER_CA_CERT
// or use else we will use defaults below...:
// default AWS cert for RSA cert/key. Use CA3 or others where appropriate:
#define IOTCONNECT_MQTT_SERVER_CA_CERT_DEFAULT_AWS "../../../lib/iotc-c-lib/tools/cert-files/AmazonRootCA1.pem"
// default Azure cert if using IOTC_CT_AZURE
#define IOTCONNECT_MQTT_SERVER_CA_CERT_DEFAULT_AZURE "../../../lib/iotc-c-lib/tools/cert-files/DigiCertGlobalRootG2.pem"

// if IOTC_X509 is used reference your device identity cert and private key here
#define IOTCONNECT_DEVICE_CERT ("../identity/client-crt.pem")
#define IOTCONNECT_DEVICE_PRIVATE_KEY ("../identity/client-key.pem")

#endif //APP_CONFIG_H
