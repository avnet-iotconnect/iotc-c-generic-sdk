/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdio.h>
#include <string.h>
#include "iotcl_dra_url.h"
#include "iotcl_dra_identity.h"
#include "iotcl_dra_discovery.h"
#include "iotconnect.h"
#include "iotc_http_request.h"
#include "iotc_device_client.h"

static IotConnectClientConfig config = {0};

static void dump_response(const char *message, IotConnectHttpResponse *response) {
    if (message) {
        IOTC_ERROR("%s", message);
    }

    if (response->data) {
        IOTC_DEBUG(" Response was:\n----\n%s\n----", response->data);
    } else {
        IOTC_WARN(" Response was empty");
    }
}

static int validate_response(IotConnectHttpResponse *response) {
    if (NULL == response->data) {
        dump_response("Unable to parse HTTP response.", response);
        return IOTCL_ERR_PARSING_ERROR;
    }
    const char *json_start = strstr(response->data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", response);
        return IOTCL_ERR_PARSING_ERROR;
    }
    if (json_start != response->data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", response);
    }
    return IOTCL_SUCCESS;
}

static int run_http_identity(IotConnectConnectionType ct, const char *cpid, const char *env, const char* duid) {
    IotclDraUrlContext discovery_url = {0};
    IotclDraUrlContext identity_url = {0};
    int status;
    switch (ct) {
        case IOTC_CT_AWS:
        IOTC_DEBUG("Using AWS discovery URL...");
            status = iotcl_dra_discovery_init_url_aws(&discovery_url, cpid, env);
            break;
        case IOTC_CT_AZURE:
        IOTC_DEBUG("Using Azure discovery URL...");
            status = iotcl_dra_discovery_init_url_azure(&discovery_url, cpid, env);
            break;
        default:
        IOTC_ERROR("Unknown connection type %d\n", ct);
            return IOTCL_ERR_BAD_VALUE;
    }

    if (status) {
        // the called function will check arguments and return error
        return status;
    }

    IotConnectHttpResponse response;
    iotconnect_https_request(&response,
                             iotcl_dra_url_get_url(&discovery_url),
                             NULL
    );
    status = validate_response(&response);
    if (status) {
        goto cleanup; // called function will print the error
    }

    status = iotcl_dra_discovery_parse(&identity_url, 0, response.data);
    if (status) {
        goto cleanup; // called function will print the error
    }
    iotconnect_free_https_response(&response);

    status = iotcl_dra_identity_build_url(&identity_url, duid);
    if (status) {
        goto cleanup; // called function will print the error
    }

    iotconnect_https_request(&response,
                             iotcl_dra_url_get_url(&identity_url),
                             NULL
    );

    status = validate_response(&response);
    if (status) {
        goto cleanup; // called function will print the error
    }

    status = iotcl_dra_identity_configure_library_mqtt(response.data);
    if (status) {
        goto cleanup; // called function will print the error
    }

    if (ct == IOTC_CT_AWS && iotcl_mqtt_get_config()->username) {
        // workaround for identity returning username for AWS.
        // https://awspoc.iotconnect.io/support-info/2024036163515369
        iotcl_free(iotcl_mqtt_get_config()->username);
        iotcl_mqtt_get_config()->username = NULL;
    }

    cleanup:
    iotcl_dra_url_deinit(&discovery_url);
    iotcl_dra_url_deinit(&identity_url);
    iotconnect_free_https_response(&response);
    return status;
}



void iotconnect_sdk_disconnect(void) {
    IOTC_DEBUG("Disconnecting...");
    if (0 == iotc_device_client_disconnect()) {
        IOTC_DEBUG("Disconnected.");
    }
}

bool iotconnect_sdk_is_connected(void) {
    return iotc_device_client_is_connected();
}

IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void) {
    memset(&config, 0, sizeof(config));
    return &config;
}

static void on_mqtt_c2d_message(const unsigned char *message, size_t message_len) {
    if (config.trace_data) {
        IOTC_DEBUG("<: %.*s", (int) message_len, message);
    }
    iotcl_c2d_process_event_with_length(message, message_len);
}

void iotconnect_sdk_mqtt_send_cb(const char *topic, size_t topic_len, const char *json_str) {
    (void) topic_len;
    if (config.trace_data) {
        IOTC_DEBUG(">: %s",  json_str);
    }
    iotc_device_client_send_message_qos(topic, json_str, config.qos);
}

int iotconnect_sdk_init(void) {
    int status;

    if (config.connection_type != IOTC_CT_AWS && config.connection_type != IOTC_CT_AZURE) {
        IOTC_ERROR("Error: Device configuration is invalid. Must set connection type");
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (!config.env || !config.cpid || !config.duid) {
        IOTC_ERROR("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.");
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (config.auth_info.type != IOTC_AT_X509 &&
        config.auth_info.type != IOTC_AT_SYMMETRIC_KEY
            ) {
        IOTC_ERROR("Error: Unsupported authentication type!");
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY && config.connection_type == IOTC_CT_AWS) {
        IOTC_ERROR("Error: Symmetric key authentication is mot supported on AWS!");
        return IOTCL_ERR_CONFIG_ERROR;
    }

    if (!config.auth_info.trust_store) {
        IOTC_ERROR("Error: Configuration server certificate is required.");
        return IOTCL_ERR_CONFIG_MISSING;
    }
    if (config.auth_info.type == IOTC_AT_X509 && (
            !config.auth_info.data.cert_info.device_cert ||
            !config.auth_info.data.cert_info.device_key)) {
        IOTC_ERROR("Error: Configuration authentication info is invalid.");
        return IOTCL_ERR_CONFIG_MISSING;
    } else if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY && (
            !config.auth_info.data.symmetric_key ||
            0 == strlen(config.auth_info.data.symmetric_key))) {
    }

    IotclClientConfig c;
    iotcl_init_client_config(&c);
    c.device.cpid = config.cpid;
    c.device.duid = config.duid;
    c.device.instance_type = IOTCL_DCT_CUSTOM;
    c.mqtt_send_cb = iotconnect_sdk_mqtt_send_cb;
    c.events.cmd_cb = config.cmd_cb;
    c.events.ota_cb = config.ota_cb;

    status = iotcl_init_and_print_config(&c);
    if (status) {
        return status; // called function will print errors
    }

    status = run_http_identity(config.connection_type, config.cpid, config.env, config.duid);
    if (status) {
        iotcl_deinit();
        return status; // called function will print errors
    }

    IOTC_DEBUG("Identity response parsing successful.");

    IotConnectDeviceClientConfig dc;
    dc.qos = config.qos;
    dc.status_cb = config.status_cb;
    dc.c2d_msg_cb = &on_mqtt_c2d_message;
    dc.auth = &config.auth_info;

    status = iotc_device_client_init(&dc);
    if (status) {
        IOTC_ERROR("Failed to connect!");
        return status;
    }

    return status;
}
