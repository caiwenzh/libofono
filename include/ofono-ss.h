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

#ifndef __OFONO_SS_H
#define __OFONO_SS_H

#include "ofono-common.h"
#include "ofono-sim.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define SS_PW_LEN_MAX 4 /* SS PWD should be four digits (0000 to 9999) */
#define CALL_DIGIT_LEN_MAX 82
#define SS_USSD_DATA_SIZE_MAX 208

enum call_forward_condition {
  SS_CF_CONDITION_CFU, /* Unconditional */
  SS_CF_CONDITION_CFB, /* Mobile Busy */
  SS_CF_CONDITION_CFNRY, /* No Reply */
  SS_CF_CONDITION_CFNRC, /* Unreachable */
//  SS_CF_CONDITION_ALL,   /* All */
//  SS_CF_CONDITION_ALL_CFC, /* ALl conditional */
};

enum call_barring_type {
  SS_CB_TYPE_NONE,
  SS_CB_TYPE_BAOC, /* All Outgoing Calls */
  SS_CB_TYPE_BOIC, /* Barring Outgoing International Calls */
  SS_CB_TYPE_BOIC_NOT_HC, /* Barring Outgoing International Calls
           except to Home Country */
  SS_CB_TYPE_BAIC, /* Barring All Incoming Calls */
  SS_CB_TYPE_BIC_ROAM, /* Barring Incoming Calls when roam,
        outside of the Home Country */
  SS_CB_TYPE_AB, /* All Call */
  SS_CB_TYPE_AG, /* All Outgoing Call */
  SS_CB_TYPE_AC, /* All Incoming Call */
};

enum cli_type {
  SS_CLI_CLIP, /* Calling Line Identification Presentation */
  SS_CLI_CDIP, /* Called Line Identification Presentation */
  SS_CLI_CNAP, /* Calling Name Presentation */
  SS_CLI_COLP, /* Connected Line Identification Presentation */
  SS_CLI_COLR, /* Connected Line Identification Restriction */
};

enum cli_status {
  SS_CLI_STATUS_ENABLED,
  SS_CLI_STATUS_DISABLED,
  SS_CLI_STATUS_UNKNOWN,
};

enum clir_network_status {
  SS_CLIR_NW_STATUS_DISABLED, /* Service isn't provisioned */
  SS_CLIR_NW_STATUS_PERMANENT, /* Provisioned in permanent mode */
  SS_CLIR_NW_STATUS_UNKOWN,
  SS_CLIR_NW_STATUS_ON, /* Temp restricted */
  SS_CLIR_NW_STATUS_OFF, /* Temp allowed */
};

enum clir_dev_status {
  SS_CLIR_DEV_STATUS_DEFAULT,
  SS_CLIR_DEV_STATUS_ENABLED, /* CLIR invoked, CLI is withheld */
  SS_CLIR_DEV_STATUS_DISABLED, /* CLIR suppressed, CLI is provided */
};

enum ussd_status {
  SS_USSD_STATUS_IDLE, /* No active USSD session */
  SS_USSD_STATUS_ACTIVE , /* Waiting for network response */
  SS_USSD_STATUS_ACTION_REQUIRE, /* Further user action required*/
};

/* The SS request initiated by MMI string. e.g. *#21# */
enum ss_req_type {
  SS_REQ_TYPE_USSD,
  SS_REQ_TYPE_CALL_BARRING,
  SS_REQ_TYPE_CALL_FORWARDING,
  SS_REQ_TYPE_CALL_WAITING,
  SS_REQ_TYPE_CLIP,
  SS_REQ_TYPE_COLP,
  SS_REQ_TYPE_CLIR,
  SS_REQ_TYPE_COLR,
};

/* SS Operation */
enum ss_op {
  SS_OP_ACTIVATION,
  SS_OP_REGISTRATION,
  SS_OP_DEACTIVATION,
  SS_OP_INTERROGATION,
  SS_OP_EARASURE,
};

/* Bearer Class */
enum bearer_class {
  BEARER_CLASS_ALL, /* bearer class isn't specified */
  BEARER_CLASS_VOICE,
  BEARER_CLASS_DATA,
  BEARER_CLASS_FAX,
  BEARER_CLASS_DEFAULT,
  BEARER_CLASS_SMS,
  BEARER_CLASS_DATA_SYNC,
  BEARER_CLASS_DATA_ASYNC,
  BEARER_CLASS_SS_DEFAULT,
  BEARER_CLASS_PACKET,
  BEARER_CLASS_PAD,
};

struct call_forward_setting {
  tapi_bool enable;
  enum call_forward_condition condition;
  char num[CALL_DIGIT_LEN_MAX];
  unsigned char timeout; /* no reply timeout (1 - 30 seconds)*/
};

struct call_barring_setting {
  enum call_barring_type incoming; /* incoming barring */
  enum call_barring_type outgoing; /* outgoing barring */
};

struct ussd_request_noti {
  char *message;
  tapi_bool response_required;
};

/**
 * get call waiting status
 *
 * Async response data: FALSE - call waiting is off, TRUE - it is on
 */
void ofono_ss_get_call_waiting(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * set call waiting
 *
 * "enable": TRUE - enable call waiting; FLASE - disable call waiting
 *
 * Async response data: NULL
 */
void ofono_ss_set_call_waiting(struct ofono_modem *modem,
      tapi_bool enable,
      response_cb cb,
      void *user_data);

/**
 * get call forwarding
 *
 * Async response data: struct call_forward_setting
 */
void ofono_ss_get_call_forward(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * set call forwarding
 *
 * Async response data: NULL
 */
void ofono_ss_set_call_forward(struct ofono_modem *modem,
      struct call_forward_setting *setting,
      response_cb cb,
      void *user_data);

/**
 * get call barring
 *
 * Async response data: struct call_barring_setting
 */
void ofono_ss_get_call_barring(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * set call barring
 *
 * Async response data: NULL
 */
void ofono_ss_set_call_barring(struct ofono_modem *modem,
      tapi_bool enable,
      enum call_barring_type type,
      const char *pwd,
      response_cb cb,
      void *user_data);

void ofono_ss_change_barring_password(struct ofono_modem *modem,
      const char *old_pwd,
      const char *new_pwd,
      response_cb cb,
      void *user_data);

/**
 * get cli_status (CLIP, CDIP, CNAP, COLP, COLR)
 *
 * Async response data: enum cli_status[5]
 */
void ofono_ss_get_cli_status(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * get CLIR status (network side)
 *
 * Async response data: enum clir_network_status
 */
void ofono_ss_get_clir(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * set CLIR (device side)
 *
 * Async response data: NULL
 */
void ofono_ss_set_clir(struct ofono_modem *modem,
      enum clir_dev_status status,
      response_cb cb,
      void *user_data);
/**
 * Initiate USSD session
 *
 * Note:
 *   Ofono support both USSD striing and MMI string. But it currently only
 * support a part of MMI string. To simply the design only allow USSD string
 * for the API. MMI string should be process at application layer.
 *
 * "str": ussd requset string (don't pass in MMI string, such as *#21#)
 *
 * Async response data: request response (char *)
 */
void ofono_ss_initiate_ussd_request(struct ofono_modem *modem,
      char *str,
      response_cb cb,
      void *user_data);

/**
 * Send USSD response to network
 *
 * "str": response string
 *
 * Async response data: network response string (char*)
 */
void ofono_ss_send_ussd_response(struct ofono_modem *modem,
      char *str,
      response_cb cb,
      void *user_data);

/**
 * Terminate the USSD session
 *
 * Async response data: NULL
 */
void ofono_ss_cancel_ussd_session(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

#ifdef  __cplusplus
}
#endif

#endif
