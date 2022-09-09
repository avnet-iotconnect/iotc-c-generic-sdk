//
// THIS HEADER FILE IS PRIVATE TO THE SDK AND SHOULD NOT BE USED OUTSIDE OF THE SDK.
//

//
// Copyright: Avnet, Softweb Inc. 2020
// Based on work by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#ifndef IOTCONNECT_PRIV_H
#define IOTCONNECT_PRIV_H

#include "iotconnect.h"

#ifdef __cplusplus
extern "C" {
#endif

// Encapsulate private definitions of IotConnectAuthInfo and IotConnectClientConfig inside SDK
typedef struct {
    IotConnectAuthType type;
    char* trust_store; // Path to a file containing the trust certificates for the remote MQTT host
    union {
        struct {
            char* device_cert; // Path to a file containing the device CA cert (or chain) in PEM format
            char* device_key; // Path to a file containing the device private key in PEM format
        } cert_info;
        char *symmetric_key;
        char *scope_id; // for TPM authentication. AKA: ID Scope
    } data;
} IotConnectAuthInfo;

typedef struct {
    char *env;    // Settings -> Key Vault -> Environment.
    char *cpid;   // Settings -> Key Vault -> CPID.
    char *duid;   // Name of the device.
    int qos; // QOS for outbound messages. Default 1.
    IotConnectAuthInfo auth_info;
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectClientConfig;

#ifdef __cplusplus
}
#endif

#endif
