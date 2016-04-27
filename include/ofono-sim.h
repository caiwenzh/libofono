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

#ifndef __OFONO_SIM_H_
#define __OFONO_SIM_H_

#include "log.h"
#include "ofono-common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_IMSI_LEN 15
#define MAX_ICCID_LEN 20
#define MAX_MCC_LEN 3
#define MAX_MNC_LEN 3
#define MAX_MSISDN_COUNT 2
#define MAX_MSISDN_LEN 20

enum pin_lock_type {
  PIN_LOCK_NONE,
  PIN_LOCK_SIM_PIN,
  PIN_LOCK_PHSIM_PIN,
  PIN_LOCK_PHFSIM_PIN,
  PIN_LOCK_SIM_PIN2,
  PIN_LOCK_PHNET_PIN,
  PIN_LOCK_PHNETSUB_PIN,
  PIN_LOCK_PHSP_PIN,
  PIN_LOCK_PHCORP_PIN,
  PIN_LOCK_SIM_PUK,
  PIN_LOCK_PHFSIM_PUK,
  PIN_LOCK_SIM_PUK2,
  PIN_LOCK_PHNET_PUK,
  PIN_LOCK_PHNETSUB_PUK,
  PIN_LOCK_PHSP_PUK,
  PIN_LOCK_PHCORP_PUK,
  PIN_LOCK_INVALID,
};

/* Note: According to ofono dbus API, can't get to know whether sim is ready
   directly, assume it's ready if retries has been got.
 */
enum sim_status {
  SIM_STATUS_ABSENT,
  SIM_STATUS_LOCKED,
  SIM_STATUS_INITIALIZING,
  SIM_STATUS_READY,
};

struct sim_info {
  enum sim_status status;
  enum pin_lock_type pin_required; /* pin currenlty required */
  tapi_bool pins[PIN_LOCK_INVALID]; /* Facility lock, TRUE: enabled */
  char imsi[MAX_IMSI_LEN + 1];
  char iccid[MAX_ICCID_LEN + 1];
  char mcc[MAX_MCC_LEN + 1];
  char mnc[MAX_MNC_LEN + 1];
  char msisdn[MAX_MSISDN_COUNT][MAX_MSISDN_LEN + 1];
  unsigned char retries[PIN_LOCK_INVALID];
};

enum sim_io_cmd {
  SIM_IO_CMD_READ_BINARY = 176,
  SIM_IO_CMD_READ_RECORD = 178,
  SIM_IO_CMD_GET_RESPONSE = 192,
  SIM_IO_CMD_UPDATE_BINARY = 214,
  SIM_IO_CMD_UPDATE_RECORD = 220,
  SIM_IO_CMD_STATUS = 242,
  SIM_IO_CMD_RETRIEVE_DATA = 203,
  SIM_IO_CMD_SET_DATA = 219,
};

/* detail of this data struct please refer to 3GPP 27.007 +CRSM */
struct sim_io_req {
  enum sim_io_cmd cmd;
  unsigned int fid; /* file identifier */
  char *path; /* path of an elementary file on the SIM/UICC in hex format */
  unsigned char p1;
  unsigned char p2;
  unsigned char p3;
  char *data; /* the data to write to card, may be NULL */
};

/* detail of this data struct please refer to 3GPP 27.007 +CRSM */
struct sim_io_resp {
  unsigned char sw1;
  unsigned char sw2;
  char *response;
};

/**
 * enable pin lock
 *
 * "type": pin type
 * "pin": pin
 *
 * Async response data: NULL
 */
void ofono_sim_enable_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data);

/**
 * disable pin lock
 *
 * "type": pin type
 * "pin": pin
 *
 * Async response data: NULL
 */
void ofono_sim_disable_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data);

/**
 * eneter password
 *
 * "type": pin type
 * "pin": pin
 *
 * Async response data: NULL
 */
void ofono_sim_enter_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data);

/**
 * reset pin
 *
 * "type": pin type
 * "puk": puk
 * "new_pin": new pin
 *
 * Async response data: NULL
 */
void ofono_sim_reset_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *puk,
      char *new_pin,
      response_cb cb,
      void *user_data);

/**
 * change pin lock password
 *
 * "type": pin type
 * "old_pin": old pin
 * "new_pin": new pin
 *
 * Async response data: NULL
 */
void ofono_sim_change_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *old_pin,
      char *new_pin,
      response_cb cb,
      void *user_data);

/**
 * get sim information: status, locked pin, imsi, iccid, mcc, mnc, msisdn etc
 *
 * sync API
 */
tapi_bool ofono_sim_get_info(struct ofono_modem *modem, struct sim_info *info);

/**
 * Restricted SIM access
 *
 * "req": SIM IO request detail
 *
 * Async response data: (struct sim_io_resp *)
 *
 * Note: oFono implements SIM IO internally, it doesn't export SIM IO API.
 *       This API isn't formally supported by oFono, it's user extending.
 */
void ofono_sim_io(struct ofono_modem *modem,
      struct sim_io_req *req,
      response_cb cb,
      void *user_data);

#ifdef  __cplusplus
}
#endif

#endif
