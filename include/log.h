/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __LOG_H_
#define __LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG

#define tapi_debug(...)
#define tapi_info(...)
#define tapi_warn(...)
#define tapi_error(...)
#define tapi_fatal(...)
#define tapi_log_bin(bin, size)

#else /* NDEBUG */

#include <stdio.h>

#define tapi_log_print printf

#define tapi_debug(fmt, args...) \
  tapi_log_print("<%s %s:%d> " fmt "\n", __FILE__, __func__, __LINE__, ##args)

#define tapi_info(fmt, args...) \
  tapi_log_print("<%s %s:%d> " fmt "\n", __FILE__, __func__, __LINE__, ##args)

#define tapi_warn(fmt, args...) \
  tapi_log_print("<Warn> %s %s:%d> " fmt "\n",__FILE__, __func__, __LINE__, ##args)

#define tapi_error(fmt, args...) \
  tapi_log_print("<Error> %s %s:%d> " fmt "\n",__FILE__, __func__, __LINE__, ##args)

#define tapi_fatal(fmt, args...) \
  tapi_log_print("<Fatal> %s %s:%d> " fmt "\n",__FILE__, __func__, __LINE__, ##args)

#define tapi_log_bin(_bin, _size) \
  { \
    int _log_i = 0; \
    unsigned char *_log_bin = (unsigned char *)_bin; \
    for (_log_i = 0; _log_i < _size; _log_i++) \
      tapi_log_print("%02X ", _log_bin[_log_i]); \
    tapi_log_print("\n"); \
  }

#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif

#endif
