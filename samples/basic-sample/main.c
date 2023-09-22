//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotconnect_common.h"
#include "iotconnect.h"

#include <curl/curl.h>

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

typedef struct local_data {

    bool reboot_needed;
    int ret_code;

} local_data_t;

static local_data_t local_data;

static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            fprintf(stdout, "IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            fprintf(stdout, "IoTConnect Client Disconnected\n");
            break;
        default:
            fprintf(stdout, "IoTConnect Client ERROR\n");
            break;
    }
}

static void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);
    fprintf(stdout, "command: %s status=%s: %s\n", command_name, status ? "OK" : "Failed", message);
    fprintf(stdout, "Sent CMD ack: %s\n", ack);
    iotconnect_sdk_send_packet(ack);
    free((void *) ack);
}

static void on_command(IotclEventData data) {
    char *command = iotcl_clone_command(data);
    if (NULL != command) {
        command_status(data, false, command, "Not implemented");
        free((void *) command);
    } else {
        command_status(data, false, "?", "Internal error");
    }
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

static int ota_handle(char *url){

    CURL* handle;
    FILE* fd;

    fd = fopen("/home/basic-sample-b", "w");

    if (!fd){
        printf("fd NULL\r\n");
        return 1;
    }

    printf("url: %s\r\n", url);

    handle = curl_easy_init();

    printf("past init\r\n");

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)fd);

    CURLcode success = 0;

    printf("before perform\r\n");

    success = curl_easy_perform(handle);

    printf("ret code: %d\r\n", success);

    curl_easy_cleanup(handle);

    return success;
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        fprintf(stdout, "Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            fprintf(stdout, "OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            fprintf(stdout, "OTA update is required for version %s.\n", version);
            success = false;
            if (ota_handle(url) == CURLE_OK){
                printf("Successfully downloaded new binary!\r\n");
                message = "OK";
                local_data.reboot_needed = true;
                local_data.ret_code = 5;
            } else {
                message = "Failed";
            }
            
        } else {
            fprintf(stdout, "Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            fprintf(stdout, "Command is: %s\n", command);
            message = "Old back end URLS are not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        fprintf(stdout, "Sent OTA ack: %s\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}


static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    fprintf(stdout, "Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}


int main(int argc, char *argv[]) {
    
    setvbuf(stdout, NULL, _IONBF, 0);
    
    local_data.reboot_needed = false;

    if (access(IOTCONNECT_SERVER_CERT, F_OK) != 0) {
        fprintf(stderr, "Unable to access IOTCONNECT_SERVER_CERT. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
               IOTCONNECT_SERVER_CERT);
    }

    if (IOTCONNECT_AUTH_TYPE == IOTC_AT_X509) {
        if (access(IOTCONNECT_IDENTITY_CERT, F_OK) != 0 ||
            access(IOTCONNECT_IDENTITY_KEY, F_OK) != 0
                ) {
            fprintf(stderr, "Unable to access device identity private key and certificate. "
                   "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
                   IOTCONNECT_SERVER_CERT);
        }
    }

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    config->cpid = IOTCONNECT_CPID;
    config->env = IOTCONNECT_ENV;
    config->duid = IOTCONNECT_DUID;
    config->auth_info.type = IOTCONNECT_AUTH_TYPE;
    config->auth_info.trust_store = IOTCONNECT_SERVER_CERT;

    if (config->auth_info.type == IOTC_AT_X509) {
        config->auth_info.data.cert_info.device_cert = IOTCONNECT_IDENTITY_CERT;
        config->auth_info.data.cert_info.device_key = IOTCONNECT_IDENTITY_KEY;
    } else if (config->auth_info.type == IOTC_AT_TPM) {
        config->auth_info.data.scope_id = IOTCONNECT_SCOPE_ID;
    } else if (config->auth_info.type == IOTC_AT_SYMMETRIC_KEY){
        config->auth_info.data.symmetric_key = IOTCONNECT_SYMMETRIC_KEY;
    } else if (config->auth_info.type != IOTC_AT_TOKEN) { // token type does not need any secret or info
        // none of the above
        fprintf(stderr, "IOTCONNECT_AUTH_TYPE is invalid\n");
        return -1;
    }


    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;


    int ret = iotconnect_sdk_init();
    if (0 != ret) {
        fprintf(stderr, "IoTConnect exited with error code %d\n", ret);
        return ret;
    }    

    local_data.ret_code = 0;

    // run infinitely if connection is successful
    while (1){
        while (iotconnect_sdk_is_connected()){
            publish_telemetry();
            // repeat approximately evey ~5 seconds
            for (int k = 0; k < 500; k++) {
                iotconnect_sdk_receive();
                usleep(10000); // 10ms
            }
            
        }

        ret = iotconnect_sdk_init();
        if (0 != ret) {
            fprintf(stderr, "IoTConnect exited with error code %d\n", ret);
            return ret;
        }    
        printf("app version: %s\r\n", APP_VERSION);
            if (local_data.reboot_needed){
                break;
            }
        usleep(100000); // 1s
    }



    return local_data.ret_code;
}
