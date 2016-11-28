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

#ifndef __OFONO_CONNMAN_H
#define __OFONO_CONNMAN_H

#include "ofono-common.h"
#include "ofono-network.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum context_type {
  CONTEXT_TYPE_UNKNOWN,
  CONTEXT_TYPE_MMS,
  CONTEXT_TYPE_INTERNET,
  CONTEXT_TYPE_WAP,
  CONTEXT_TYPE_IMS,
};

enum ip_protocol {
  IP_PROTOCAL_IPV4,
  IP_PROTOCAL_IPV6,
  IP_PROTOCAL_IPV4_IPV6,
};

struct pdp_context {
  char *apn;
  char *user_name;
  char *pwd;
  char *proxy; /* wap proxy */
  char *mmsc;
  enum ip_protocol protocol;
};

struct ipv4_settings {
  char *iface; /* network interface */
  char *ip;
  char *netmask;
  char *gateway;
  char *proxy;
  char *dns[2];
};

struct ipv6_settings {
  char *iface; /* network interface */
  char *ip;
  unsigned char prefix_len; /* ip prefix length */
  char *gateway;
  char *dns[2];
};

struct pdp_context_info {
  tapi_bool actived;
  enum context_type type;
  struct ipv4_settings ipv4;
  struct ipv6_settings ipv6;
};

struct context_actived_noti {
  char *path; /* the object path of the pdp context */
  tapi_bool actived;
};

struct ps_reg_status {
  /* network attaching status, indicates whether data service is available */
  tapi_bool attached;
  enum access_tech tech; /* data network acess technology */
};

/**
 * Add a primary PDP context
 *
 * "type": Context type, can be: internet, wap, mms or ims.
 *
 * Async response data: (char *) the new added pdp context object path
 */
void ofono_connman_add_context(struct ofono_modem *modem,
      enum context_type type,
      response_cb cb,
      void *user_data);

/**
 * Remove the specified primary PDP context
 *
 * "path": the pdp context object path (returned by ofono_add_context)
 *
 * Async response data: NULL
 */
void ofono_connman_remove_context(struct ofono_modem *modem,
      const char *path,
      response_cb cb,
      void *user_data);

/**
 * Set context
 *
 * "context": Context settings, include APN, user name, password, poxy, mmsc etc
 *
 * Async response data: NULL
 */
void ofono_connman_set_context(struct ofono_modem *modem,
      char *path,
      struct pdp_context *context,
      response_cb cb,
      void *user_data);

/**
 * Get context information
 *
 * "path": pdp context object path (returned by ofono_add_context)
 *
 * Sync API: (struct pdp_context_info) context setting, should be released
 */
tapi_bool ofono_connman_get_context_info(struct ofono_modem *modem,
      char *path, struct pdp_context_info *info);

/**
 * Get all pdp contexts
 *
 * Sync API, response data: struct str_list *, a list of pdp context object
 * paths, should free it by ofono_string_list_free.
 *
 */
tapi_bool ofono_connman_get_contexts(struct ofono_modem *modem,
      struct str_list **contexts);

/**
 * Active pdp context
 *
 * "path": the context obect path (returned by ofono_add_context)
 *
 * Async response data: NULL
 */
void ofono_connman_activate_context(struct ofono_modem *modem,
      char *path,
      response_cb cb,
      void *user_data);

/**
 * Deactive pdp context
 *
 * "path": the context object path (returned by ofono_add_context)
 *
 * Async response data: NULL
 */
void ofono_connman_deactivate_context(struct ofono_modem *modem,
      char *path,
      response_cb cb,
      void *user_data);

/**
 * Deactive all pdp context
 *
 * Async response data: NULL
 */
void ofono_connman_deactivate_all_contexts(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);

/**
 * Get whether data service is allowed
 *
 * Sync API
 */
tapi_bool ofono_connman_get_powered(struct ofono_modem *modem,
      tapi_bool *powered);

/**
 * Controls whether data service is allowed
 *
 * "powered": TRUE - allowed; FALSE - not allowed
 *
 * Async response data: NULL
 */
void ofono_connman_set_powered(struct ofono_modem *modem,
      tapi_bool powered,
      response_cb cb,
      void *user_data);

/**
 * Get whether data service is allowed when roaming
 *
 * Sync API
 */
tapi_bool ofono_connman_get_roaming_allowed(struct ofono_modem *modem,
      tapi_bool *allowed);

/**
 * Controls whether data service is allowed when roaming
 *
 * "powered": TRUE - allowed; FALSE - not allowed
 *
 * Async response data: NULL
 */
void ofono_connman_set_roaming_allowed(struct ofono_modem *modem,
      tapi_bool allowed,
      response_cb cb,
      void *user_data);

/**
 * Get if Packet Radio Service status
 *
 * Sync API
 */
tapi_bool ofono_connman_get_status(struct ofono_modem *modem,
      struct ps_reg_status *status);

#ifdef  __cplusplus
}
#endif

#endif /* __OFONO_CONNMAN_H */
