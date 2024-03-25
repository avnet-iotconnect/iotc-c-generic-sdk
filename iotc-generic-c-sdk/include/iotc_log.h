/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Neil Matthews <nmatthews@witekio.com>, Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_LOG_H
#define IOTC_LOG_H

// MBEDTLS config file style - include your own to override the config. Similar to iotc-c-lib.
#if defined(IOTC_USER_CONFIG_FILE)
#include IOTC_USER_CONFIG_FILE
#endif


// define USE_SYSLOG to route messages to syslog
#ifdef USE_SYSLOG
#include <syslog.h>
#define IOTC_ERROR(...) syslog(LOG_ERR, __VA_ARGS__)
#define IOTC_WARN(...) syslog(LOG_WARNING, __VA_ARGS__)
#define IOTC_INFO(...) syslog(LOG_DEBUG, __VA_ARGS__)

#else

#ifndef IOTC_ENDLN
#define IOTC_ENDLN "\n"
#endif

#ifndef IOTC_INFO_LEVEL
#define IOTC_INFO_LEVEL 2
#endif

#ifndef IOTC_ERROR
#define IOTC_ERROR(...) fprintf(stderr, __VA_ARGS__);fprintf(stderr, IOTC_ENDLN)
#endif

#ifndef IOTC_WARN
#if IOTC_INFO_LEVEL > 0
#define IOTC_WARN(...) fprintf(stderr, __VA_ARGS__);fprintf(stderr, IOTC_ENDLN)
#else
#define IOTC_WARN(...)
#endif // IOTC_INFO_LEVEL
#endif

#ifndef IOTC_INFO
#if IOTC_INFO_LEVEL > 1
#define IOTC_INFO(...) printf(__VA_ARGS__);printf(IOTC_ENDLN)
#else
#define IOTC_INFO(...)
#endif // IOTC_INFO_LEVEL
#endif

#endif // USE_SYSLOG

#endif // IOTC_LOG_H

