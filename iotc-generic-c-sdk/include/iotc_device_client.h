/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_DEVICE_CLIENT_H
#define IOTC_DEVICE_CLIENT_H

#include "iotconnect.h"

#ifdef __cplusplus
extern   "C" {
#endif


typedef void (*IotConnectC2dCallback)(const unsigned char* message, size_t message_len);

typedef struct {
    int qos; // default QOS is 1
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

int iotc_device_client_send_message(const char* topic, const char *message);

// sends message with specified qos
int iotc_device_client_send_message_qos(const char* topic, const char *message, int qos);

void iotc_device_client_receive(void);

#ifdef __cplusplus
}
#endif

#endif // IOTC_DEVICE_CLIENT_H
