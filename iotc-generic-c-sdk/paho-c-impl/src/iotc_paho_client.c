/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "iotc_log.h"
#include "iotc_algorithms.h"
#include "iotconnect.h"
#include "iotc_device_client.h"

#define HOST_URL_FORMAT "ssl://%s:8883"

#ifndef MQTT_PUBLISH_TIMEOUT_MS
#define MQTT_PUBLISH_TIMEOUT_MS     10000L
#endif

static bool is_initialized = false;
static MQTTClient client = NULL;
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages
static IotConnectMqttStatusCallback status_cb = NULL; // callback for connection status
static bool is_in_async_callback = false;

static void paho_deinit(void) {
    if (client) {
        MQTTClient_destroy(&client);
        client = NULL;
    }
    c2d_msg_cb = NULL;
    status_cb = NULL;
}

static int on_c2d_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    (void) context;
    (void) topicLen;

    if (c2d_msg_cb) {
        is_in_async_callback = true;
        c2d_msg_cb(message->payload, (size_t) message->payloadlen);
        is_in_async_callback = false;
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void on_connection_lost(void *context, char *cause) {
    (void) context;

    IOTC_INFO("MQTT Connection lost. Cause: %s", cause);

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_DISCONNECTED);
    }
    paho_deinit();
}

int iotc_device_client_disconnect(void) {
    int rc;
    is_initialized = false;
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to disconnect, return code %d", rc);
    }
    paho_deinit();
    return rc;
}

bool iotc_device_client_is_connected(void) {
    if (!is_initialized) {
        return false;
    }
    return MQTTClient_isConnected(client);
}

int iotc_device_client_send_message_qos(const char* topic, const char *message, int qos) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    pubmsg.payload = (void *) message;
    pubmsg.payloadlen = (int) strlen(message);
    if (is_in_async_callback) {
        // TODO: If we send messages while in async callback with QOS1,
        // message sending hangs and says that sending has failed, but it actually succeeds on the back end
        // We may need to be queueing messages or use the "synchronous" paho mode with polling.
        pubmsg.qos = 0;
    } else {
        pubmsg.qos = qos;
    }
    pubmsg.retained = 0;
    if ((rc = MQTTClient_publishMessage(client, topic, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to publish message, return code %d", rc);
        return rc;
    }

    rc = MQTTClient_waitForCompletion(client, token, MQTT_PUBLISH_TIMEOUT_MS);
    if (status_cb) {
        if (0 == rc) {
            status_cb(IOTC_CS_MQTT_DELIVERED);
        } else {
            status_cb(IOTC_CS_MQTT_SEND_FAILED);
        }
    }
    //IOTC_INFO("Message with delivery token %d delivered", token);
    return rc;
}

int iotc_device_client_send_message(const char* topic, const char *message) {
    return iotc_device_client_send_message_qos(topic, message, 1);
}

int iotc_device_client_connect(IotConnectDeviceClientConfig *c) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    char * password = NULL;
    int rc;

    IotclMqttConfig *mc = iotcl_mqtt_get_config();
    if (!mc) {
        return IOTCL_ERR_CONFIG_MISSING; // caled function will print the error
    }


    paho_deinit(); // reset all locals

    char *paho_host_url = malloc((size_t) snprintf(NULL, 0, HOST_URL_FORMAT, mc->host) + 1);
    if (NULL == paho_host_url) {
        IOTC_ERROR("ERROR: Unable to allocate memory for paho host URL!");
        return -1;
    }
    sprintf(paho_host_url, HOST_URL_FORMAT, mc->host);

    if ((rc = MQTTClient_create(&client, paho_host_url, mc->client_id,
                                MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to create client, return code %d", rc);
        free(paho_host_url);
        return rc;
    }
    free(paho_host_url);

    if ((rc = MQTTClient_setCallbacks(client, NULL, on_connection_lost, on_c2d_message, NULL)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to set callbacks, return code %d", rc);
        paho_deinit();
        return rc;
    }

    ssl_opts.verify = 1;
    ssl_opts.trustStore = c->auth->trust_store;
    if (c->auth->type == IOTC_AT_X509) {
        ssl_opts.keyStore = c->auth->data.cert_info.device_cert;
        ssl_opts.privateKey = c->auth->data.cert_info.device_key;
    } else if (c->auth->type  == IOTC_AT_SYMMETRIC_KEY) {
        if (c->auth->data.symmetric_key && strlen(c->auth->data.symmetric_key) > 0) {
            // for paho we need to pass the generated sas token
            char *sas_token = gen_sas_token(mc->host,
                                            mc->client_id,
                                            c->auth->data.symmetric_key,
                                            60
            );
            if (!sas_token) {
                IOTC_ERROR("Unable to generate SAS token!");
                return IOTCL_ERR_FAILED; // could be OOM or a different reason
            }
            // a bit of a hack - the token will be freed when freeing the sync response
            // paho will use the SAS token as the broker password
            password = sas_token;
        } else {
            IOTC_ERROR("Error: Configuration symmetric key is missing.");
            return -1;
        }
    }
    conn_opts.ssl = &ssl_opts;

    status_cb = c->status_cb;
    conn_opts.username = iotcl_mqtt_get_config()->username;
    conn_opts.password = password;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to connect, return code %d", rc);
        paho_deinit();
        free(password);
        return rc;
    }
    free(password);

    is_initialized = true; // even if we fail below, we are ok

    if ((rc = MQTTClient_subscribe(client, mc->sub_c2d, 1)) != MQTTCLIENT_SUCCESS) {
        IOTC_ERROR("Failed to subscribe to c2d topic, return code %d", rc);
        rc = IOTCL_ERR_FAILED;
    }
    c2d_msg_cb = c->c2d_msg_cb;

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_CONNECTED);
    }

    return IOTCL_SUCCESS;
}



