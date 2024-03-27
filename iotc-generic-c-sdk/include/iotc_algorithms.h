/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_ALGORITHMS_H
#define IOTC_ALGORITHMS_H

#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern   "C" {
#endif

char *gen_sas_token(const char *host, const char* client_id, const char *b64key, time_t expiry_secs);

#ifdef __cplusplus
}
#endif

#endif // IOTC_ALGORITHMS_H
