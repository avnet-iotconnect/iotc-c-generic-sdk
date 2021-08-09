// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 7/16/21.
//

#include <stdlib.h>
#include <string.h>
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/threadapi.h"

#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "iotconnect.h"
#include "iotc_device_client.h"



#ifndef MQTT_PUBLISH_TIMEOUT_MS
#define MQTT_PUBLISH_TIMEOUT_MS     10000L
#endif

#define IOTC_CONNECTION_STRING_FORMAT_KEY  "HostName=%s;DeviceId=%s;SharedAccessKey=%s"
#define IOTC_CONNECTION_STRING_FORMAT_X509 "HostName=%s;DeviceId=%s;x509=true"
#define IOTC_CONNECTION_STRING_FORMAT_SAS_TOKEN "HostName=%s;DeviceId=%s;SharedAccessSignature=%s"

#ifndef IOTC_PROVISIONING_URL
#define IOTC_PROVISIONING_URL "global.azure-devices-provisioning.net"
#endif

typedef struct DPS_CLIENT_INFO_TAG
{
    char* iothub_uri;
    char* device_id;
    int registration_complete;
} DPS_CLIENT_INFO;
DPS_CLIENT_INFO dps_data = {0};

static bool is_client_active = false;
static bool is_iothub_initialized = false;
static bool is_message_confirmed = false;
static IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle = NULL;
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages
static IotConnectStatusCallback status_cb = NULL; // callback for connection connection_status
static IotConnectConnectionStatus connection_status = IOTC_CS_UNDEFINED;

// file_to_string() credit: https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
static char *file_to_string(const char *filename) {
    char *buffer = 0;
    long length;
    FILE *f = fopen(filename, "rb");
    size_t num_read = 0;
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            num_read = fread(buffer, 1, length, f);
        }
        fclose(f);
    }

    if (0 == num_read || num_read != length) {
        fprintf(stderr, "Unable to read device PEM info at %s\n", filename);
        free(buffer);
        return NULL;
    }

    return buffer;
}

static void client_deinit() {
    is_client_active = false;
    if (device_ll_handle) {
        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        ThreadAPI_Sleep(10);
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
        device_ll_handle = NULL;
    }
    free(dps_data.device_id);
    free(dps_data.iothub_uri);
    memset(&dps_data, 0, sizeof(dps_data));
    prov_dev_security_deinit();
    c2d_msg_cb = NULL;
    status_cb = NULL;
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *user_context_callback) {
    (void) user_context_callback;
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result) {
        is_message_confirmed = true;
    } else {
        // When a message is sent this callback will get invoked
        printf("Message confirmation result error: %s\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void *user_context) {
    (void) user_context;
    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY) {
        const unsigned char *buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK) {
            fprintf(stderr, "Error: Unable to extract c2d message from IoTHub.");
        } else {
            if (c2d_msg_cb) {
                c2d_msg_cb((unsigned char *) buff_msg, buff_len);
            }
        }
    } else {
        const char *string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL) {
            fprintf(stderr, "Error: Unable to extract c2d message from IoTHub.");
        }
        if (c2d_msg_cb) {
            c2d_msg_cb((unsigned char *) string_msg, strlen(string_msg));
        }
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

static void
connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                           void *user_context) {
    (void) user_context;

    if (reason == IOTHUB_CLIENT_CONNECTION_OK && result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        connection_status = IOTC_CS_MQTT_CONNECTED;
    } else {
        connection_status = IOTC_CS_MQTT_DISCONNECTED;
    }
    if (status_cb) {
        status_cb(connection_status);
    }
}

int iotc_device_client_disconnect() {
    client_deinit();
    return 0;
}

bool iotc_device_client_is_connected() {
    if (!is_client_active) {
        return false;
    }
    return (connection_status == IOTC_CS_MQTT_CONNECTED);
}


int iotc_device_client_send_message(const char *message) {
    if (connection_status != IOTC_CS_MQTT_CONNECTED) {
        fprintf(stderr, "Error: Failed to send message: %s. Client is not connected.", message);
        return -1;
    }
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);
    is_message_confirmed = false;
    if (IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL) !=
        IOTHUB_MESSAGE_OK) {
        fprintf(stderr, "Error: Failed to send message: %s", message);
    }
    IoTHubMessage_Destroy(message_handle);

    for (int i = 0; i < MQTT_PUBLISH_TIMEOUT_MS / 10 && !is_message_confirmed; i++) {
        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        ThreadAPI_Sleep(10);
    }
    if (is_message_confirmed) {
        is_message_confirmed = true;
        return 0;
    } else {
        fprintf(stderr, "Unable to obtain message confirmation for message %s", message);
        return -1;
    }
}


void iotc_device_client_receive() {
    if (is_iothub_initialized && device_ll_handle) {
        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
    }
}

int iotc_device_client_send_message_qos(const char *message, int qos) {
    fprintf(stderr,
            "WARNING: QOS level and retry policy is currently not supported by the Azure Device SDK implementation");
    return iotc_device_client_send_message(message);
}

static void registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context) {
    (void)user_context;
    switch (reg_status) {
        case PROV_DEVICE_REG_STATUS_CONNECTED:
            printf("Connected to provisioning service...\n");
            break;
        case PROV_DEVICE_REG_STATUS_ASSIGNING:
            printf("Assigning...\n");
            break;
        default:
            printf("Provisioning Status: %d\n", reg_status);
            break;
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    (void)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK) {
        printf("Registration received: URI=%s device_id=%s.\n", iothub_uri, device_id);
        mallocAndStrcpy_s(&dps_data.iothub_uri, iothub_uri);
        mallocAndStrcpy_s(&dps_data.device_id, device_id);
        dps_data.registration_complete = 1;
    } else {
        fprintf(stderr, "Failure encountered during registration. Error code %d\n", register_result);
        dps_data.registration_complete = 2;
    }
}

// returned value needs to be freed
char* iotc_device_client_get_tpm_registration_id() {
    char *ret;
    if (!is_client_active) {
        prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);
    }
    PROV_AUTH_HANDLE security_handle = prov_auth_create();
    ret = prov_auth_get_registration_id(security_handle);
    if (!is_client_active) {
        prov_dev_security_deinit();
    }
    return ret;
}


IOTHUB_DEVICE_CLIENT_LL_HANDLE provision_with_dps(const char* id_scope, const char* registration_id) {
    printf("Provisioning...\n");

    prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);
    PROV_DEVICE_LL_HANDLE prov_handle;


    if ((prov_handle = Prov_Device_LL_Create(IOTC_PROVISIONING_URL, id_scope, Prov_Device_AMQP_Protocol)) == NULL) {
        fprintf(stderr, "Failed calling Prov_Device_LL_Create\n");
        return NULL;
    }
    Prov_Device_LL_SetOption(prov_handle, PROV_REGISTRATION_ID, registration_id);

#ifdef IOTC_PROVISIONING_LOG_TRACE
    // in case one would like to display additional tracing information - for example, thethe server responses
    bool traceOn = true;
    Prov_Device_LL_SetOption(prov_handle, PROV_OPTION_LOG_TRACE, &traceOn);
#endif

    if (Prov_Device_LL_Register_Device(prov_handle, register_device_callback, NULL, registration_status_callback, NULL) != PROV_DEVICE_RESULT_OK) {
        fprintf(stderr, "Failed calling Prov_Device_LL_Register_Device\n");
        return NULL;
    }

    do {
        Prov_Device_LL_DoWork(prov_handle);
        ThreadAPI_Sleep(10);
    } while (dps_data.registration_complete == 0);

    Prov_Device_LL_Destroy(prov_handle);

    if (dps_data.registration_complete != 1) {
        (void)printf("Device registration failed!\n");
    }

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle;
    (void)printf("Creating IoTHub Device handle\n");
    if ((device_handle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(dps_data.iothub_uri, dps_data.device_id, MQTT_Protocol) ) == NULL) {
        fprintf(stderr, "Failed create IoTHub with provisioned URI %s and device ID %s!\n", dps_data.iothub_uri, dps_data.device_id);
    }
    free(dps_data.iothub_uri);
    dps_data.iothub_uri = NULL;
    free(dps_data.device_id);
    dps_data.device_id = NULL;
    return device_handle;
}

int iotc_device_client_init(IotConnectDeviceClientConfig *c) {

    // Used to initialize IoTHub SDK subsystem
    if (!is_iothub_initialized) {
        is_iothub_initialized = true;
        (void) IoTHub_Init();
    }

    client_deinit(); // reset all locals

    char *connection_string_buffer = NULL;

    switch (c->auth->type) {
        case IOTC_AT_TOKEN:
            if (NULL == c->sr->broker.pass || strlen(c->sr->broker.pass) == 0) {
                fprintf(stderr, "Sync response did not return a SAS token. Is your device auth type set to Token?\n");
                return -1;
            }
            connection_string_buffer = malloc(sizeof(IOTC_CONNECTION_STRING_FORMAT_KEY)
                                              + strlen(c->sr->broker.host)
                                              + strlen(c->sr->broker.client_id)
                                              + strlen(c->sr->broker.pass)
            );
            sprintf(connection_string_buffer, IOTC_CONNECTION_STRING_FORMAT_SAS_TOKEN,
                    c->sr->broker.host,
                    c->sr->broker.client_id,
                    c->sr->broker.pass
            );
            break;

        case IOTC_AT_X509:
            connection_string_buffer = malloc(sizeof(IOTC_CONNECTION_STRING_FORMAT_X509)
                                              + strlen(c->sr->broker.host)
                                              + strlen(c->sr->broker.client_id)
            );
            sprintf(connection_string_buffer, IOTC_CONNECTION_STRING_FORMAT_X509,
                    c->sr->broker.host,
                    c->sr->broker.client_id
            );
            break;
        case IOTC_AT_TPM:
            break;
        case IOTC_AT_SYMMETRIC_KEY:
            if (NULL == c->auth->data.symmetric_key || strlen(c->auth->data.symmetric_key) == 0) {
                fprintf(stderr, "Symmetric key is required\n");
                return -1;
            }
            connection_string_buffer = malloc(sizeof(IOTC_CONNECTION_STRING_FORMAT_KEY)
                                              + strlen(c->sr->broker.host)
                                              + strlen(c->sr->broker.client_id)
                                              + strlen(c->auth->data.symmetric_key)
            );
            sprintf(connection_string_buffer, IOTC_CONNECTION_STRING_FORMAT_KEY,
                    c->sr->broker.host,
                    c->sr->broker.client_id,
                    c->auth->data.symmetric_key
            );
            break;
        default:
            fprintf(stderr, "Unknown authentication type\n");
            return -1;
    }

    if (c->auth->type == IOTC_AT_TPM) {
        device_ll_handle = provision_with_dps(c->auth->data.scope_id, c->sr->broker.client_id);
    } else {
        device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connection_string_buffer, MQTT_Protocol);
    }

    if (device_ll_handle == NULL) {
        fprintf(stderr, "Failure creating IotHub device client.\n");
        return -1;
    }

    if (c->auth->type == IOTC_AT_X509) {
        char *device_cert = file_to_string(c->auth->data.cert_info.device_cert);
        char *device_key = file_to_string(c->auth->data.cert_info.device_key);
        if (
                (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_CERT,
                                                 device_cert) != IOTHUB_CLIENT_OK) ||
                (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_X509_PRIVATE_KEY,
                                                 device_key) != IOTHUB_CLIENT_OK)
                ) {
            fprintf(stderr, "Failed to set options for x509, aborting\n");
            client_deinit();
            free(device_cert);
            free(device_key);
            return -1;
        }
        free(device_cert);
        free(device_key);
    }

    if (IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle, receive_msg_callback, NULL) != IOTHUB_CLIENT_OK) {
        fprintf(stderr, "Failed to set IoTHub client message callback\n");
        return -1;
    }
    if (IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL) !=
        IOTHUB_CLIENT_OK) {
        fprintf(stderr, "Failed to set IoTHub connection connection_status callback\n");
        return -1;
    }
    connection_status = IOTC_CS_UNDEFINED;

    for (int i = 0; i < 6000; i++) {
        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        ThreadAPI_Sleep(10);
        if (connection_status != IOTC_CS_UNDEFINED) {
            break;
        }
    }

    is_client_active = true;

    c2d_msg_cb = c->c2d_msg_cb;

    if (status_cb) {
        status_cb(connection_status);
    }

    return connection_status == IOTC_CS_MQTT_CONNECTED ? 0 : -1;
}

