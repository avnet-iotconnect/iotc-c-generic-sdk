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
    const char* trust_store; // Path to a file containing the trust certificates for the remote MQTT host
    union {
        struct {
            const char* device_cert; // Path to a file containing the device CA cert (or chain) in PEM format
            const char* device_key; // Path to a file containing the device private key in PEM format
        } cert_info;
        const char *symmetric_key;
        const char *scope_id; // for TPM authentication. AKA: ID Scope
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
    bool dynamic_memory;
} IotConnectClientConfig;


IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void);
IotConnectClientConfig *iotconnect_sdk_init_and_alloc_config(char *env, char *cpid, char *duid, int authentication_type, char *symmetric_key);

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
void iotconnect_sdk_reset_config(void);

void iotconnect_command_status(IotConnectEventType type, const char *ack_id, bool status, const char *message);
void iotconnect_ota_status(const char *ack_id, bool status, const char *message);

#ifdef __cplusplus
}
#endif

#endif
