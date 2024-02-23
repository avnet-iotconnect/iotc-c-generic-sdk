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

#include <stdio.h>

/*
 * To use iotconnect need to specify debug routines IOTC_DEBUG, IOTC_WARN and IOTC_ERROR
 *
 * Two example implementations are shown below.
 * Another possible implementation would be to use syslog().
 */
// #define USE_SYSLOG 1
#if USE_SYSLOG
#include <syslog.h>
#define IOTC_ERROR(...) syslog(LOG_ERR, __VA_ARGS__)
#define IOTC_WARN(...) syslog(LOG_WARNING, __VA_ARGS__)
#define IOTC_DEBUG(...) syslog(LOG_DEBUG, __VA_ARGS__)
#else
#define IOTC_ENDLN "\n"

#define IOTC_DEBUG_LEVEL 2

#define IOTC_ERROR(...) fprintf(stderr, __VA_ARGS__);fprintf(stderr, IOTC_ENDLN)
#define IOTC_WARN(...)
#define IOTC_DEBUG(...)

#if IOTC_DEBUG_LEVEL > 0
#undef IOTC_WARN
#define IOTC_WARN(...) fprintf(stderr, __VA_ARGS__);fprintf(stderr, IOTC_ENDLN)
#if IOTC_DEBUG_LEVEL > 1
#undef IOTC_DEBUG
#define IOTC_DEBUG(...) printf(__VA_ARGS__);printf(IOTC_ENDLN)
#endif
#endif

#endif // USE_SYSLOG

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
