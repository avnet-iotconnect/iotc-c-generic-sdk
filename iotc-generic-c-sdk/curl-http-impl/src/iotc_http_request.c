//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "iotc_http_request.h"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int iotconnect_https_request(
        IotConnectHttpResponse *response,
        const char *url,
        const char *send_str
) {
    CURL *curl;
    CURLcode res = (!CURLE_OK); /* FIXME there's probably a better value to initialize this too */

    if (NULL == response) {
        fprintf(stderr, "iotconnect_https_request() requires a valid IotConnectHttpResponse pointer.");
        return res;
    }
    response->data = NULL;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        struct curl_slist *header_slist = NULL;
        header_slist = curl_slist_append(header_slist, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_slist);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (send_str) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, send_str);
        }
        response->data = malloc(1); // start with 1 byte and regrow into write_memory_cb
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "iotconnect_https_request() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }
        response->data = chunk.memory;
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return (int) res;
}


void iotconnect_free_https_response(IotConnectHttpResponse *response) {
    free(response->data);
    response->data = NULL;
}
