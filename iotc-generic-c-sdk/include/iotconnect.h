//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include <stddef.h>
#include <time.h>
#include "iotconnect_event.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Probably unused??
#if 0
///////////////////////////////////////////////////////////////////////////////////
// you need to pass cpid , env and the HOST at GET_TEMPLATE
// This templates can be used for raw HTTP headers in case that the platform doesn't GET/POST functionality
#define IOTCONNECT_DISCOVERY_HEADER_TEMPLATE \
    "GET /api/sdk/cpid/%s/lang/M_C/ver/2.0/env/%s HTTP/1.1\r\n" \
    "Host: " IOTCONNECT_DISCOVERY_HOSTNAME "\r\n" \
    "Connection: close\r\n" \
    "\r\n"

///////////////////////////////////////////////////////////////////////////////
// This templates can be used for raw HTTP headers in case that the platform doesn't GET/POST functionality
// you need to pass URL returned from discovery host ,host form discovery host, post_data_lan and post_data
#define IOTCONNECT_SYNC_HEADER_TEMPLATE \
    "POST %s/sync? HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Content-Type: application/json; charset=utf-8\r\n" \
    "Connection: close\r\n" \
    "Content-length: %d\r\n" \
    "\r\n" \
    "%s"
#endif

// You will typically use this JSON post data to get mqtt client information
#define IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE "{\"cpId\":\"%s\",\"uniqueId\":\"%s\",\"option\":{\"attribute\":false,\"setting\":false,\"protocol\":true,\"device\":false,\"sdkConfig\":false,\"rule\":false}}"

// add 1 for string terminator
#define IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN (\
    sizeof(IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE) + \
    CONFIG_IOTCONNECT_DUID_MAX_LEN + CONFIG_IOTCONNECT_CPID_MAX_LEN \
    )

#define DEFAULT_IOTCONNECT_DISCOVERY_HOSTNAME "discovery.iotconnect.io"

typedef enum {
    // Authentication based on your CPID. Sync HTTP endpoint returns a long lived SAS token
    // This auth type is only intended as a simple way to connect your test and development devices
    // and must not be used in production
    IOTC_AT_TOKEN = 1,

    // CA Cert and Self Signed Cert
    IOTC_AT_X509 = 2,

    // TPM hardware devices
    IOTC_AT_TPM = 4, // 4 for compatibility with sync

    // IoTHub Key based authentication with Symmetric Keys (Primary or Secondary key)
    IOTC_AT_SYMMETRIC_KEY = 5

} IotConnectAuthType;

typedef enum {
    IOTC_CS_UNDEFINED,
    IOTC_CS_MQTT_CONNECTED,
    IOTC_CS_MQTT_DISCONNECTED
} IotConnectConnectionStatus;

typedef void (*IotConnectStatusCallback)(IotConnectConnectionStatus data);

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
    const char *discovery_host;    // Settings -> Key Vault -> Discovery Host.
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


IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void);

// call iotconnect_sdk_init_and_get_config first and configure the SDK before calling iotconnect_sdk_init()
int iotconnect_sdk_init(void);

bool iotconnect_sdk_is_connected(void);

// Will check if there are inbound messages and call adequate callbacks if there are any
// This is technically not required for the Paho implementation.
void iotconnect_sdk_receive(void);

// blocks until sent and returns 0 if successful.
// data is a null-terminated string
int iotconnect_sdk_send_packet(const char *data);

void iotconnect_sdk_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif
