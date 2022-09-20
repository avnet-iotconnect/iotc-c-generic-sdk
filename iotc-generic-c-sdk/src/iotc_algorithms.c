//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/26/21.
//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

#ifndef IOTHUB_RESOURCE_URI_FORMAT
#define IOTHUB_RESOURCE_URI_FORMAT "%s/devices/%s-%s"
#endif

// resourceURI + \n + target expiry epoch timestamp
#ifndef IOTHUB_SIGNATURE_STR_FORMAT
#define IOTHUB_SIGNATURE_STR_FORMAT "%s\n%lu"
#endif

#ifndef IOTHUB_SAS_TOKEN_FORMAT
#define IOTHUB_SAS_TOKEN_FORMAT "SharedAccessSignature sr=%s&sig=%s&se=%lu"
#endif

unsigned char *hmac_sha256(const void *key, int keylen,
                                   const unsigned char *data, int datalen,
                                   unsigned char *result, unsigned int *resultlen) {
    return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}

unsigned char *b64_string_to_buffer(const char *input, unsigned int *len) {
    BIO *b64, *source;
    size_t length = strlen(input);

    unsigned char *buffer = malloc(length / 4 * 3 + 5); // extra 5 bytes to be safe
    if(!buffer) {
        return NULL;
    }

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    source = BIO_new_mem_buf(input, length);
    BIO_push(b64, source);

    *len = BIO_read(b64, buffer, length);

    BIO_free_all(b64);

    return buffer;
}

char *b64_buffer_to_string(const unsigned char *input, unsigned int length) {
    BIO *bmem, *b64;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    //BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, (int) length);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *) malloc(bptr->length);
    if(!buff) {
        return NULL;
    }
    memcpy(buff, bptr->data, bptr->length - 1);
    buff[bptr->length - 1] = 0;

    BIO_free_all(b64);

    return buff;
}

// outbuff length should be at least ((uri_len * 3) + 1)
char *uri_encode(const char *uri) {
    const size_t uri_len = strlen(uri);
    char *outbuff = malloc((uri_len * 3) + 1);
    if(!outbuff) {
        return NULL;
    }

    char *p = outbuff;
    size_t i = 0;
    for (; i < strlen(uri); i++) {
        char c = uri[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '~' || c == '.') {
            *p = c;
            p++;
        } else {
            sprintf(p, "%%%02X", c);
            p += 3;
        }
    }
    *p = 0;
    return outbuff;
}

char *gen_sas_token(const char *host, const char *cpid, const char *duid, char *b64key, time_t expiry_secs) {
    // example: SharedAccessSignature sr=poc-iotconnect-iothub-eu.azure-devices.net%2Fdevices%2CPID-DUUID&sig=WBBsC0rhu1idLR6aWaKiMbcrBCm9jPI4st2clhVKrW4%3D&se=1656689541
    // SharedAccessSignature sr={URL-encoded-resourceURI}&sig={signature-string}&se={expiry}
    // URL-encoded-resourceURI: myHub.azure-devices.net/devices/mydevice
    // expiry: unix time of expiry of signature
    // signature-string: {URL-encoded-resourceURI} + "\n" + expiry
    const size_t len_host = strlen(host);
    const size_t len_cpid = strlen(cpid);
    const size_t len_duid = strlen(duid);
    const size_t len_resource_uri = (sizeof(IOTHUB_RESOURCE_URI_FORMAT) + len_host + len_cpid + len_duid) * 3;

    const time_t expiration = time(NULL) + expiry_secs;
    char *resource_uri = malloc(len_resource_uri);
    if(!resource_uri) {
        return NULL;
    }

    sprintf(resource_uri, IOTHUB_RESOURCE_URI_FORMAT,
            host,
            cpid,
            duid
    );
    char *encoded_resource_uri = uri_encode(resource_uri);
    free(resource_uri);

    char *string_to_sign = malloc(strlen(encoded_resource_uri) + 1 /* \n */ + 10 /* epoch time */ + 1 /* NULL */);
    if(!string_to_sign) {
        free(encoded_resource_uri);
        return NULL;
    }
    sprintf(string_to_sign, IOTHUB_SIGNATURE_STR_FORMAT,
            encoded_resource_uri,
            expiration
    );


    unsigned int bufflen = 0;
    unsigned char *key = b64_string_to_buffer(b64key, &bufflen);

    unsigned char digest[EVP_MAX_MD_SIZE];;
    unsigned int digest_len = 0;
    hmac_sha256(key, (int) bufflen, (const unsigned char*) string_to_sign, (int) strlen(string_to_sign), digest, &digest_len);
    free(key);
    free(string_to_sign);

    char *b64_digest = b64_buffer_to_string(digest, digest_len);
    char *encoded_b64_digest = uri_encode(b64_digest);
    free(b64_digest);

    char *sas_token = malloc(sizeof(IOTHUB_SAS_TOKEN_FORMAT) +
                             strlen(encoded_resource_uri) +
                             strlen(encoded_b64_digest) +
                             +10 /* unix time */
    );
    if(!sas_token) {
        free(encoded_resource_uri);
        free(encoded_b64_digest);
        return NULL;
    }
    sprintf(sas_token, IOTHUB_SAS_TOKEN_FORMAT,
            encoded_resource_uri,
            encoded_b64_digest,
            expiration
    );
    free(encoded_resource_uri);
    free(encoded_b64_digest);
    printf("Token: %s\n", sas_token);
    return sas_token;
}

