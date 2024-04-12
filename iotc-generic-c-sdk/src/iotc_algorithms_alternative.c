/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Neil Matthews <nmatthews@witekio.com> et al.
 */

//
// Alternative implementation of gen_sas_token() that doesn't use OpenSSL
// Can be used as reference for embedded systems
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "iotconnect.h"
#include "iotc_algorithms.h"
#include "iotc_log.h"

#if IOTCONNECT_USE_CUSTOM_ALGORITHMS

#ifndef IOTHUB_RESOURCE_URI_FORMAT
#define IOTHUB_RESOURCE_URI_FORMAT "%s/devices/%s"
#endif

// resourceURI + \n + target expiry epoch timestamp
#ifndef IOTHUB_SIGNATURE_STR_FORMAT
#define IOTHUB_SIGNATURE_STR_FORMAT "%s\n%lu"
#endif

/*
 * Need to supply these routines, if want to generate SAS tokens
 */
static void iotc_hmac_sha256(const void *key, unsigned int keylen, const unsigned char *data, unsigned int datalen, unsigned char *result, unsigned int *resultlen);
static unsigned char *b64_string_to_buffer(const char *input, unsigned int *len);
static char *b64_buffer_to_string(const unsigned char *input, unsigned int length);

#ifndef IOTHUB_SAS_TOKEN_FORMAT
#define IOTHUB_SAS_TOKEN_FORMAT "SharedAccessSignature sr=%s&sig=%s&se=%lu"
#endif

static void sha256_helper(const unsigned char *data1, unsigned int datalen1, const unsigned char *data2, unsigned int datalen2, unsigned char *result, unsigned int *resultlen);

#if USE_OPENSSL_FOR_SHA_HELPER
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

static void sha256_helper(const unsigned char *data1, unsigned int datalen1,
            const unsigned char *data2, unsigned int datalen2,
            unsigned char *result, unsigned int *resultlen) {
	EVP_MD_CTX *c = EVP_MD_CTX_new();

	EVP_DigestInit_ex(c, EVP_sha256(), NULL);
	if(data1 != NULL && datalen1 != 0) {
            EVP_DigestUpdate(c, data1, datalen1);
	}
	if(data2 != NULL && datalen2 != 0) {
            EVP_DigestUpdate(c, data2, datalen2);
	}
        EVP_DigestFinal_ex(c, result, resultlen);
	EVP_MD_CTX_free(c);
}
#endif // USE_OPENSSL_FOR_SHA_HELPER

#if USE_MBEDTLS_FOR_SHA_HELPER
/*
 * sha256 helper is based on https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-tls-hashing/file/c68a6dc8d494/main.cpp/
 */
/*
 *  Hello world example of using the hashing functions of mbed TLS
 *
 *  Copyright (C) 2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
//#include "mbed.h"
#include "mbedtls/sha256.h" /* SHA-256 only */
#include "mbedtls/md.h"     /* generic interface */

#if DEBUG_LEVEL > 0
#include "mbedtls/debug.h"
#endif

#include "mbedtls/platform.h"

/*
 * result must be preallocated 32 byte array
 */
static void sha256_helper(const unsigned char *data1, unsigned int datalen1,
            const unsigned char *data2, unsigned int datalen2,
            unsigned char *result, unsigned int *resultlen) {
    *resultlen = 0;

    mbedtls_sha256_context c;

    mbedtls_sha256_init(&c);

    /* 0 means SHA-256 not SHA-224 */
    if(mbedtls_sha256_starts_ret(&c, 0) != 0)
    {
        IOTC_ERROR("mbedtls_sha256_starts_ret failed\n");
        return;
    }

    if(data1 != NULL && datalen1 != 0) {
        if(mbedtls_sha256_update_ret(&c, data1, datalen1) != 0)
        {
            IOTC_ERROR("mbedtls_sha256_update_ret #1 failed\n");
            return;
        }
    }

    if(data2 != NULL && datalen2 != 0) {
        if(mbedtls_sha256_update_ret(&c, data2, datalen2) != 0)
        {
            IOTC_ERROR("mbedtls_sha256_update_ret #2 failed\n");
            return;
        }
    }

    if(mbedtls_sha256_finish_ret(&c, result) != 0)
    {
        IOTC_ERROR("mbedtls_sha256_finish_ret failed\n");
        return;
    }
    *resultlen = 32;
}
#endif // USE_MBEDTLS_FOR_SHA_HELPER

#if defined(USE_OPENSSL_FOR_SHA_HELPER) || defined(USE_MBEDTLS_FOR_SHA_HELPER)
/*
 * This implementation of hmac_sha256 is based on the description in Wikipedia
 * https://en.wikipedia.org/wiki/HMAC
 */
static void iotc_hmac_sha256(const void *key, unsigned int keylen,
                                   const unsigned char *data, unsigned int datalen,
                                   unsigned char *result, unsigned int *resultlen) {
    const int hashSize = 32;
    const int blockSize = 64;
    unsigned int dummySize;

    unsigned char k_dash[blockSize];
    unsigned char i_key_pad_message_hash[hashSize];

    const unsigned char i_key_padding = (unsigned char) 0x36;
    const unsigned char o_key_padding = (unsigned char) 0x5c;

    memset(k_dash, 0, blockSize);
    if(keylen > blockSize) {
	sha256_helper(key, keylen, NULL, 0, k_dash, &dummySize);
    } else {
        memcpy(k_dash, key, keylen);
    }

    {
        unsigned char i_key_pad[blockSize];
        for(int i = 0;i < blockSize;i++) {
            i_key_pad[i] = (unsigned char) (i_key_padding ^ k_dash[i]);
        }

        sha256_helper(i_key_pad, blockSize, data, datalen, i_key_pad_message_hash, &dummySize);
    }

    unsigned char o_key_pad[blockSize];
    for(int i = 0;i < blockSize;i++) {
        o_key_pad[i] = (unsigned char) (o_key_padding ^ k_dash[i]);
    }

    sha256_helper(o_key_pad, blockSize, i_key_pad_message_hash, hashSize, result, resultlen);
}
#else

#define __must_check // needed to get crypto.h to compile
#include "crypto.h"
    
static void iotc_hmac_sha256(const void *key, unsigned int keylen,
                                   const unsigned char *data, unsigned int datalen,
                                   unsigned char *result, unsigned int *resultlen) {
    *resultlen = 0;
    if(hmac_sha256(key, keylen, data, datalen, result) != 0)
    {
        IOTC_ERROR("hmac_sha256 failed\n");
    }
    *resultlen = 32;
}
#endif

#define PADDING 255
#define SKIP 254
static unsigned char decode_base64_value(char c)
{
    if(c == '=')
    {
        return PADDING;
    } 

    if(c >= 'A' && c <= 'Z')
    {
        return (unsigned char) (c - 'A');
    }
    if(c >= 'a' && c <= 'z')
    {
        return (unsigned char) (26 + c - 'a');
    }
    if(c >= '0' && c <= '9')
    {
        return (unsigned char) (52 + c - '0');
    }
    if(c == '+')
    {
        return 62;
    }
    if(c == '/')
    {
        return 63;
    }

    return SKIP;
}

static unsigned char *b64_string_to_buffer(const char *input, unsigned int *len) {
    unsigned char read[4];
    unsigned int input_len;
    unsigned int max_decoded_b64_len;
    unsigned char *decoded_b64;
    unsigned int input_pos, to_be_decoded, output_pos;
    unsigned char value;

    *len = 0;

    input_len = strlen(input);
    max_decoded_b64_len = ((input_len + 3)/4) * 3;
    decoded_b64 = malloc(max_decoded_b64_len + 1); // unclear if need to NULL terminate, but just in case
    if(decoded_b64 == NULL)
    {
        return NULL;
    }

    for(input_pos = 0, to_be_decoded = 0, output_pos = 0;input_pos < input_len;input_pos++)
    {
        value = decode_base64_value(input[input_pos]);
        if(value == SKIP)
        {
            continue;
        }
        if(value == PADDING)
        {
            break;
        }
        read[to_be_decoded++] = (unsigned char) value;

        if(to_be_decoded == 4)
        {
            decoded_b64[output_pos + 0] = (unsigned char) ( ((read[0] & 0x3F) << 2) | ((read[1] & 0x30) >> 4) );
            decoded_b64[output_pos + 1] = (unsigned char) ( ((read[1] & 0x0F) << 4) | ((read[2] & 0x3C) >> 2) );
            decoded_b64[output_pos + 2] = (unsigned char) ( ((read[2] & 0x03) << 6) | (read[3] & 0x7F) );
            output_pos += 3;
            to_be_decoded = 0;
        }
    }

    switch(to_be_decoded)
    {
        case 3:
            // have 1 padding byte so output 2 characters
            if(value != PADDING) { ; /* possibly an issue here - input has run out but not enough for a full decode */ }

            decoded_b64[output_pos + 0] = (unsigned char) ((read[0] & 0x3F) << 2) | (unsigned char) ((read[1] & 0x30) >> 4);
            decoded_b64[output_pos + 1] = (unsigned char) ((read[1] & 0x0F) << 4) | (unsigned char) ((read[2] & 0x3C) >> 2);
            output_pos += 2;
            break;
        case 2:
            // have (presumably) 2 padding bytes so output 1 characters
            if(value != PADDING) { ; /* possibly an issue here - input has run out but not enough for a full decode */ }

            decoded_b64[output_pos + 0] = (unsigned char) ((read[0] & 0x3F) << 2) | (unsigned char) ((read[1] & 0x30) >> 4);
            output_pos += 1;
            break;
        case 1:
            // can't have 3 padding bytes
            break;
        case 0: // fallthrough
        default:
            break;
    }
    decoded_b64[output_pos] = '\0'; // unclear if need to NULL terminate, but just in case

    *len = output_pos;
    return decoded_b64;
}

char *b64_buffer_to_string(const unsigned char *input, unsigned int length) {
    unsigned char value[4];
    char *encoded_b64;
    unsigned int max_encoded_b64_len;
    static const char encoding[64] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
                                      'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
                                      '0','1','2','3','4','5','6','7','8','9',
                                      '+','/'};

    if(length == 0)
    {
        return NULL;
    }

    max_encoded_b64_len = ((length+2)/3)*4;
    encoded_b64 = malloc(max_encoded_b64_len + 1); // unclear if need to NULL terminate, but just in case
    if(encoded_b64 == NULL)
    {
        return NULL;
    }

    for(unsigned int i = 0, j = 0;i < length;i += 3, j += 4)
    {
        unsigned char c, d, e;

        c = input[i + 0];
        d = (unsigned char) (i + 1 < length) ? input[i+1] : 0;
        e = (unsigned char) (i + 2 < length) ? input[i+2] : 0;

        value[0] = (unsigned char) (c & 0xFC) >> 2;
        value[1] = (unsigned char) ( ((c & 0x03) << 4) | ((d & 0xF0) >> 4) );
        value[2] = (unsigned char) ( ((d & 0x0F) << 2) | ((e & 0xC0) >> 6) );
        value[3] = (unsigned char) (e & 0x3F);

        encoded_b64[j] = encoding[value[0]];
        encoded_b64[j+1] = encoding[value[1]];
        encoded_b64[j+2] = (i + 1 < length) ? encoding[value[2]] : '=';
        encoded_b64[j+3] = (i + 2 < length) ? encoding[value[3]] : '=';
    }
    encoded_b64[max_encoded_b64_len] = '\0'; // unclear if need to NULL terminate, but just in case

    return encoded_b64;
}

// outbuff length should be at least ((uri_len * 3) + 1)
static char *uri_encode(const char *uri) {
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

char *gen_sas_token(const char *host, const char *client_id, const char *b64key, time_t expiry_secs) {
    // example: SharedAccessSignature sr=poc-iotconnect-iothub-eu.azure-devices.net%2Fdevices%2CPID-DUUID&sig=WBBsC0rhu1idLR6aWaKiMbcrBCm9jPI4st2clhVKrW4%3D&se=1656689541
    // SharedAccessSignature sr={URL-encoded-resourceURI}&sig={signature-string}&se={expiry}
    // URL-encoded-resourceURI: myHub.azure-devices.net/devices/mydevice
    // expiry: unix time of expiry of signature
    // signature-string: {URL-encoded-resourceURI} + "\n" + expiry
    const size_t len_resource_uri = (size_t) snprintf(NULL, 0, IOTHUB_RESOURCE_URI_FORMAT, client_id, host);

    unsigned long int expiration = ((unsigned long int) time(NULL)) + (unsigned long int) expiry_secs;
    char *resource_uri = malloc(len_resource_uri + 1);
    if(!resource_uri) {
        return NULL;
    }

    sprintf(resource_uri, IOTHUB_RESOURCE_URI_FORMAT, host, client_id);
    char *encoded_resource_uri = uri_encode(resource_uri);
    free(resource_uri);

    const size_t len_string_to_sign = (size_t) snprintf(NULL, 0, IOTHUB_SIGNATURE_STR_FORMAT,
            encoded_resource_uri,
            (unsigned long) expiration
    );
    char *string_to_sign = malloc(len_string_to_sign + 1);
    if(!string_to_sign) {
        free(encoded_resource_uri);
        return NULL;
    }
    sprintf(string_to_sign, IOTHUB_SIGNATURE_STR_FORMAT,
            encoded_resource_uri,
            (unsigned long int) expiration
    );


    unsigned int keylen = 0;
    unsigned char *key = b64_string_to_buffer(b64key, &keylen);

    unsigned char digest[32];
    unsigned int digest_len = 0;
    iotc_hmac_sha256(key, keylen, (const unsigned char*) string_to_sign, strlen(string_to_sign), digest, &digest_len);
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
    if(sas_token) {
        sprintf(sas_token, IOTHUB_SAS_TOKEN_FORMAT,
                encoded_resource_uri,
                encoded_b64_digest,
                (unsigned long int) expiration);
    }
    free(encoded_resource_uri);
    free(encoded_b64_digest);

    return sas_token;
}

#endif // EMBEDDED_DEVICE

