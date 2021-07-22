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


typedef enum {
    IOTC_AT_KEY = 1,
    IOTC_AT_X509 = 2,
    IOTC_AT_TPM = 4 // 4 for compatibility with sync
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
    char *env;    // Settings -> Key Vault -> CPID.
    char *cpid;   // Settings -> Key Vault -> Evnironment.
    char *duid;   // Name of the device.
    int qos; // QOS for outbound messages. Default 1.
    IotConnectAuthInfo auth_info;
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectClientConfig;


IotConnectClientConfig *iotconnect_sdk_init_and_get_config();

int iotconnect_sdk_init();

bool iotconnect_sdk_is_connected();

IotclConfig *iotconnect_sdk_get_lib_config();

int iotconnect_sdk_send_packet(const char *data);

void iotconnect_sdk_disconnect();

#ifdef __cplusplus
}
#endif

#endif
