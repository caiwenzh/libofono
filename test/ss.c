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
#include "main.h"
#include "ofono-ss.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_ss_get_call_waiting();
static void test_ss_set_call_waiting();
static void test_ss_get_call_forward();
static void test_ss_set_call_forward();
static void test_ss_get_call_barring();
static void test_ss_set_call_barring();
static void test_ss_change_barring_password();
static void test_ss_get_cli_status();
static void test_ss_get_clir();
static void test_ss_set_clir();
static void test_ss_initiate_ussd_request();
static void test_ss_send_ussd_response();
static void test_ss_cancel_ussd_session();

struct menu_info ss_menu[] = {
  {"ofono_ss_get_call_waiting", test_ss_get_call_waiting, main_menu, NULL},
  {"ofono_ss_set_call_waiting", test_ss_set_call_waiting, main_menu, NULL},
  {"ofono_ss_get_call_forward", test_ss_get_call_forward, main_menu, NULL},
  {"ofono_ss_set_call_forward", test_ss_set_call_forward, main_menu, NULL},
  {"ofono_ss_get_call_barring", test_ss_get_call_barring, main_menu, NULL},
  {"ofono_ss_set_call_barring", test_ss_set_call_barring, main_menu, NULL},
  {"ofono_ss_change_barring_password", test_ss_change_barring_password, main_menu, NULL},
  {"ofono_ss_get_cli_status", test_ss_get_cli_status, main_menu, NULL},
  {"ofono_ss_get_clir", test_ss_get_clir, main_menu, NULL},
  {"ofono_ss_set_clir", test_ss_set_clir, main_menu, NULL},
  {"ofono_ss_initiate_ussd_request", test_ss_initiate_ussd_request, main_menu, NULL},
  {"ofono_ss_send_ussd_response", test_ss_send_ussd_response, main_menu, NULL},
  {"ofono_ss_cancel_ussd_session", test_ss_cancel_ussd_session, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_ss_get_call_waiting()
{
  ofono_ss_get_call_waiting(g_modem, NULL, NULL);
}

static void test_ss_set_call_waiting()
{
  int enabled;

  printf("please input call waiting setting (0 - disabled, else enabled):\n");
  if (scanf("%d", &enabled) == EOF)
    return;

  ofono_ss_set_call_waiting(g_modem, enabled != 0, NULL, NULL);
}

static void test_ss_get_call_forward()
{
  ofono_ss_get_call_forward(g_modem, NULL, NULL);
}

static void test_ss_set_call_forward()
{
  struct call_forward_setting setting;

  printf("please input call forwarding setting (0 - disabled, else enabled):\n");
  if (scanf("%d", &setting.enable) == EOF)
    return;

  printf("please input call forwarding condiation (0 - Unconditional, 1 - Busy, 2 - No reply, 3 - Unreachable):\n");
  if (scanf("%d", (int *)&setting.condition) == EOF)
    return;

  printf("please input call forwarding number:\n");
  if (scanf("%s", setting.num) == EOF)
    return;

  if (setting.condition == SS_CF_CONDITION_CFNRY) {
    printf("please input call forwarding no reply timeout:\n");
    if (scanf("%c", &setting.timeout) == EOF)
      return;
  }

  ofono_ss_set_call_forward(g_modem, &setting, NULL, NULL);
}

static void test_ss_get_call_barring()
{
  ofono_ss_get_call_barring(g_modem, NULL, NULL);
}

static void test_ss_set_call_barring()
{
  int enabled;
  int type;
  char pwd[64];

  printf("please input call barring setting (0 - disabled, else enabled):\n");
  if (scanf("%d", &enabled) == EOF)
    return;

  printf("please input call barring type (1 - All Outgoing, 2 - International Outgoing, 3 - Except home,\
    4 - All Incoming, 5 - Incoming when roam, 6 - All, 7 - All Outgoing, 8 - All Incoming):\n");
  if (scanf("%d", &type) == EOF)
    return;

  printf("please input call barring password:\n");
  if (scanf("%s", pwd) == EOF)
    return;

  ofono_ss_set_call_barring(g_modem, enabled != 0,
        (enum call_barring_type)type, pwd, NULL, NULL);
}

static void test_ss_change_barring_password()
{
  char pwd[64];
  char new_pwd[64];

  printf("please input old password:\n");
  if (scanf("%s", pwd) == EOF)
    return;

  printf("please input new password:\n");
  if (scanf("%s", new_pwd) == EOF)
    return;

  ofono_ss_change_barring_password(g_modem, pwd, new_pwd, NULL, NULL);
}

static void test_ss_get_cli_status()
{
  ofono_ss_get_cli_status(g_modem, NULL, NULL);
}

static void test_ss_get_clir()
{
  ofono_ss_get_clir(g_modem, NULL, NULL);
}

static void test_ss_set_clir()
{
  int status;

  printf("please input call line restriction setting(0 - Default, 1 - enabled, 2 - disable):\n");
  if (scanf("%d", &status) == EOF)
    return;

  ofono_ss_set_clir(g_modem, (enum clir_dev_status)status, NULL, NULL);
}

static void test_ss_initiate_ussd_request()
{
  char ussd[256];

  printf("please input USSD string:\n");
  if (scanf("%s", ussd) == EOF)
    return;

  ofono_ss_initiate_ussd_request(g_modem, ussd, NULL, NULL);
}

static void test_ss_send_ussd_response()
{
  char ussd[256];

  printf("please input USSD string:\n");
  if (scanf("%s", ussd) == EOF)
    return;

  ofono_ss_send_ussd_response(g_modem, ussd, NULL, NULL);
}

static void test_ss_cancel_ussd_session()
{
  ofono_ss_cancel_ussd_session(g_modem, NULL, NULL);
}
