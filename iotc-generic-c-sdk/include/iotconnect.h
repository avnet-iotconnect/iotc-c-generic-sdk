/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */


#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include <stddef.h>
#include <time.h>
#include "iotcl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/*
 * To use iotconnect need to specify debug routines IOTC_INFO, IOTC_WARN and IOTC_ERROR
 *
 * Two example implementations are shown below.
 * Another possible implementation would be to use syslog().
 */


typedef enum {
    IOTC_CT_AWS = 1,
    IOTC_CT_AZURE
} IotConnectConnectionType;

typedef enum {
    // CA Cert and Self Signed Cert
    IOTC_AT_X509 = 1,
    // IoTHub Key based authentication with Symmetric Keys (Primary or Secondary key)
    IOTC_AT_SYMMETRIC_KEY
} IotConnectAuthType;

typedef enum {
    IOTC_CS_MQTT_CONNECTED = 1,
    IOTC_CS_MQTT_DISCONNECTED,
    IOTC_CS_MQTT_DELIVERED, // Message delivered(qos=0) or acknowledged (qos>0)
    IOTC_CS_MQTT_SEND_FAILED // // Message message delivery failed (qos=0) or not acknowledged (qos>0)
} IotConnectMqttStatus;

typedef void (*IotConnectMqttStatusCallback)(IotConnectMqttStatus data);

typedef struct {
    IotConnectAuthType type;
    char* trust_store; // Path to a file containing the trust certificates for the remote MQTT host
    union {
        struct {
            char* device_cert; // Path to a file containing the device CA cert (or chain) in PEM format
            char* device_key; // Path to a file containing the device private key in PEM format
        } cert_info;
        char *symmetric_key;
    } data;
} IotConnectAuthInfo;

typedef struct {
    IotConnectConnectionType connection_type;
    char *env;    // Settings -> Key Vault -> CPID.
    char *cpid;   // Settings -> Key Vault -> Environment.
    char *duid;   // Name of the device.
    int qos; // QOS for outbound messages. Default 1.
    IotConnectAuthInfo auth_info;
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotConnectMqttStatusCallback status_cb; // callback for connection status
    bool verbose; // If true, we will output extra info and sent and received MQTT json data to standard out
} IotConnectClientConfig;


void iotconnect_sdk_init_config(IotConnectClientConfig * c);

// call iotconnect_sdk_init_config first and configure the SDK before calling iotconnect_sdk_init()
// NOTE: the client does not need to keep references to the struct or any values inside it
int iotconnect_sdk_init(IotConnectClientConfig * c);

int iotconnect_sdk_connect(void);

bool iotconnect_sdk_is_connected(void);

void iotconnect_sdk_disconnect(void);

void iotconnect_sdk_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
