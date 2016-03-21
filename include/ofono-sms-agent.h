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

#ifndef _OFONO_SMS_AGENT__H
#define _OFONO_SMS_AGENT__H

#include "ofono-common.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct ofono_push_noti_agent;

struct ofono_push_noti_info {
  char *content; /* push notification content */
  int length; /* push notification content length */

  char *local_senttime; /* local time (device system time) */
  char *senttime; /* sevice center time stamp */
  char *sender;
};

typedef void (*push_notify_cb_t)(struct ofono_push_noti_info *info, void *user_data);

struct ofono_push_noti_agent* ofono_new_push_agent(struct ofono_modem *modem);
void ofono_free_push_agent(struct ofono_push_noti_agent* agent);

TResult ofono_register_push_notification_callback(
      struct ofono_push_noti_agent* agent,
      push_notify_cb_t cb,
      void *user_data);

TResult ofono_unregister_push_notification_callback(
      struct ofono_push_noti_agent* agent,
      push_notify_cb_t cb);

#ifdef  __cplusplus
}
#endif

#endif /* _OFONO_SMS_AGENT__H */
