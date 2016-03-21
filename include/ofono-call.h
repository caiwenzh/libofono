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
#ifndef __OFONO_CALL_H
#define __OFONO_CALL_H

#include "ofono-common.h"
#include "ofono-ss.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_CALL_PARTIES 6
#define MAX_CALLING_NAME_LEN 82
#define MAX_CALLING_NUMBER_LEN 82

enum ofono_call_status {
  CALL_STATUS_ACTIVE = 0,
  CALL_STATUS_HELD,
  CALL_STATUS_DIALING,
  CALL_STATUS_ALERTING,
  CALL_STATUS_INCOMING,
  CALL_STATUS_WAITING,
  CALL_STATUS_DISCONNECTED,
};

enum ofono_call_disc_reason {
  CALL_DISCONNECT_REASON_UNKNOWN = 0,
  CALL_DISCONNECT_REASON_LOCAL_HANGUP,
  CALL_DISCONNECT_REASON_REMOTE_HANGUP,
  CALL_DISCONNECT_REASON_ERROR,
};

struct ofono_call_info {
  unsigned int call_id; /* Note: the id is assigned by ofonod, it may not
         be the same as the id assigned by modem(CP) */
  enum ofono_call_status status;
  char line_id[MAX_CALLING_NUMBER_LEN + 1]; /* call line identification,
            e.g. call number */
  char name[MAX_CALLING_NAME_LEN + 1];
  tapi_bool multiparty;
  tapi_bool emergency;
};

struct ofono_calls {
  unsigned int count;
  struct ofono_call_info calls[MAX_CALL_PARTIES];
};

struct ofono_call_disconnect_reason {
  unsigned int call_id;
  enum ofono_call_disc_reason reason;
};

/**
 * Get emergency numbers
 *
 * Sync API: response data "ecc": (data: struct str_list, should free it by
 *   ofono_string_list_free).
 */
tapi_bool ofono_call_get_ecc(struct ofono_modem *modem, struct str_list** ecc);

/**
 * Initiate a new outgoing call
 *
 * "number": the number to dial
 * "clir": call line identification restriction information
 *
 * Async response data: unsigned int (the call id assigned by ofonod)
 */
void ofono_call_dial(struct ofono_modem *modem,
                char *number,
                enum clir_dev_status clir,
                response_cb cb,
                void *user_data);

/**
 * Answer the incoming call
 *
 * Async response data: NULL
 */
void ofono_call_answer(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Release sepcific call (any status except disconnected)
 *
 * "call_id": the id of the call to release
 *
 * Async response data: NULL
 */
void ofono_call_release_specific(struct ofono_modem *modem,
                unsigned int call_id,
                response_cb cb,
                void *user_data);

/**
 * Release all calls except waiting call
 *
 * Async response data: NULL
 */
void ofono_call_release_all(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);
/**
 * Swap active and held calls
 *
 * Async response data: NULL
 */
void ofono_call_swap(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Release currently active call (0 or more) and answer
 * the currently waiting call.
 *
 * Async response data: NULL
 */
void ofono_call_release_and_answer(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Release currently active call (0 or more) and activate
 * any currently held calls.
 *
 * Async response data: NULL
 */
void ofono_call_release_and_swap(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Place all active calls (if any exist) on hold and accept
 * the other(held or waiting) call(s).
 *
 * Async response data: NULL
 */
void ofono_call_hold_and_answer(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Join current active call and held call
 *
 * Async response data: NULL
 */
void ofono_call_transfer(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Redirect an incoming or a waiting call to the specified destination
 *
 * "number": redirect destination
 *
 * Async response data: NULL
 */
void ofono_call_deflect(struct ofono_modem *modem,
                char *number,
                response_cb cb,
                void *user_data);

/**
 * Joins active and held calls together into a multi-party call
 *
 * Async response data: NULL
 */
void ofono_call_create_multiparty(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Hangs up multi-party call
 *
 * Async response data: NULL
 */
void ofono_call_hangup_multiparty(struct ofono_modem *modem,
                response_cb cb,
                void *user_data);

/**
 * Hold others in a conference and only keep one actived
 *
 * "call_id": the id of the call to keep actived
 *
 * Async response data: NULL
 */
void ofono_call_private_chat(struct ofono_modem *modem,
                unsigned int call_id,
                response_cb cb,
                void *user_data);

/**
 * Sends DTMF tones to network
 *
 * "tones": the tones to sent
 *
 * Async response data: NULL
 */
void ofono_call_send_tones(struct ofono_modem *modem,
                const char *tones,
                response_cb cb,
                void *user_data);

/**
 * Get current calls
 *
 * Sync API
 */
tapi_bool ofono_call_get_calls(struct ofono_modem *modem,
                struct ofono_calls *calls);

/**
 * Get the information of sepcific call
 *
 * "call_id": the id of the call which information will be got
 * "info": returned call inforamtion
 *
 * Sync API
 */
tapi_bool ofono_call_get_call_info(struct ofono_modem *modem,
                unsigned int call_id,
                struct ofono_call_info *info);

/**
 * Get mute status
 *
 * sync API
 */
tapi_bool ofono_call_get_mute_status(struct ofono_modem *modem,
                tapi_bool *muted);

/**
 * Set mute status
 *
 * Async response data: NULL
 */
void ofono_call_set_mute_status(struct ofono_modem *modem,
                tapi_bool muted,
                response_cb cb,
                void *user_data);

/**
 * Get speaker volume: [0, 100]
 *
 * sync API
 */
tapi_bool ofono_call_get_speaker_volume(struct ofono_modem *modem,
                unsigned char *vol);

/**
 * Set speaker volume
 *
 * "vol": volume, should be [0, 100]
 *
 * Async response data: NULL
 */
void ofono_call_set_speaker_volume(struct ofono_modem *modem,
                unsigned char vol,
                response_cb cb,
                void *user_data);

/**
 * Get microphone volume: [0, 100]
 *
 * sync API
 */
tapi_bool ofono_call_get_microphone_volume(struct ofono_modem *modem,
                unsigned char *vol);

/**
 * Set microphone volume
 *
 * "vol": volume, should be [0, 100]
 *
 * Async response data: NULL
 */
void ofono_call_set_microphone_volume(struct ofono_modem *modem,
                unsigned char vol,
                response_cb cb,
                void *user_data);

/**
 * Set call volume by audio system (This API only for IMC modem)
 *
 * "vol": volume, should be [0, 100]
 *
 * Async response data: NULL
 */
void ofono_call_set_volume_by_alsa(struct ofono_modem *modem,
                unsigned char vol,
                response_cb cb,
                void *user_data);

/**
 * Set call sound path (This API only for IMC modem)
 *
 * "path": audio path, should be "speaker" or "earpiece"
 *
 * Async response data: NULL
 */
void ofono_call_set_sound_path(struct ofono_modem *modem,
                const char *path,
                response_cb cb,
                void *user_data);

#ifdef  __cplusplus
}
#endif

#endif
