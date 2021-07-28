//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

#define IOTCONNECT_CPID "avtds"
#define IOTCONNECT_ENV  "Avnet"

#define IOTCONNECT_DUID "IGS01SGW01" // you can supply a custom device UID, or...

// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY

#define IOTCONNECT_SYMMETRIC_KEY "kn3Uo3+Z0Uy4tzINRsWobQ0ZGYvRpWEqyLY+SjS77YE="

// to use TPM, enable IOTC_TPM_SUPPORT TPM support in cmake first
#define IOTCONNECT_SCOPE_ID "One0005A911"// AKA ID Scope.

#define IOTCONNECT_CERT_PATH "../certs"

// This is the CA Certificate used to validate the IoTHub TLS Connection and it is required for all authentication types.
// Alternatively, you can point this file to /etc/ssl/certs/Baltimore_CyberTrust_Root.pem on some Linux systems
#define IOTCONNECT_SERVER_CERT (IOTCONNECT_CERT_PATH "/server.pem")

// if IOTC_X509 is used:
#define IOTCONNECT_IDENTITY_CERT (IOTCONNECT_CERT_PATH "/client-crt.pem")
#define IOTCONNECT_IDENTITY_KEY (IOTCONNECT_CERT_PATH "/client-key.pem")

#endif //APP_CONFIG_H
