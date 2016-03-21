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
#include "ofono-sat.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_sat_get_main_menu();
static void test_sat_select_item();

struct menu_info sat_menu[] = {
  {"ofono_sat_get_main_menu", test_sat_get_main_menu, main_menu, NULL},
  {"ofono_sat_select_item", test_sat_select_item, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_sat_get_main_menu()
{
  struct sat_main_menu menu;

  ofono_sat_get_main_menu(g_modem, &menu);
}

static void test_sat_select_item()
{
  ofono_sat_select_item(g_modem, 0, NULL, NULL);
}