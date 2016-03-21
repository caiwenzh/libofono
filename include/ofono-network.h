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

#ifndef __OFONO_NETWORK_H_
#define __OFONO_NETWORK_H_
#include "ofono-common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_MCC_LEN 3
#define MAX_MNC_LEN 3

enum registration_status {
  REG_STATUS_NOT_REGISTERED,
  REG_STATUS_REGISTERED_HOME,
  REG_STATUS_SEARCHING,
  REG_STATUS_DENIED,
  REG_STATUS_UNKNOWN,
  REG_STATUS_REGISTERED_ROAMING,
};

enum network_selection_mode {
  NETWORK_SELECTION_MODE_UNKNOWN,
  NETWORK_SELECTION_MODE_AUTO,
  NETWORK_SELECTION_MODE_MANUAL,
  NETWORK_SELECTION_MODE_AUTO_ONLY
};

enum access_tech {
  ACCESS_TECH_UNKNOWN,
  ACCESS_TECH_GSM,
  ACCESS_TECH_GSM_COMPACT, /* GPRS */
  ACCESS_TECH_UTRAN, /* 3G */
  ACCESS_TECH_EDGE,
  ACCESS_TECH_UTRAN_HSDPA,
  ACCESS_TECH_UTRAN_HSUPA,
  ACCESS_TECH_UTRAN_HSDPA_HSUPA,
  ACCESS_TECH_EUTRAN, /* 4G */
};

enum operator_status {
  OPERATOR_STATUS_UNKNOWN,
  OPERATOR_STATUS_AVAILABLE,
  OPERATOR_STATUS_CURRENT,
  OPERATOR_STATUS_FORBIDDEN,
};

enum network_mode {
  NETWORK_MODE_UNKNOWN = 0x00,
  NETWORK_MODE_AUTO = 0x01, /* auto or hybird */
  NETWORK_MODE_2G = 0x02, /* 2G only */
  NETWORK_MODE_3G = 0x04, /* 3G only */
  NETWORK_MODE_4G = 0x08, /* 4G only */
};

struct operator_info {
  char plmn[MAX_MCC_LEN + MAX_MNC_LEN + 1];
    /* MCCMNC, numeric operator name */
  char *name; /* Operator name */
  unsigned int techs; /* act_tech bitmap */
  enum operator_status status;
  char *path;
};

struct operators_info {
  int count;
  struct operator_info *ops;
};

struct registration_info {
  enum registration_status status;
  enum access_tech act;
  unsigned short lac;
  unsigned int cid;
  char mcc[MAX_MCC_LEN + 1];
  char mnc[MAX_MNC_LEN + 1];
};

/**
 * get network registration information, include registration status, access
 * technology, lac, cid, operator name, mcc, mnc and network selection mode.
 *
 * sync API
 */
 tapi_bool ofono_network_get_registration_info(struct ofono_modem *modem,
       struct registration_info *info);

/**
 * Get current registered operator name
 *
 * sync API (should free *name)
 */
 tapi_bool ofono_network_get_operator_name(struct ofono_modem *modem,
       char **name);

/**
 * get sigal strength [0 - 100]
 *
 * sync API
 */
tapi_bool ofono_network_get_signal_strength(struct ofono_modem *modem,
      unsigned char *signal);

/**
 * get network selection mode
 *
 * sync API
 */
tapi_bool ofono_network_get_network_selection_mode(struct ofono_modem *modem,
       enum network_selection_mode *mode);

/**
 * get network mode
 *
 * Async response data: network mode (enum network_mode)
 */
void ofono_network_get_mode(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * set network mode
 *
 * "mode": network mode
 *
 * Async response data: NULL
 */
void ofono_network_set_mode(struct ofono_modem *modem,
      enum network_mode mode,
      response_cb cb,
      void *user_data);

/**
 * register to specified network
 *
 * "plmn": the numeric name (MCCMNC) of the network to register to
 * //"act": access technology (Currently doesn't support)
 *
 * Async response data: NULL
 */
void ofono_network_register(struct ofono_modem *modem,
      const char *plmn,
      response_cb cb,
      void *user_data);

/**
 * auto register to network
 *
 * Async response data: NULL
 */
void ofono_network_auto_register(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * scan operators
 *
 * Async response data: struct operators_info*, operators information)
 */
void ofono_network_scan_operators(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

#ifdef  __cplusplus
}
#endif

#endif
