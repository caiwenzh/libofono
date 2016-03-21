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

#ifndef __OFONO_SMS__H
#define __OFONO_SMS__H

#include <time.h>
#include "ofono-common.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum ofono_sms_sent_state {
  OFONO_SMS_SENT_STATE_PENDING = 0,
  OFONO_SMS_SENT_STATE_SENT,
  OFONO_SMS_SENT_STATE_FAILED
};

enum ofono_cbs_emergency_type {
  OFONO_CBS_EMERG_TYPE_EARTHQUAKE = 0,
  OFONO_CBS_EMERG_TYPE_TSUNAMI,
  OFONO_CBS_EMERG_TYPE_EARTHQUAKE_TSUNAMI,
  OFONO_CBS_EMERG_TYPE_OTHER,
  OFONO_CBS_EMERG_TYPE_UNKNOWN
};

struct ofono_sms_sent_staus_noti {
  char *uuid;
  enum ofono_sms_sent_state state;
};

struct ofono_sms_incoming_noti {
  tapi_bool class0;
  time_t timestamp;
  char *sender;
  char *message;
};

struct ofono_sms_status_report_noti {
  char *message;
  char *uuid;
  time_t timestamp;
};

struct ofono_sms_cbs_config {
  tapi_bool powered;
  char *topics;
};

struct ofono_cbs_incoming_noti {
  char *message;
  guint16 channel;
};

struct ofono_cbs_emergency_noti {
  char *message;
  enum ofono_cbs_emergency_type type;
  tapi_bool popup;
  tapi_bool alert;
};

/**
 * Get SMS service center address
 *
 * Async response data: SMS service center address(char *)
 */
void ofono_sms_get_sca(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * Set SMS service center address
 *
 * "sca": sca address
 *
 * Async response data: NULL
 */
void ofono_sms_set_sca(struct ofono_modem *modem,
      const char *sca,
      response_cb cb,
      void *user_data);

/**
 * Get delivery report setting
 *
 * Async response data: delivery report setting(tapi_bool*)
 *   TURE - enabled, FALSE - disabled
 */
void ofono_sms_get_delivery_report(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * Set delivery report setting
 *
 * Async response data: NULL
 */
void ofono_sms_set_delivery_report(struct ofono_modem *modem,
      tapi_bool on,
      response_cb cb,
      void *user_data);

/**
 * Send sms
 *
 * "number": the destination
 * "message": SMS content
 *
 * Async response data: sms object path (char *)
 */
void ofono_sms_send_sms(struct ofono_modem *modem,
      const char *number,
      const char *message,
      response_cb cb,
      void *user_data);

/**
 * Send vcard
 *
 * "number": the destination
 * "message": v-card content
 *
 * Async response data: sms object (char *)
 */
void ofono_sms_send_vcard(struct ofono_modem *modem,
      const char *number,
      const unsigned char *message,
      response_cb cb,
      void *user_data);

/**
 * Send vcalendar
 *
 * "number": the destination
 * "message": v-calendar content
 *
 * Async response data: sms object (char *)
 */
void ofono_sms_send_vcalendar(struct ofono_modem *modem,
      const char *number,
      const unsigned char *message,
      response_cb cb,
      void *user_data);

/**
 * Get cell broadcast message config
 *
 * Async response data: cbs config (struct ofono_sms_cbs_config*)
 */
void ofono_sms_get_cbs_config(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * Enable/disable cell broadcast message of defined topics
 *
 * "powered": TURE - enable the cbs of topics, FALSE - disable cbs of topics
 *
 * Async response data: NULL
 */
void ofono_sms_set_cbs_powered(struct ofono_modem *modem,
      tapi_bool powered,
      response_cb cb,
      void *user_data);

/**
 * Set cell broadcast message topics (e.g. "0,1,5,320-478,922")
 *
 * "topics": cbs topics
 *
 * Async response data: NULL
 */
void ofono_sms_set_cbs_topics(struct ofono_modem *modem,
      const char *topics,
      response_cb cb,
      void *user_data);

#ifdef  __cplusplus
}
#endif

#endif
