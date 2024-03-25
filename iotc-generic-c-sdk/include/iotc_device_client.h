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

int iotc_device_client_connect(IotConnectDeviceClientConfig *c);

int iotc_device_client_disconnect(void);

bool iotc_device_client_is_connected(void);

// Sends the message with the underlying MQTT client (Paho) with configured default QOS and returns the error if
// sending or confirming (acknowledging) the message fails. The error will be client-specific.
int iotc_device_client_send_message(const char* topic, const char *message);

// Same as iotc_device_client_send_message() with with specified qos
int iotc_device_client_send_message_qos(const char* topic, const char *message, int qos);

void iotc_device_client_receive(void);

#ifdef __cplusplus
}
#endif

#endif // IOTC_DEVICE_CLIENT_H
