//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

#if 0
#define IOTCONNECT_CPID "avtds"
#define IOTCONNECT_ENV  "avnetpoc"

// Device Unique ID
// If using TPM, and this value is a blank string, Registration ID will be used from output of tpm_device_provision. Otherwise, the provide Device Uinque ID will be used.
#define IOTCONNECT_DUID "selfsignedv1"
#else
#define IOTCONNECT_CPID  "97FF86E8728645E9B89F7B07977E4B15"
#define IOTCONNECT_ENV "poc"

#define IOTCONNECT_DISCOVERY_HOSTNAME "awsdiscovery.iotconnect.io"

// Device Unique ID
// If using TPM, and this value is a blank string, Registration ID will be used from output of tpm_device_provision. Otherwise, the provide Device Uinque ID will be used.
#define IOTCONNECT_DUID "ssigned21"
#endif

// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_X509

// if using Symmetric Key based authentication, provide the primary or secondary key here:
#define IOTCONNECT_SYMMETRIC_KEY ""

// If using TPM, provide the Scope ID here:
#define IOTCONNECT_SCOPE_ID ""// AKA ID Scope.

#define IOTCONNECT_CERT_PATH "../certs/"

// This is the CA Certificate used to validate the IoTHub TLS Connection and it is required for all authentication types.
// Alternatively, you can point this file to /etc/ssl/certs/Baltimore_CyberTrust_Root.pem on some Linux systems
#define IOTCONNECT_SERVER_CERT (IOTCONNECT_CERT_PATH "AmazonRootCA1.pem")

// if IOTC_X509 is used:
#define IOTCONNECT_IDENTITY_CERT (IOTCONNECT_CERT_PATH "cert_ssigned21.crt")
#define IOTCONNECT_IDENTITY_KEY (IOTCONNECT_CERT_PATH "pk_ssigned21.pem")

#endif //APP_CONFIG_H
