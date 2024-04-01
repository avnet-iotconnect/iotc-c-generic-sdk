/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdio.h>
#include <string.h>
#include "iotcl_util.h"
#include "iotcl_dra_url.h"
#include "iotcl_dra_identity.h"
#include "iotcl_dra_discovery.h"
#include "iotc_log.h"
#include "iotc_http_request.h"
#include "iotc_device_client.h"
#include "iotconnect.h"

static IotConnectClientConfig config = {0};
static bool is_config_valid = false;

static int iotconnect_clone_client_config(IotConnectClientConfig* c) {
    bool oom_error = false;
    memcpy(&config, c, sizeof(IotConnectClientConfig));
    config.cpid = iotcl_strdup(c->cpid);
    config.env = iotcl_strdup(c->env);
    config.duid = iotcl_strdup(c->duid);
    config.auth_info.trust_store = iotcl_strdup(c->auth_info.trust_store);

    if (!config.cpid && c->cpid) oom_error = true;
    if (!config.env && c->env) oom_error = true;
    if (!config.duid && c->duid) oom_error = true;
    if (!config.auth_info.trust_store && c->auth_info.trust_store) { oom_error = true; }

    if (c->auth_info.type == IOTC_AT_X509) {
        config.auth_info.data.cert_info.device_cert = iotcl_strdup(c->auth_info.data.cert_info.device_cert);
        config.auth_info.data.cert_info.device_key = iotcl_strdup(c->auth_info.data.cert_info.device_key);
        if (!config.auth_info.data.cert_info.device_cert && c->auth_info.data.cert_info.device_cert) {
            oom_error = true;
        }
        if (!config.auth_info.data.cert_info.device_key && c->auth_info.data.cert_info.device_key) {
            oom_error = true;
        }
    } else if (c->auth_info.type == IOTC_AT_SYMMETRIC_KEY) {
        config.auth_info.data.symmetric_key = iotcl_strdup(c->auth_info.data.symmetric_key);
        if (!config.auth_info.data.symmetric_key && c->auth_info.data.symmetric_key) {
            oom_error = true;
        }
    }

    if (oom_error) {
        IOTC_ERROR("Out of memory while cloning config!");
        return IOTCL_ERR_OUT_OF_MEMORY;
    }
    
    return 0;
}

static void dump_response(const char *message, IotConnectHttpResponse *response) {
    if (message) {
        IOTC_ERROR("%s", message);
    }

    if (response->data) {
        IOTC_INFO(" Response was:\n----\n%s\n----", response->data);
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
        IOTC_INFO("Using AWS discovery URL...");
            status = iotcl_dra_discovery_init_url_aws(&discovery_url, cpid, env);
            break;
        case IOTC_CT_AZURE:
        IOTC_INFO("Using Azure discovery URL...");
            status = iotcl_dra_discovery_init_url_azure(&discovery_url, cpid, env);
            break;
        default:
        IOTC_ERROR("Unknown connection type %d\n", ct);
            return IOTCL_ERR_BAD_VALUE;
    }

    if (status) {
        return status; // called function will print the error
    }

    IotConnectHttpResponse response;
    iotconnect_https_request(&response,
                             iotcl_dra_url_get_url(&discovery_url),
                             NULL
    );
    status = validate_response(&response);
    if (status) goto cleanup; // called function will print the error


    status = iotcl_dra_discovery_parse(&identity_url, 0, response.data);
    if (status) {
        IOTC_ERROR("Error while parsing discovery response from %s", iotcl_dra_url_get_url(&discovery_url));
        dump_response(NULL, &response);
        goto cleanup;
    }

    iotconnect_free_https_response(&response);

    status = iotcl_dra_identity_build_url(&identity_url, duid);
    if (status) goto cleanup; // called function will print the error

    iotconnect_https_request(&response,
                             iotcl_dra_url_get_url(&identity_url),
                             NULL
    );

    status = validate_response(&response);
    if (status) goto cleanup; // called function will print the error

    status = iotcl_dra_identity_configure_library_mqtt(response.data);
    if (status) {
        IOTC_ERROR("Error while parsing identity response from %s", iotcl_dra_url_get_url(&identity_url));
        dump_response(NULL, &response);
        goto cleanup;
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

bool iotconnect_sdk_is_connected(void) {
    return iotc_device_client_is_connected();
}

void iotconnect_sdk_init_config(IotConnectClientConfig *c) {
    memset(c, 0, sizeof(IotConnectClientConfig));
    c->qos = 1;
}

static void on_mqtt_c2d_message(const unsigned char *message, size_t message_len) {
    if (config.verbose) {
        IOTC_INFO("<: %.*s", (int) message_len, message);
    }
    iotcl_c2d_process_event_with_length(message, message_len);
}

void iotconnect_sdk_mqtt_send_cb(const char *topic, const char *json_str) {
    if (config.verbose) {
        IOTC_INFO(">: %s",  json_str);
    }
    iotc_device_client_send_message_qos(topic, json_str, config.qos);
}

int iotconnect_sdk_init(IotConnectClientConfig *c) {
    int status;

    // clear existing global config
    iotconnect_sdk_deinit();

    if (iotconnect_clone_client_config(c)) {
        iotconnect_sdk_deinit();
        return IOTCL_ERR_OUT_OF_MEMORY; // called function will print the error
    }

    if (config.connection_type != IOTC_CT_AWS && config.connection_type != IOTC_CT_AZURE) {
        IOTC_ERROR("Error: Device configuration is invalid. Must set connection type");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (!config.env || !config.cpid || !config.duid) {
        IOTC_ERROR("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (config.auth_info.type != IOTC_AT_X509 &&
        config.auth_info.type != IOTC_AT_SYMMETRIC_KEY
            ) {
        IOTC_ERROR("Error: Unsupported authentication type!");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_MISSING_VALUE;
    }
    if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY && config.connection_type == IOTC_CT_AWS) {
        IOTC_ERROR("Error: Symmetric key authentication is mot supported on AWS!");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_CONFIG_ERROR;
    }

    if (!config.auth_info.trust_store) {
        IOTC_ERROR("Error: Configuration server certificate is required.");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_CONFIG_MISSING;
    }
    if (config.auth_info.type == IOTC_AT_X509 && (
            !config.auth_info.data.cert_info.device_cert ||
            !config.auth_info.data.cert_info.device_key)) {
        IOTC_ERROR("Error: Configuration authentication info is invalid.");
        iotconnect_sdk_deinit();
        return IOTCL_ERR_CONFIG_MISSING;
    } else if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY && (
            !config.auth_info.data.symmetric_key ||
            0 == strlen(config.auth_info.data.symmetric_key))) {
    }

    IotclClientConfig iotcl_cfg;
    iotcl_init_client_config(&iotcl_cfg);
    iotcl_cfg.device.cpid = config.cpid;
    iotcl_cfg.device.duid = config.duid;
    iotcl_cfg.device.instance_type = IOTCL_DCT_CUSTOM;
    iotcl_cfg.mqtt_send_cb = iotconnect_sdk_mqtt_send_cb;
    iotcl_cfg.events.cmd_cb = config.cmd_cb;
    iotcl_cfg.events.ota_cb = config.ota_cb;

    if (c->verbose) {
        status = iotcl_init_and_print_config(&iotcl_cfg);
    } else {
        status = iotcl_init(&iotcl_cfg);
    }
    if (status) {
        iotconnect_sdk_deinit();
        return status; // called function will print errors
    }

    status = run_http_identity(config.connection_type, config.cpid, config.env, config.duid);
    if (status) {
        iotcl_deinit();
        return status; // called function will print errors
    }

    IOTC_INFO("Identity response parsing successful.");
    is_config_valid = true;
    return status;
}

int iotconnect_sdk_connect(void) {
    if (!is_config_valid) {
        IOTC_ERROR("iotconnect_sdk_connect called, but config is invalid!");
        return IOTCL_ERR_CONFIG_MISSING;
    }
    IotConnectDeviceClientConfig dc;
    dc.qos = config.qos;
    dc.status_cb = config.status_cb;
    dc.c2d_msg_cb = &on_mqtt_c2d_message;
    dc.auth = &config.auth_info;

    int status = iotc_device_client_connect(&dc);
    if (status) {
        IOTC_ERROR("Failed to connect!");
        iotconnect_sdk_deinit();
        return status;
    }
    return 0;
}

void iotconnect_sdk_disconnect(void) {
    IOTC_INFO("Disconnecting...");
    if (0 == iotc_device_client_disconnect()) {
        IOTC_INFO("Disconnected.");
    }
}

void iotconnect_sdk_deinit() {

    iotcl_deinit();

    is_config_valid = false;
    if (config.cpid) iotcl_free(config.cpid);
    if (config.env) iotcl_free(config.env);
    if (config.duid) iotcl_free(config.duid);

    if (config.auth_info.trust_store) iotcl_free(config.auth_info.trust_store);

    if (config.auth_info.type == IOTC_AT_X509) {
        if (config.auth_info.data.cert_info.device_cert) iotcl_free(config.auth_info.data.cert_info.device_cert);
        if (config.auth_info.data.cert_info.device_key) iotcl_free(config.auth_info.data.cert_info.device_key);
    } else if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY) {
        if (config.auth_info.data.symmetric_key) iotcl_free(config.auth_info.data.symmetric_key);
    }
    memset(&config, 0, sizeof(IotConnectClientConfig));
}