/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotcl.h"
#include "iotconnect.h"

#include "app_config.h"

// windows compatibility
#if defined(_WIN32) || defined(_WIN64)
#define F_OK 0
#include <Windows.h>
#include <io.h>
int usleep(unsigned long usec) {
    Sleep(usec / 1000);
    return 0;
}
#define access    _access_s
#else
#include <unistd.h>
#endif

#define APP_VERSION "00.01.00"

static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            printf("IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            printf("IoTConnect Client Disconnected\n");
            break;
        default:
            printf("IoTConnect Client ERROR\n");
            break;
    }
}

static void on_command(IotclC2dEventData data) {
    const char *command = iotcl_c2d_get_command(data);
    const char *ack_id = iotcl_c2d_get_ack_id(data);
    if (command) {
        printf("Command %s received with %s ACK ID\n", command, ack_id ? ack_id : "no");
        // could be a command without acknowledgement, so ackID can be null
        if (ack_id) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Not implemented");
        }
    } else {
        // could be a command without acknowledgement, so ackID can be null
        printf("Failed to parse command\n");
        if (ack_id) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
        }
    }
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

// This sample OTA handling only checks the version and verifies if the firmware needs an update but does not download.
static void on_ota(IotclC2dEventData data) {
    const char *message = NULL;
    const char *url = iotcl_c2d_get_ota_url(data, 0);
    const char *ack_id = iotcl_c2d_get_ack_id(data);
    bool success = false;
    if (NULL != url) {
        printf("Download URL is: %s\n", url);
        const char *version = iotcl_c2d_get_ota_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            printf("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            printf("OTA update is required for version %s.\n", version);
            success = false;
            message = "Not implemented";
        } else {
            printf("Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }
    }

    iotcl_mqtt_send_ota_ack(ack_id, (success ? IOTCL_C2D_EVT_OTA_SUCCESS : IOTCL_C2D_EVT_OTA_DOWNLOAD_FAILED), message);
}

static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // STRING template field type
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);

    // INTEGER template field type
    int random_int =  (int) ((double) rand() / RAND_MAX * 10.0); // ger an integer from 0 to 9 first
    iotcl_telemetry_set_number(msg, "random_int", (double) random_int);

    // DECIMAL template field type
    iotcl_telemetry_set_number(msg, "random_decimal", (double) rand() / RAND_MAX);

    // BOOLEAN template field type
    iotcl_telemetry_set_bool(msg, "random_boolean", ((double) rand() / RAND_MAX) > 0.5 ? true: false);

    // OBJECT template field type with two nested DECIMAL values
    iotcl_telemetry_set_number(msg, "coordinate.x", (double) rand() / RAND_MAX * 10.0);
    iotcl_telemetry_set_number(msg, "coordinate.y", (double) rand() / RAND_MAX * 10.0);

    iotcl_mqtt_send_telemetry(msg, false);
    iotcl_telemetry_destroy(msg);
}

int main(int argc, char *argv[]) {
    char *trust_store;

    (void) argc;
    (void) argv;

#ifdef IOTCONNECT_MQTT_SERVER_CA_CERT
    trust_store = IOTCONNECT_CA_CERT_PATH
#else
    if (IOTCONNECT_CONNECTION_TYPE == IOTC_CT_AWS) {
        trust_store = IOTCONNECT_MQTT_SERVER_CA_CERT_DEFAULT_AWS;
    } else {
        trust_store = IOTCONNECT_MQTT_SERVER_CA_CERT_DEFAULT_AZURE;
    }
#endif

    if (access(trust_store, F_OK) != 0) {
        printf("Unable to access the MQTT CA certificate. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH",
               trust_store);
        return -1;
    }

    if (IOTCONNECT_AUTH_TYPE == IOTC_AT_X509) {
        if (access(IOTCONNECT_DEVICE_CERT, F_OK) != 0
                ) {
            printf("Unable to access device identity certificate. "
                   "Please change directory so that %s can be accessed from the application or update IOTCONNECT_DEVICE_CERT",
                   IOTCONNECT_DEVICE_CERT);
            return -1;
        }
        if (access(IOTCONNECT_DEVICE_PRIVATE_KEY, F_OK) != 0
                ) {
            printf("Unable to access device identity private key. "
                   "Please change directory so that %s can be accessed from the application or update IOTCONNECT_DEVICE_PRIVATE_KEY",
                   IOTCONNECT_DEVICE_PRIVATE_KEY);
            return -1;
        }
    }

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    config->cpid = IOTCONNECT_CPID;
    config->env = IOTCONNECT_ENV;
    config->duid = IOTCONNECT_DUID;
    config->connection_type = IOTCONNECT_CONNECTION_TYPE;
    config->auth_info.type = IOTCONNECT_AUTH_TYPE;
    config->auth_info.trust_store = trust_store;
    config->trace_data = true;

    if (config->auth_info.type == IOTC_AT_X509) {
        config->auth_info.data.cert_info.device_cert = IOTCONNECT_DEVICE_CERT;
        config->auth_info.data.cert_info.device_key = IOTCONNECT_DEVICE_PRIVATE_KEY;
    } else if (config->auth_info.type == IOTC_AT_SYMMETRIC_KEY){
        config->auth_info.data.symmetric_key = IOTCONNECT_SYMMETRIC_KEY;
    } else {
        // none of the above
        printf("Unknown IotConnectAuthType\n");
        return -1;
    }


    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;

    // initialize random seed for the telemetry test
    srand((unsigned int) time(NULL));

    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    for (int j = 0; j < 10; j++) {
        int ret = iotconnect_sdk_init();
        if (0 != ret) {
            printf("iotconnect_sdk_init() exited with error code %d\n", ret);
            return ret;
        }

        // send 10 messages
        for (int i = 0; iotconnect_sdk_is_connected() && i < 10; i++) {
            publish_telemetry();
            // repeat evey ~5 seconds
            usleep(5000000);
        }
        iotconnect_sdk_disconnect();
    }

    printf("Basic sample demo is complete. Exiting.\n");
    return 0;
}
