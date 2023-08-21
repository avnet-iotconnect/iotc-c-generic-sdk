//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotconnect_common.h"
#include "iotconnect_discovery.h"
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
            IOTC_DEBUG("IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            IOTC_DEBUG("IoTConnect Client Disconnected\n");
            break;
        default:
            IOTC_DEBUG("IoTConnect Client ERROR\n");
            break;
    }
}

static void on_command(IotclEventData data) {
    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        IOTC_ERROR("%s: ack_Id  == NULL\n", __func__);
        goto cleanup;
    }

    char *command = iotcl_clone_command(data);
    if (NULL != command) {
        iotconnect_command_status(type, ack_id, false, "Internal error");
        free((void *) command);
    } else {
        IOTC_ERROR("%s: command == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");
    }

    free((void *) ack_id);

cleanup:
    iotcl_destroy_event(data);
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

static void on_ota(IotclEventData data) {
    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        IOTC_ERROR("%s: ack_Id  == NULL\n", __func__);
        goto cleanup;
    }

    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        IOTC_DEBUG("Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            IOTC_DEBUG("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            IOTC_DEBUG("OTA update is required for version %s.\n", version);
            success = false;
            message = "Not implemented";
        } else {
            IOTC_DEBUG("Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        iotconnect_command_status(type, ack_id, success, message);

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            IOTC_DEBUG("Command is: %s\n", command);
            message = "Old back end URLS are not supported by the app";

           iotconnect_command_status(type, ack_id, success, message);

            free((void *) command);
        } else {
            IOTC_ERROR("%s: command  == NULL\n", __func__);
            iotconnect_command_status(type, ack_id, false, "Internal error");
        }
    }

cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}


static void publish_telemetry() {
    const char *str = NULL;

    char *string_names[] = { "version" };
    char *string_values[] = { APP_VERSION };

    char *number_names[] = { "cpu" };
    double number_values[] = { 3.123 };

    str = iotcl_serialise_telemetry_strings(1, string_names, string_values,
                                            1, number_names, number_values,
                                            0, NULL, NULL,
                                            0, NULL);
    if(str == NULL) {
        IOTC_ERROR("iotcl_serialise_telemetry_strings() failed\n");
        return;
    }

    IOTC_DEBUG("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}


int main(int argc, char *argv[]) {
    if (access(IOTCONNECT_SERVER_CERT, F_OK) != 0) {
        IOTC_ERROR("Unable to access IOTCONNECT_SERVER_CERT. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
               IOTCONNECT_SERVER_CERT);
    }

    if (IOTCONNECT_AUTH_TYPE == IOTC_AT_X509) {
        if (access(IOTCONNECT_IDENTITY_CERT, F_OK) != 0 ||
            access(IOTCONNECT_IDENTITY_KEY, F_OK) != 0
                ) {
            IOTC_ERROR("Unable to access device identity private key and certificate. "
                   "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
                   IOTCONNECT_SERVER_CERT);
        }
    }

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    if(config == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
        return -1;
    }

    config->cpid = IOTCONNECT_CPID;
    config->env = IOTCONNECT_ENV;
    config->duid = IOTCONNECT_DUID;
    config->auth_info.type = IOTCONNECT_AUTH_TYPE;
    config->auth_info.trust_store = IOTCONNECT_SERVER_CERT;
    if (!config->auth_info.trust_store) {
        IOTC_ERROR("Error: Configuration server certificate is required.\n");
        return -1;
    }

    IOTC_DEBUG("IOTC_ENV = %s\n", config->env);
    IOTC_DEBUG("IOTC_CPID = %s\n", config->cpid);
    IOTC_DEBUG("IOTC_DUID = %s\n", config->duid);
    IOTC_DEBUG("IOTC_AUTH_TYPE = %d\n", config->auth_info.type);
    IOTC_DEBUG("IOTC_AUTH_SYMMETRIC_KEY = %s\n", config->auth_info.data.symmetric_key ? config->auth_info.data.symmetric_key : "(null)");

    if (config->auth_info.type == IOTC_AT_X509) {
        config->auth_info.data.cert_info.device_cert = IOTCONNECT_IDENTITY_CERT;
        config->auth_info.data.cert_info.device_key = IOTCONNECT_IDENTITY_KEY;

        if (!config->auth_info.data.cert_info.device_cert ||
            !config->auth_info.data.cert_info.device_key) {
            IOTC_ERROR("Error: device authentication info is missing.\n");
            return -1;
        }

    } else if (config->auth_info.type == IOTC_AT_TPM) {
        IOTC_WARN("IOTC_AT_TPM may not be implemented (in hardware)\n");
        config->auth_info.data.scope_id = IOTCONNECT_SCOPE_ID;
    } else if (config->auth_info.type == IOTC_AT_SYMMETRIC_KEY){
        config->auth_info.data.symmetric_key = IOTCONNECT_SYMMETRIC_KEY;
    } else if (config->auth_info.type == IOTC_AT_TOKEN) {
        // token type does not need any secret or info
    } else {
        // none of the above
        IOTC_ERROR("IOTCONNECT_AUTH_TYPE is invalid\n");
        return -1;
    }


    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;


    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    for (int j = 0; j < 10; j++) {
        int ret = iotconnect_sdk_init();
        if (0 != ret) {
            IOTC_ERROR("iotconnect_sdk_init() exited with error code %d\n", ret);
            return ret;
        }
        IOTC_DEBUG("<- iotconnect_sdk_init() %d\n", j);

        // send 10 messages
        for (int i = 0; i < 10; i++) {
            if  (!iotconnect_sdk_is_connected()) {
                IOTC_ERROR("iotconnect_sdk_is_connected() returned false\n");
                break;
            }

            publish_telemetry();
            // repeat approximately evey ~5 seconds
            for (int k = 0; k < 500; k++) {
                iotconnect_sdk_receive();
                usleep(10000); // 10ms
            }
        }
        iotconnect_sdk_disconnect();
    }

    config = NULL;

    IOTC_DEBUG("exiting basic_sample()\n");
    return 0;
}
