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
#ifndef __OFONO_SAT_H__
#define __OFONO_SAT_H__

#include "ofono-common.h"

struct ofono_sat_agent;

enum sat_result {
  SAT_RESP_OK,
  SAT_RESP_GO_BACK,
  SAT_RESP_END_SESSION,
  SAT_RESP_SCREEN_BUSY
};

enum sat_response_type {
  SAT_RESP_TYPE_VOID,
  SAT_RESP_TYPE_BOOL,
  SAT_RESP_TYPE_BYTE,
  SAT_RESP_TYPE_STRING
};

struct sat_menu_item {
  char *text;
  unsigned char icon_id;
};

struct sat_main_menu {
  char *title;
  unsigned char icon;
  int item_count;
  struct sat_menu_item *items;
};

struct sat_command_select_item {
  char *title;
  unsigned char icon_id;
  int item_count;
  struct sat_menu_item *items;
  short int def_sel; /* default selected item */
};

struct sat_command_display_text {
  char *text;
  unsigned char icon_id;
  tapi_bool urgent;
};

struct sat_command_get_input {
  char *alpha; /* text to display */
  unsigned char icon_id;
  char *default_str;
  unsigned char min; /* min characters */
  unsigned char max; /* max characters */
  tapi_bool hide;
  tapi_bool is_num;
};

struct sat_command_launch_browser {
  char *url;
  char *info; /* information to display */
  unsigned char icon_id;
};

struct sat_command_get_inkey {
  char *text;
  tapi_bool is_num;
  tapi_bool quick_resp;
  tapi_bool yes_no; /* request a "Yes/No" response */
  unsigned char icon_id;
};

struct sat_display_info {
  char *text;
  unsigned char icon_id;
};

struct sat_command_play_tone {
  char *tone;
  char *text;
  unsigned char icon_id;
  tapi_bool loop;
};

typedef tapi_bool (*ofono_sat_handle) (const void *data, void *user_data);

/*
  The callback is called to interact with UI layer with specific inforation
  (the information data struct see below comment). Generally, a terminal
  response is requred to send back with the result (enum sat_result) and
  data (if result is SAT_RESP_OK, otherwise, it isn't required).
*/
struct sat_agent_callbacks {
  /* Display a menu. Data struct sat_command_select_item, must send
     response (SAT_RESP_TYPE_BYTE, menu index) after user tack action */
  ofono_sat_handle request_selection;

  /* Display text till user take action or "cancel" callback is callaed.
     Data struct sat_command_display_text.
     Response data: SAT_RESP_TYPE_VOID */
  ofono_sat_handle display_text;

  /* Get user input. Data struct sat_command_get_input
     Response data: SAT_RESP_TYPE_STRING (the string user input) */
  ofono_sat_handle request_input;

  /* Get input(a key or equivalent). Data struct sat_command_get_inkey
     Response data: SAT_RESP_TYPE_STRING (the key(s) user input) */
  ofono_sat_handle request_key;

  /* Display info and play ringtone. Data struct sat_command_play_tone
     Response data: SAT_RESP_TYPE_VOID */
  ofono_sat_handle play_tone;

  /* Display the info till 'cancel' callback is called or user takes
     action. Data struct sat_display_info. It isn't required to send
     response data if user doesn't take action */
  ofono_sat_handle display_action_information;

  /* Confrim launch browser. Data struct sat_command_launch_browser
     Response data: SAT_RESP_TYPE_BOOL (Whether launch browser) */
  ofono_sat_handle confirm_launch_browser;

  /* Display the info till 'cancel' callback is called or user end the
     session. Data struct sat_display_info. It isn't required to send
     response data except user ends the session.*/
  ofono_sat_handle display_action;

  /* Previous a text display UI is show and user doesn't take any action,
     this callback terminal the UI. Data struct N/A, it isn't required
     to send terminal response. */
  ofono_sat_handle cancel;

  /* Ofonod becomes unavailable (it is crashed or modem is removed etc).
     This callback take necessary action to handle this exception.
     Data struct N/A, it isn't required to send terminal response. */
  ofono_sat_handle release;
};

/**
 * Init a sat agent
 *
 * "modem": the modem to use
 * "callbacks": UI callbacks to process sat ui information
 * "user_data": the "user_data" will be passed when UI callback is invoked
 *              (depends on user propose, if not need, set it as NULL)
 *
 * Sync API
 */
struct ofono_sat_agent* ofono_sat_init_agent(struct ofono_modem *modem,
      struct sat_agent_callbacks *callbacks,
      void *user_data);

/**
 * Deinit a sat agent
 *
 * Sync API
 */
void ofono_sat_deinit_agent(struct ofono_sat_agent *agent);

/**
 * Get sat main menu
 *
 * Sync API (Note: free "menu" memory)
 */
tapi_bool ofono_sat_get_main_menu(struct ofono_modem *modem,
      struct sat_main_menu *menu);

/**
 * Selects an main menu item
 *
 * "index": the item index (not item id), start from 0
 *
 * Async response data: NULL
 */
void ofono_sat_select_item(struct ofono_modem *modem,
      unsigned char index,
      response_cb cb,
      void *user_data);

/**
 * Send response
 *
 * "result": UI operation result
 * "type": response data type
 * "data": reponse data
 *
 * Async response data: NULL
 */
void ofono_sat_send_response(struct ofono_sat_agent *agent,
      enum sat_result result,
      enum sat_response_type type,
      void *data);

#endif
