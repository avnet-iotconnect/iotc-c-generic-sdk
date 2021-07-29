//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

#define IOTCONNECT_CPID "your-cpid"
#define IOTCONNECT_ENV  "your-enviroment"

// Device Unique ID
// If using TPM, and this value is a blank string, Registration ID will be used from output of tpm_device_provision. Otherwise, the provide Device Uinque ID will be used.
#define IOTCONNECT_DUID "your-device-unique-id"

// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY

// if using Symmetric Key based authentication, provide the primary or secondary key here:
#define IOTCONNECT_SYMMETRIC_KEY ""

// If using TPM, provide the Scope ID here:
#define IOTCONNECT_SCOPE_ID ""// AKA ID Scope.

#define IOTCONNECT_CERT_PATH "../certs"

// This is the CA Certificate used to validate the IoTHub TLS Connection and it is required for all authentication types.
// Alternatively, you can point this file to /etc/ssl/certs/Baltimore_CyberTrust_Root.pem on some Linux systems
#define IOTCONNECT_SERVER_CERT (IOTCONNECT_CERT_PATH "/server.pem")

// if IOTC_X509 is used:
#define IOTCONNECT_IDENTITY_CERT (IOTCONNECT_CERT_PATH "/client-crt.pem")
#define IOTCONNECT_IDENTITY_KEY (IOTCONNECT_CERT_PATH "/client-key.pem")

#endif //APP_CONFIG_H
