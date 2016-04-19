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
#include "ofono-connman.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_connman_add_context();
static void test_connman_remove_context();
static void test_connman_set_context();
static void test_connman_get_context_info();
static void test_connman_get_contexts();
static void test_connman_activate_context();
static void test_connman_deactivate_context();
static void test_connman_deactivate_all_contexts();
static void test_connman_get_powered();
static void test_connman_set_powered();
static void test_connman_get_roaming_allowed();
static void test_connman_set_roaming_allowed();
static void test_connman_get_status();

struct menu_info connman_menu[] = {
  {"ofono_connman_add_context", test_connman_add_context, main_menu, NULL},
  {"ofono_connman_remove_context", test_connman_remove_context, main_menu, NULL},
  {"ofono_connman_set_context", test_connman_set_context, main_menu, NULL},
  {"ofono_connman_get_context_info", test_connman_get_context_info, main_menu, NULL},
  {"ofono_connman_get_contexts", test_connman_get_contexts, main_menu, NULL},
  {"ofono_connman_activate_context", test_connman_activate_context, main_menu, NULL},
  {"ofono_connman_deactivate_context", test_connman_deactivate_context, main_menu, NULL},
  {"ofono_connman_deactivate_all_contexts", test_connman_deactivate_all_contexts, main_menu, NULL},
  {"ofono_connman_get_powered", test_connman_get_powered, main_menu, NULL},
  {"ofono_connman_set_powered", test_connman_set_powered, main_menu, NULL},
  {"ofono_connman_get_roaming_allowed", test_connman_get_roaming_allowed, main_menu, NULL},
  {"ofono_connman_set_roaming_allowed", test_connman_set_roaming_allowed, main_menu, NULL},
  {"ofono_connman_get_status", test_connman_get_status, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_connman_add_context()
{
  int type;

  printf("please input PDP context type (1 - mms, 2 - internet, 3 - wap, 4 - ims):\n");
  scanf("%d", &type);

  ofono_connman_add_context(g_modem, (enum context_type)type, NULL, NULL);
}

static void test_connman_remove_context()
{
  char path[256];

  printf("please input PDP context object path (/<modem_name>/context<id>):\n");
  scanf("%s", path);

  ofono_connman_remove_context(g_modem, path, NULL, NULL);
}

static void test_connman_set_context()
{
  char path[256];
  char apn[64];
  struct pdp_context context;

  printf("please input PDP context object path (/<modem_name>/context<id>):\n");
  scanf("%s", path);

  printf("please input APN:\n");
  scanf("%s", apn);

  memset(&context, 0, sizeof(context));
  context.apn = apn;
  context.protocol = IP_PROTOCAL_IPV4;

  ofono_connman_set_context(g_modem, path, &context, NULL, NULL);
}

static void test_connman_get_context_info()
{
  struct pdp_context_info info;
  char path[256];

  printf("please input PDP context object path (/<modem_name>/context<id>):\n");
  scanf("%s", path);
  ofono_connman_get_context_info(g_modem, path, &info);
}

static void test_connman_get_contexts()
{
  struct str_list* list;

  ofono_connman_get_contexts(g_modem, &list);
  ofono_string_list_free(list);
}

static void test_connman_activate_context()
{
  char path[256];

  printf("please input PDP context object path (/<modem_name>/context<id>):\n");
  scanf("%s", path);

  ofono_connman_activate_context(g_modem, path, NULL, NULL);
}

static void test_connman_deactivate_context()
{
  char path[256];

  printf("please input PDP context object path (/<modem_name>/context<id>):\n");
  scanf("%s", path);

  ofono_connman_deactivate_context(g_modem, path, NULL, NULL);
}
static void test_connman_deactivate_all_contexts()
{
  ofono_connman_deactivate_all_contexts(g_modem, NULL, NULL);
}

static void test_connman_get_powered()
{
  tapi_bool powered;

  ofono_connman_get_powered(g_modem, &powered);
}

static void test_connman_set_powered()
{
  int powered;

  printf("please input connman powered (0 - powered off, else powered on):\n");
  scanf("%d", &powered);

  ofono_connman_set_powered(g_modem, powered != 0, NULL, NULL);
}

static void test_connman_get_roaming_allowed()
{
  tapi_bool allowed;

  ofono_connman_get_roaming_allowed(g_modem, &allowed);
}

static void test_connman_set_roaming_allowed()
{
  int allowed;

  printf("please input connman roaming setting (0 - not allowed, else allowed):\n");
  scanf("%d", &allowed);

  ofono_connman_set_roaming_allowed(g_modem, allowed != 0, NULL, NULL);
}

static void test_connman_get_status()
{
  struct ps_reg_status status;

  ofono_connman_get_status(g_modem, &status);
}