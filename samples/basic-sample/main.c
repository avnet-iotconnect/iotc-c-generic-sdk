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
    int reading;
} sensor_info_t;

typedef struct sensors_data {

    __uint8_t size;
    sensor_info_t *sensor;

} sensors_data_t;

typedef enum command_type
{
    ECHO = 1,
    LED = 2,
    COMMANDS_END
} command_type_t;

const char *command_strings[] = {
    [ECHO] = "echo ",
    [LED] = "led ",
};

#define DOES_COMMAND_MATCH(input_str, command_enum) (strncmp((input_str), command_strings[(command_enum)], strlen(command_strings[(command_enum)])) == STRINGS_ARE_EQUAL)

// MOVE THIS LATER -afk
static void publish_message(const char* key_str,const char* value_str);

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

static int get_command_type(const char* command_str) {

    int command = 0;

    if (DOES_COMMAND_MATCH(command_str, ECHO)){
        command = ECHO;
    } else if (DOES_COMMAND_MATCH(command_str, LED)){
        command = LED;
    } else {
        printf("Unknown command\r\n");
    }

    return command;

}

static int command_led(const char* command_str){

    char* command_str_copy = strdup(command_str);
    char* token = NULL;

    token = strtok(command_str_copy, " ");

    int res = 0;

    while(token){
        printf("token: %s\r\n", token);

        FILE *fd = NULL;

        fd = fopen("/sys/class/leds/usr_led/brightness", "w");

        if (!fd) {
            printf("failed to open file.\r\n");
            if (command_str_copy)
            {
                free(command_str_copy);
                command_str_copy = NULL;
            }
            return 1;
        }

        res = fputs(token, fd);

        printf("res: %d\r\n", res);

        fclose(fd);
        if (res == EOF){
            printf("failed to write. aborting\r\n");
            if (command_str_copy)
            {
                free(command_str_copy);
                command_str_copy = NULL;
            }
            return 1;
        }
        usleep(3000000); // 3s
        token = strtok(NULL, " ");
    }

    if (command_str_copy)
    {
        free(command_str_copy);
        command_str_copy = NULL;
    }
    return 0;

}

static void command_status(IotclEventData data, const char *command_name) {

    int command_type = 0;

    bool success = false;

    if (strcmp(command_name, "Internal error") == STRINGS_ARE_EQUAL){
        printf("Internal error (null ptr command)\r\n");
        goto END;
    }

    command_type = get_command_type(command_name);

    switch (command_type)
    {
    case ECHO:
        printf("%s\r\n", &command_name[strlen(command_strings[ECHO])]);
        publish_message("last_command", command_name);
        success = true;
        break;
    case LED:
        printf("LED\r\n");
        if (command_led(command_name) != 0)
        {
            printf("failed to parse LED command\r\n");
        }
        else
        {
            success = true;
        }
        break;
    default:
        success = false;
        printf("Unsupported command\r\n");
        break;
    }

END:

    const char* message = success ? "OK" : "Failed_or_not_implemented";

    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    printf("command: %s status=%s: %s\n", command_name, success ? "OK" : "Failed", message);
    printf("Sent CMD ack: %s\n", ack);
    iotconnect_sdk_send_packet(ack);
    free((void *)ack);
}

static void on_command(IotclEventData data) {
    char *command = iotcl_clone_command(data);
    if (NULL != command) {
        command_status(data, command);
        free((void *) command);
    } else {
        command_status(data, "Internal error");
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


static void publish_telemetry(sensors_data_t sensors) {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers
    
    for (int i = 0; i < sensors.size; i++){
        iotcl_telemetry_set_number(msg, sensors.sensor[i].s_name, sensors.sensor[i].reading);
    }

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}


static void publish_message(const char* key_str,const char* value_str) {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, key_str, value_str);

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}


static int parse_x509_certs(const cJSON* json_parser, char** id_key, char** id_cert){

    if (!json_parser){
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

    *id_cert = calloc(cert_len, sizeof(char));

    if (!*id_cert){
        printf("failed to malloc\r\n");
        *id_cert = NULL;
        return 1;
    }


    *id_key = calloc(key_len, sizeof(char));


    if (!*id_key){
        printf("failed to malloc\r\n");
        *id_key = NULL;
        return 1;
    }


    memcpy(*id_cert, x509_id_cert->valuestring, sizeof(char)*cert_len);

    memcpy(*id_key, x509_id_key->valuestring, sizeof(char)*key_len);


    id_cert[cert_len] = '\0';
    id_key[key_len] = '\0';

    return 0;
}

//TODO: add proper error checking
static int parse_sensors(const cJSON* json_parser, sensors_data_t* sensors){

    if (!json_parser || !sensors){
        printf("NULL PTR. Aborting\r\n");
    }

    cJSON *sensor_obj = NULL;

    cJSON *device_name = NULL;
    cJSON *device_path = NULL;

    cJSON *json_array_item = NULL;

    sensor_obj = cJSON_GetObjectItem(json_parser, "sensors");

    if (!sensor_obj){
        printf("Failed to get sensor object. Aborting\n");
        cJSON_Delete(sensor_obj);
        return 1;
    }

    sensors->size = cJSON_GetArraySize(sensor_obj);

    if (sensors->size <= 0) {
        printf("Failed to get array size\r\n");
        return 1;
    }

    sensors->sensor = calloc(sensors->size, sizeof(sensor_info_t));

    if (!sensors->sensor){
        printf("failed to allocate\r\n");
        return 1;
    }

    for (int i = 0; i < sensors->size; i++){

        json_array_item = cJSON_GetArrayItem(sensor_obj, i);

        if (!json_array_item){
            printf("Failed to access element %d of json sensor array\r\n", i);
            return 1;
        }

        device_name = cJSON_GetObjectItemCaseSensitive(json_array_item, "name");
        device_path = cJSON_GetObjectItemCaseSensitive(json_array_item, "path");    

        int s_name_len = 0;
        s_name_len = strlen(device_name->valuestring)*(sizeof(char));

        int s_path_len = 0;
        s_path_len = strlen(device_path->valuestring)*(sizeof(char));

        sensors->sensor[i].s_name = calloc(s_name_len, sizeof(char));

        if (!sensors->sensor[i].s_name){
            printf("failed to malloc\r\n");
            sensors->sensor[i].s_name = NULL;
            return 1;
        }


        sensors->sensor[i].s_path = calloc(s_path_len, sizeof(char));


        if (!sensors->sensor[i].s_path){
            printf("failed to malloc\r\n");
            sensors->sensor[i].s_path = NULL;
            return 1;
        }

        memcpy(sensors->sensor[i].s_name, device_name->valuestring, sizeof(char)*s_name_len);

        memcpy(sensors->sensor[i].s_path, device_path->valuestring, sizeof(char)*s_path_len);

        sensors->sensor[i].s_name[s_name_len] = '\0';
        sensors->sensor[i].s_path[s_path_len] = '\0';

    }

    return 0;
}

static int parse_base_params(char** dest, char* json_src, cJSON* json_parser){
    if (!json_parser){
        printf("NULL PTR. Aborting\r\n");
        return 1;
    }

    cJSON* req_json_str = NULL;

    req_json_str = cJSON_GetObjectItemCaseSensitive(json_parser, json_src);

    if (!req_json_str) {
        printf("Failed to get %s from json. Aborting\n\r", json_src);
        return 1;
    }
    int str_len = 0;
    str_len = strlen(req_json_str->valuestring)*(sizeof(char));

    //printf("str len: %d:\r\n",str_len);

    *dest = calloc(str_len+1, sizeof(char));

    if (!*dest){
        printf("failed to malloc\r\n");
        *dest = NULL;
        return 1;
    }

    memcpy(*dest, req_json_str->valuestring, sizeof(char)*str_len);
    //printf("copied: %s\r\n", *dest);
    dest[str_len] = '\0';

    return 0;
}

static void free_sensor_data(sensors_data_t *sensors) {

    printf("freeing sensor data\r\n");

    for (int i = 0; i < sensors->size; i++){
        if (sensors->sensor[i].s_path){
            free(sensors->sensor[i].s_path);
            sensors->sensor[i].s_path = NULL;
        }

        if (sensors->sensor[i].s_path){
            free(sensors->sensor[i].s_path);
            sensors->sensor[i].s_path = NULL;
        }


    }

    if (sensors->sensor){
        free(sensors->sensor);
        sensors->sensor = NULL;
    }

}

static void free_iotc_config(IotConnectClientConfig* iotc_config) {
    
    printf("freeing iotconnect conf\r\n");

    if (iotc_config->cpid){
        free(iotc_config->cpid);
        iotc_config->cpid = NULL;
    }


    if (iotc_config->duid){
        free(iotc_config->duid);
        iotc_config->duid = NULL;
    }


    if (iotc_config->env){
        free(iotc_config->env);
        iotc_config->env = NULL;
    }

    if (iotc_config->auth_info.trust_store){
        free(iotc_config->auth_info.trust_store);
        iotc_config->auth_info.trust_store = NULL;
    }


    if (iotc_config->auth_info.data.cert_info.device_cert){
        free(iotc_config->auth_info.data.cert_info.device_cert);
        iotc_config->auth_info.data.cert_info.device_cert = NULL;
    }


    if (iotc_config->auth_info.data.cert_info.device_key){
        free(iotc_config->auth_info.data.cert_info.device_key);
        iotc_config->auth_info.data.cert_info.device_key = NULL;
    }


    if (iotc_config->auth_info.data.symmetric_key){
        free(iotc_config->auth_info.data.symmetric_key);
        iotc_config->auth_info.data.symmetric_key = NULL;
    }


}

//TODO: add error checking
static int parse_paramaters_json(const char* json_str, IotConnectClientConfig* iotc_config, sensors_data_t* sensors){

    if (!json_str){
        printf("NULL PTR. Aborting\n");
        return 1;
    }

    cJSON *json_parser = NULL;

    cJSON *auth_type = NULL;


    json_parser = cJSON_Parse(json_str);

    if (!json_parser){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto FAIL;
    }

    if (parse_base_params(&iotc_config->duid, "duid", json_parser) != 0){
        printf("Failed to get duid from json file. Aborting.\r\n");
        goto FAIL;
    }

    if (parse_base_params(&iotc_config->cpid, "cpid", json_parser) != 0){
        printf("Failed to get duid from json file. Aborting.\r\n");
        goto FAIL;
    }

    if (parse_base_params(&iotc_config->env, "env", json_parser) != 0){
        printf("Failed to get duid from json file. Aborting.\r\n");
        goto FAIL;
    }

    if (parse_base_params(&iotc_config->auth_info.trust_store, "iotc_server_cert", json_parser) != 0){
        printf("Failed to get iotc_server_cert from json file. Aborting.\r\n");
        goto FAIL;
    }
    
    auth_type = cJSON_GetObjectItemCaseSensitive(json_parser, "auth_type");

    if (!auth_type) {
        printf("Failed to get auth_type. Aborting\n");
        goto FAIL;
    }

    printf("auth type: %s\n", auth_type->valuestring);

    if (strcmp(auth_type->valuestring, "IOTC_AT_X509") == STRINGS_ARE_EQUAL){
        iotc_config->auth_info.type = IOTC_AT_X509;
    } else if (strcmp(auth_type->valuestring, "IOTC_AT_SYMMETRIC_KEY") == STRINGS_ARE_EQUAL){
        iotc_config->auth_info.type = IOTC_AT_SYMMETRIC_KEY;
    } else if (strcmp(auth_type->valuestring, "IOTC_AT_TPM") == STRINGS_ARE_EQUAL) {
        iotc_config->auth_info.type = IOTC_AT_TPM;
    } else if (strcmp(auth_type->valuestring, "IOTC_AT_TOKEN") == STRINGS_ARE_EQUAL) {
        iotc_config->auth_info.type = IOTC_AT_TOKEN;
    } else {
        printf("unsupported auth type. Aborting\r\n");
        goto FAIL;
    }

    if (iotc_config->auth_info.type == IOTC_AT_X509 && cJSON_HasObjectItem(json_parser, "x509_certs") == true){
        if (parse_x509_certs(json_parser, &iotc_config->auth_info.data.cert_info.device_key,&iotc_config->auth_info.data.cert_info.device_cert) != 0) {
            printf("failed to parse x509 certs. Aborting\r\n");
            return 1;
        }
        

    } else if (iotc_config->auth_info.type == IOTC_AT_SYMMETRIC_KEY && cJSON_HasObjectItem(json_parser, "symmkey") == true){
        if (parse_base_params(&iotc_config->auth_info.data.symmetric_key, "symmkey", json_parser) != 0){
            printf("Failed to get duid from json file. Aborting.\r\n");
            goto FAIL;
        }
        printf("SYMMKEY: %s\r\n", iotc_config->auth_info.data.symmetric_key);
    } else {
        //TODO: placeholder for other auth types
    }

    //TODO; maybe rethink this
    if (cJSON_HasObjectItem(json_parser, "sensors") == true){
        if (parse_sensors(json_parser, sensors) != 0){
            printf("failed to parse sensor. Aborting\r\n");
            goto FAIL;
        }
        //printf("sensor data: name - %s; path - %s\r\n", sensor->s_name, sensor->s_path);
    }
    


    cJSON_Delete(json_parser);
    return 0;

FAIL:

    free_iotc_config(iotc_config);
    free_sensor_data(sensors);

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

    return reading;
}

int main(int argc, char *argv[]) {
    if (access(IOTCONNECT_SERVER_CERT, F_OK) != 0) {
        fprintf(stderr, "Unable to access IOTCONNECT_SERVER_CERT. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
               IOTCONNECT_SERVER_CERT);
    }

    char* input_json_file = NULL;

    sensors_data_t sensors;
    sensors.size = 0;

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    // leaving this non-modifiable or now
    // 

    if (argc == 2) {
        // assuming only 1 parameters for now
           
        char * s = NULL;
        s = strstr(argv[1], ".json");
        if (!s) {
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
        //printf ("end str: \n%s\n", json_str);

        fclose(fd);

        if (parse_paramaters_json(json_str, config, &sensors) != 0) {
            printf("Failed to parse input JSON file. Aborting\n");
            if (json_str) {
                free(json_str);
                json_str = NULL;
            }
            return 1;
        }

        printf("DUID in main: %s\r\n", config->duid);

        

        if (config->auth_info.type == IOTC_AT_X509){
            printf("id cert path: {%s}\n", config->auth_info.data.cert_info.device_cert);
            printf("id key path: {%s}\n", config->auth_info.data.cert_info.device_key);
            if(access(config->auth_info.data.cert_info.device_cert, F_OK) != 0){
                printf("failed to access parameter 1 - %s ; Aborting\n", config->auth_info.data.cert_info.device_cert);
                return 1;
            }
            

            if(access(config->auth_info.data.cert_info.device_key, F_OK) != 0){
                printf("failed to access parameter 2 - %s ; Aborting\n", config->auth_info.data.cert_info.device_key);
                return 1;
            }
        }
        
        /*
        for (int i = 0; i < sensors.size; i++){
            printf("id: %d;\n name %s;\n path %s;\r\n_____________\r\n", i, sensors.sensor[i].s_name, sensors.sensor[i].s_path);
        }
        */

    } else {
        

        if (IOTCONNECT_AUTH_TYPE == IOTC_AT_X509) {
            if (access(IOTCONNECT_IDENTITY_CERT, F_OK) != 0 ||
                access(IOTCONNECT_IDENTITY_KEY, F_OK) != 0
                    ) {
                fprintf(stderr, "Unable to access device identity private key and certificate. "
                    "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
                    IOTCONNECT_SERVER_CERT);
            }
        }

        
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
            for (int i = 0; i < sensors.size; i++){
                sensors.sensor[i].reading = read_sensor(sensors.sensor[i]);
            }
                
            publish_telemetry(sensors);
            // repeat approximately evey ~5 seconds
            for (int k = 0; k < 500; k++) {
                iotconnect_sdk_receive();
                usleep(10000); // 10ms
            }
        }
        iotconnect_sdk_disconnect();
    }

    free_iotc_config(config);
    free_sensor_data(&sensors);

    return 0;
}
