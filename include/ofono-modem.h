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

#ifndef __OFONO_MODEM_H_
#define __OFONO_MODEM_H_

#include "ofono-common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_MODEM_INFO_SIZE 2048

enum modem_type {
  MODEM_TYPE_TEST, /* virtual modem for testing */
  MODEM_TYPE_HFP,  /* Bluethooh HFP */
  MODEM_TYPE_SAP,  /* Bluethooh SAP */
  MODEM_TYPE_HARDWARE, /* Regular modem */
};

enum modem_status {
  MODEM_STATUS_OFF, /* modem is power off */
  MODEM_STATUS_OFFLINE, /* modem is offline (power on) */
  MODEM_STATUS_ONLINE,  /* modem is online */
};

struct modem_info {
  char manufacturer[MAX_MODEM_INFO_SIZE + 1];
  char model[MAX_MODEM_INFO_SIZE + 1];
  char revision[MAX_MODEM_INFO_SIZE + 1];
  char serial[MAX_MODEM_INFO_SIZE + 1]; /* product sn, e.g. IMEI */
  enum modem_type type;
};

/**
 * get if modem is online
 *
 * (sync API)
 */
tapi_bool ofono_modem_get_online(struct ofono_modem *modem);

/**
 * set modem online/offline
 *
 * Async response data: NULL
 */
void ofono_modem_set_online(struct ofono_modem *modem,
      tapi_bool online,
      response_cb cb,
      void *user_data);

/**
 * get if modem is powered on
 *
 * Sync API
 */
tapi_bool ofono_modem_get_powered(struct ofono_modem *modem);

/**
 * Power on/off modem
 *
 * Async response data: NULL
 */
void ofono_modem_set_powered(struct ofono_modem *modem,
      tapi_bool online,
      response_cb cb,
      void *user_data);

/**
 * get modem information: manufacturer, model, revision, serial number
 *
 * Sync API
 */
tapi_bool ofono_modem_get_info(struct ofono_modem *modem, struct modem_info* info);

#ifdef  __cplusplus
}
#endif

#endif
