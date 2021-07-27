//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

#define IOTCONNECT_CPID "your-cpid" // Account Settings -> Key Vault
#define IOTCONNECT_ENV  "your-environment" // Account Settings -> Key Vault

#define IOTCONNECT_DUID "test-device" // you can supply a custom device UID, or...

// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY

#define IOTCONNECT_SYMMETRIC_KEY "your-symmetric-key" // In device connection info panel

// to use TPM, enable IOTC_TPM_SUPPORT TPM support in cmake first
#define IOTCONNECT_SCOPE_ID "One000XXXXX"// AKA ID Scope

// If executing from the build directory, then certs directory will be relative to it
#define IOTCONNECT_CERT_PATH "../certs"
#define IOTCONNECT_SERVER_CERT (IOTCONNECT_CERT_PATH "/server.pem")

// if IOTC_X509 is used:
#define IOTCONNECT_IDENTITY_CERT (IOTCONNECT_CERT_PATH "/client-crt.pem")
#define IOTCONNECT_IDENTITY_KEY (IOTCONNECT_CERT_PATH "/client-key.pem")


#endif //APP_CONFIG_H
