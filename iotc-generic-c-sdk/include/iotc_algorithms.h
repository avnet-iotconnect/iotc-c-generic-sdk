//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 7/1/21.
//

#ifndef IOTC_ALGORITHMS_H
#define IOTC_ALGORITHMS_H

#include <stdlib.h>

#ifdef __cplusplus
extern   "C" {
#endif

char *gen_sas_token(const char *host, const char *cpid, const char *duid, const char *b64key, time_t expiry_secs);

#ifdef __cplusplus
}
#endif

#endif // IOTC_ALGORITHMS_H
