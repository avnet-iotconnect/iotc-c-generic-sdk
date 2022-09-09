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

void iotconnect_sdk_set_auth_type(IotConnectAuthType type);
void iotconnect_sdk_set_auth_trust_store(char *trust_store_filename);
void iotconnect_sdk_set_auth_device_cert(char *device_cert_filename);
void iotconnect_sdk_set_auth_device_key(char *device_key_filename);
void iotconnect_sdk_set_auth_symmetric_key(char *symmetric_key);
void iotconnect_sdk_set_auth_scope_id(char *scope_id);

void iotconnect_sdk_set_config_env(char *env);
void iotconnect_sdk_set_config_cpid(char *cpid);
void iotconnect_sdk_set_config_duid(char *duid);
void iotconnect_sdk_set_config_qos(int qos);
void iotconnect_sdk_set_config_ota_cb(IotclOtaCallback ota_cb);
void iotconnect_sdk_set_config_cmd_cb(IotclCommandCallback cmd_cb);
void iotconnect_sdk_set_config_msg_cb(IotclMessageCallback msg_cb);
void iotconnect_sdk_set_config_status_cb(IotConnectStatusCallback status_cb);

void iotconnect_sdk_dump_configuration(void);

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
