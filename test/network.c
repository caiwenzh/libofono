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
#include "ofono-network.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_network_get_registration_info();
static void test_network_get_operator_name();
static void test_network_get_signal_strength();
static void test_network_get_network_selection_mode();
static void test_network_get_mode();
static void test_network_set_mode();
static void test_network_register();
static void test_network_auto_register();
static void test_network_scan_operators();

struct menu_info network_menu[] = {
  {"ofono_network_get_registration_info", test_network_get_registration_info, main_menu, NULL},
  {"ofono_network_get_operator_name", test_network_get_operator_name, main_menu, NULL},
  {"ofono_network_get_signal_strength", test_network_get_signal_strength, main_menu, NULL},
  {"ofono_network_get_network_selection_mode", test_network_get_network_selection_mode, main_menu, NULL},
  {"ofono_network_get_mode", test_network_get_mode, main_menu, NULL},
  {"ofono_network_set_mode", test_network_set_mode, main_menu, NULL},
  {"ofono_network_register", test_network_register, main_menu, NULL},
  {"ofono_network_auto_register", test_network_auto_register, main_menu, NULL},
  {"ofono_network_scan_operators", test_network_scan_operators, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_network_get_registration_info()
{
  struct registration_info info;

  ofono_network_get_registration_info(g_modem, &info);
}

static void test_network_get_operator_name()
{
  char *operator_name;

  ofono_network_get_operator_name(g_modem, &operator_name);
}

static void test_network_get_signal_strength()
{
  unsigned char strength;

  ofono_network_get_signal_strength(g_modem, &strength);
}

static void test_network_get_network_selection_mode()
{
  enum network_selection_mode mode;

  ofono_network_get_network_selection_mode(g_modem, &mode);
}

static void test_network_get_mode()
{
  ofono_network_get_mode(g_modem, NULL, NULL);
}

static void test_network_set_mode()
{
  int mode;

  printf("please input network mode (1 - auto, 2 - 2G, 4 - 3G, 8 - 4G):\n");
  scanf("%d", &mode);

  ofono_network_set_mode(g_modem, (enum network_mode)mode, NULL, NULL);
}

static void test_network_register()
{
  char plmn[256];

  printf("please input plmn (MCCMNC):\n");
  scanf("%s", plmn);

  ofono_network_register(g_modem, plmn, NULL, NULL);
}

static void test_network_auto_register()
{
  ofono_network_auto_register(g_modem, NULL, NULL);
}

static void test_network_scan_operators()
{
  ofono_network_scan_operators(g_modem, NULL, NULL);
}
