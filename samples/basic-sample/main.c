//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotconnect_common.h"
#include "iotconnect.h"
#include "cJSON.h"

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
#define STRINGS_ARE_EQUAL 0

typedef struct cert_struct {

    char* x509_id_cert;
    char* x509_id_key;

} cert_struct_t;

typedef struct sensor_info {
    char* s_name;
    char* s_path;
} sensor_info_t;

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

static void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);
    printf("command: %s status=%s: %s\n", command_name, status ? "OK" : "Failed", message);
    printf("Sent CMD ack: %s\n", ack);
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

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        printf("Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
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

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            printf("Command is: %s\n", command);
            message = "Old back end URLS are not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        printf("Sent OTA ack: %s\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}


static void publish_telemetry(sensor_info_t sensor, int reading) {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers
    iotcl_telemetry_set_number(msg, sensor.s_name, reading);
    iotcl_telemetry_set_bool(msg, "is_vlads_test", true);
    iotcl_telemetry_set_string(msg, "my_str", "MY STRING WILL BE DESTROYED");
    iotcl_telemetry_set_null(msg, "my_str");

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}


static int parse_x509_certs(const cJSON* json_parser, cert_struct_t* certs){

    if (!json_parser || !certs){
        printf("NULL PTR.\r\n");
        return 1;
    }

    cJSON *x509_obj = NULL;
    cJSON *x509_id_cert = NULL;
    cJSON *x509_id_key = NULL; 
    // ignoring auth type for now

    x509_obj = cJSON_GetObjectItem(json_parser, "x509_certs");

    if (!x509_obj){
        printf("Failed to get x509 object. Aborting\n");
        return 1;
    }

    printf("auth type: %s\n", x509_obj->valuestring);

    // TODO: add error checking
    x509_id_cert = cJSON_GetObjectItemCaseSensitive(x509_obj, "client_cert");
    x509_id_key = cJSON_GetObjectItemCaseSensitive(x509_obj, "client_key");

    printf("id cert path: {%s}\n", x509_id_cert->valuestring);
    printf("id key path: {%s}\n", x509_id_key->valuestring);


    int key_len = 0;
    key_len = strlen(x509_id_key->valuestring)*(sizeof(char));

    int cert_len = 0;
    cert_len = strlen(x509_id_cert->valuestring)*(sizeof(char));

    printf("cert len: %d, key len: %d:\r\n",cert_len, key_len);

    certs->x509_id_cert = calloc(cert_len, sizeof(char));

    if (!certs->x509_id_cert){
        printf("failed to malloc\r\n");
        certs->x509_id_cert = NULL;
        return 1;
    }


    certs->x509_id_key = calloc(key_len, sizeof(char));


    if (!certs->x509_id_key){
        printf("failed to malloc\r\n");
        certs->x509_id_key = NULL;
        return 1;
    }

    printf("cert: %s\r\n", x509_id_cert->valuestring);
    memcpy(certs->x509_id_cert, x509_id_cert->valuestring, sizeof(char)*cert_len);
    printf("current id key: {%s}\n\r current id cert: {%s}\r\n", certs->x509_id_key, certs->x509_id_cert);

    memcpy(certs->x509_id_key, x509_id_key->valuestring, sizeof(char)*key_len);
    printf("current id key: {%s}\n\r current id cert: {%s}\r\n", certs->x509_id_key, certs->x509_id_cert);
   
    printf("id cert path in struct: {%s}\n", certs->x509_id_cert);
    printf("id key path in struct: {%s}\n", certs->x509_id_key);

    certs->x509_id_cert[cert_len] = '\0';
    certs->x509_id_key[key_len] = '\0';

    return 0;
}

//TODO: add proper error checking
static int parse_sensors(const cJSON* json_parser, sensor_info_t* sensor){

    if (!json_parser || !sensor){
        printf("NULL PTR. Aborting\r\n");
    }

    cJSON *sensor_obj = NULL;

    cJSON *device_name = NULL;
    cJSON *device_path = NULL;

    sensor_obj = cJSON_GetObjectItem(json_parser, "sensor");

    if (!sensor_obj){
        printf("Failed to get sensor object. Aborting\n");
        cJSON_Delete(sensor_obj);
        return 1;
    }


    device_name = cJSON_GetObjectItemCaseSensitive(sensor_obj, "name");
    device_path = cJSON_GetObjectItemCaseSensitive(sensor_obj, "path");    

    int s_name_len = 0;
    s_name_len = strlen(device_name->valuestring)*(sizeof(char));

    int s_path_len = 0;
    s_path_len = strlen(device_path->valuestring)*(sizeof(char));

    printf("path len: %d, name len: %d:\r\n",s_path_len, s_name_len);

    sensor->s_name = calloc(s_name_len, sizeof(char));

    if (!sensor->s_name){
        printf("failed to malloc\r\n");
        sensor->s_name = NULL;
        return 1;
    }


    sensor->s_path = calloc(s_path_len, sizeof(char));


    if (!sensor->s_path){
        printf("failed to malloc\r\n");
        sensor->s_path = NULL;
        return 1;
    }

    memcpy(sensor->s_name, device_name->valuestring, sizeof(char)*s_name_len);
    printf("current name key: {%s}\n\r current path cert: {%s}\r\n", sensor->s_name, sensor->s_path);

    memcpy(sensor->s_path, device_path->valuestring, sizeof(char)*s_path_len);
    printf("current name: {%s}\n\r current path: {%s}\r\n", sensor->s_name, sensor->s_path);

    sensor->s_name[s_name_len] = '\0';
    sensor->s_path[s_path_len] = '\0';

    printf("device name (struct): %s\r\n",sensor->s_name);
    printf("device path (struct): %s\r\n",sensor->s_path); 

    return 0;
}

//TODO: add error checking
static int parse_paramaters_json(const char* json_str, cert_struct_t* certs, sensor_info_t* sensor){

    if (!json_str || !certs){
        printf("NULL PTR. Aborting\n");
        return 1;
    }

    printf("json_str in function: %s\n", json_str);

    cJSON *auth_type = NULL;

    cJSON *json_parser = NULL;


    json_parser = cJSON_Parse(json_str);

    if (!json_parser){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto FAIL;
    }

    auth_type = cJSON_GetObjectItemCaseSensitive(json_parser, "auth_type");

    if (!auth_type) {
        printf("Failed to get auth_type. Aborting\n");
        goto FAIL;
    }

    printf("auth type: %s\n", auth_type->valuestring);

    if (strcmp(auth_type->valuestring, "X509") == STRINGS_ARE_EQUAL && cJSON_HasObjectItem(json_parser, "x509_certs") == true){
        if (parse_x509_certs(json_parser, certs) != 0) {
            printf("failed to parse x509 certs. Aborting\r\n");
            return 1;
        }
    }

    //TODO; maybe rethink this
    if (cJSON_HasObjectItem(json_parser, "sensor") == true){
        if (parse_sensors(json_parser, sensor) != 0){
            printf("failed to parse sensor. Aborting\r\n");
            return 1;
        }
        printf("sensor data: name - %s; path - %s\r\n", sensor->s_name, sensor->s_path);
    }
    


    cJSON_Delete(json_parser);
    return 0;

FAIL:

    cJSON_Delete(json_parser);
    return 1;

}

// TODO: currently will only read first 5 characters from specified file
static int read_sensor(sensor_info_t sensor_data){

    char buff[6];

    if(access(sensor_data.s_path, F_OK) != 0){
        printf("failed to access sensor file - %s ; Aborting\n", sensor_data.s_path);
        return 1;
    }

    FILE* fd = NULL;
    int reading = 0;

        
    fd = fopen(sensor_data.s_path, "r");

    //TODO: magic number
    for (int i = 0; i < 5; i++){
        buff[i] = fgetc(fd);
    }

    buff[5] = '\0';

    fclose(fd);

    reading = (int)atof(buff);

    printf("raw int: %d", reading);

    return reading;
}

int main(int argc, char *argv[]) {
    if (access(IOTCONNECT_SERVER_CERT, F_OK) != 0) {
        fprintf(stderr, "Unable to access IOTCONNECT_SERVER_CERT. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
               IOTCONNECT_SERVER_CERT);
    }

    printf("input counter: %d\n", argc);

    char* input_json_file = NULL;
    cert_struct_t certs;
    sensor_info_t sensor_data;

    char* x509_identity_cert = NULL;
    char* x509_identity_key = NULL;

    if (argc > 1) {
        // assuming only 2 parameters for now
           
        char * s = NULL;
        s = strstr(argv[1], ".json");
        if (s){ 
            printf("Found it in %s\n", argv[1]);

        } else if (!s) {
            printf("String .json not found inside of %s\n", argv[1]);
            return 1;
        }
    
        input_json_file = argv[1];

        printf("file: %s\n", input_json_file);

        if(access(input_json_file, F_OK) != 0){
            printf("failed to access input json file - %s ; Aborting\n", input_json_file);
            return 1;
        }

        FILE *fd = NULL;


        
        fd = fopen(input_json_file, "r");

        fseek(fd, 0l, SEEK_END);
        long file_len = ftell(fd);

        if (file_len <= 0){
            printf("failed calculating file length: %ld. Aborting\n", file_len);
            return 1;
        }

        printf("found length %ld \n", file_len);
        //char *text = NULL;
        rewind(fd);


        char* json_str = (char*)calloc(file_len+1, sizeof(char));

        if (!json_str) {
            printf("failed to calloc. Aborting\n");
            json_str = NULL;
            return 1;
        }

        for (int i = 0; i < file_len; i++){
            json_str[i] = fgetc(fd);
        }
        printf ("end str: \n%s\n", json_str);

        fclose(fd);

        if (parse_paramaters_json(json_str, &certs, &sensor_data) != 0) {
            printf("Failed to parse input JSON file. Aborting\n");
            if (json_str) {
                free(json_str);
                json_str = NULL;
            }
            return 1;
        }


        printf("id cert path in struct IN MAIN: {%s}\n", certs.x509_id_cert);
        printf("id key path in struct IN MAIN: {%s}\n", certs.x509_id_key);

        if(access(certs.x509_id_key, F_OK) != 0){
            printf("failed to access parameter 1 - %s ; Aborting\n", certs.x509_id_key);
            return 1;
        } else {
            x509_identity_key = certs.x509_id_key;
        }
        

        if(access(certs.x509_id_cert, F_OK) != 0){
            printf("failed to access parameter 2 - %s ; Aborting\n", certs.x509_id_cert);
            return 1;
        } else {
            x509_identity_cert = certs.x509_id_cert;
        }

        printf("read sensor device params. name - %s; path - %s\r\n", sensor_data.s_name, sensor_data.s_path);

    } else {
        x509_identity_cert = IOTCONNECT_IDENTITY_CERT;
        x509_identity_key = IOTCONNECT_IDENTITY_KEY;
        printf("Using built-in config parameters.\r\n");
    }

    if (!x509_identity_cert || !x509_identity_key){
        printf("one of the cert paths is NULL\r\n");
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
        config->auth_info.data.cert_info.device_cert = x509_identity_cert;
        config->auth_info.data.cert_info.device_key = x509_identity_key;
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


    int reading = 0;

    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    for (int j = 0; j < 10; j++) {
        int ret = iotconnect_sdk_init();
        if (0 != ret) {
            fprintf(stderr, "IoTConnect exited with error code %d\n", ret);
            return ret;
        }

        // send 10 messages
        for (int i = 0; iotconnect_sdk_is_connected() && i < 10; i++) {
            reading = read_sensor(sensor_data);
            publish_telemetry(sensor_data, reading);
            // repeat approximately evey ~5 seconds
            for (int k = 0; k < 500; k++) {
                iotconnect_sdk_receive();
                usleep(10000); // 10ms
            }
        }
        iotconnect_sdk_disconnect();
    }

    free(certs.x509_id_cert);
    free(certs.x509_id_key);
    free(sensor_data.s_name);
    free(sensor_data.s_path);

    certs.x509_id_cert = NULL;
    certs.x509_id_key = NULL;
    sensor_data.s_name = NULL;
    sensor_data.s_path = NULL;

    return 0;
}
