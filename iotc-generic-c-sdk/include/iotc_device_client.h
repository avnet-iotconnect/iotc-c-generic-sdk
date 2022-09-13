//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#ifndef IOTC_DEVICE_CLIENT_H
#define IOTC_DEVICE_CLIENT_H

#include "iotconnect_discovery.h"
#include "iotconnect.h"

#ifdef __cplusplus
extern   "C" {
#endif


typedef void (*IotConnectC2dCallback)(const unsigned char* message, size_t message_len);

typedef struct {
    int qos; // default QOS is 1
    IotclSyncResponse* sr;
    IotConnectAuthInfo *auth; // Pointer to IoTConnect auth configuration
    IotConnectC2dCallback c2d_msg_cb; // callback for inbound messages
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectDeviceClientConfig;

int iotc_device_client_init(IotConnectDeviceClientConfig *c);

int iotc_device_client_disconnect(void);

bool iotc_device_client_is_connected(void);

// sends message with QOS 1
// If called wthin an Azure C SDK callback (like on_message), the the message confirmation cannot be established
// and the return code will be non-zero
// returns:
// -1 if message confirmation timed out
// -2 if message confirmation cannot be established
// -3 if the client is not connected

int iotc_device_client_send_message(const char *message);

// sends message with specified qos
int iotc_device_client_send_message_qos(const char *message, int qos);

void iotc_device_client_receive(void);

// Returns the TPM registration ID from the TPM chip.
// Not supported in Paho Client. It will always return null.
// The returned value MUST be freed.
char* iotc_device_client_get_tpm_registration_id(void);

#ifdef __cplusplus
}
#endif

#endif // IOTC_DEVICE_CLIENT_H
