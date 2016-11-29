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
#include "ofono-modem.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_modem_get_online();
static void test_modem_set_online();
static void test_modem_get_powered();
static void test_modem_set_powered();
static void test_modem_get_info();

struct menu_info modem_menu[] = {
  {"ofono_modem_get_online", test_modem_get_online, main_menu, NULL},
  {"ofono_modem_set_online", test_modem_set_online, main_menu, NULL},
  {"ofono_modem_get_powered", test_modem_get_powered, main_menu, NULL},
  {"ofono_modem_set_powered", test_modem_set_powered, main_menu, NULL},
  {"ofono_modem_get_info", test_modem_get_info, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_modem_get_online()
{
  ofono_modem_get_online(g_modem);
}

static void test_modem_set_online()
{
  int online;

  printf("please input online setting (0 - offline, else online):\n");
  if (scanf("%d", &online) == EOF)
    return;

  ofono_modem_set_online(g_modem, online != 0, NULL, NULL);
}

static void test_modem_get_powered()
{
  ofono_modem_get_powered(g_modem);
}

static void test_modem_set_powered()
{
  int powered;

  printf("please input powered setting (0 - power off, else powered on):\n");
  if (scanf("%d", &powered) == EOF)
    return;

  ofono_modem_set_powered(g_modem, powered != 0, NULL, NULL);
}

static void test_modem_get_info()
{
  struct modem_info info;

  ofono_modem_get_info(g_modem, &info);
}