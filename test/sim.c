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
#include "ofono-sim.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_sim_enable_pin();
static void test_sim_disable_pin();
static void test_sim_enter_pin();
static void test_sim_reset_pin();
static void test_sim_change_pin();
static void test_sim_get_info();

struct menu_info sim_menu[] = {
  {"ofono_sim_enable_pin", test_sim_enable_pin, main_menu, NULL},
  {"ofono_sim_disable_pin", test_sim_disable_pin, main_menu, NULL},
  {"ofono_sim_enter_pin", test_sim_enter_pin, main_menu, NULL},
  {"ofono_sim_reset_pin", test_sim_reset_pin, main_menu, NULL},
  {"ofono_sim_change_pin", test_sim_change_pin, main_menu, NULL},
  {"ofono_sim_get_info", test_sim_get_info, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_sim_enable_pin()
{
  char pin[64];

  printf("please input pin:\n");
  scanf("%s", pin);

  ofono_sim_enable_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_disable_pin()
{
  char pin[64];

  printf("please input pin:\n");
  scanf("%s", pin);

  ofono_sim_disable_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_enter_pin()
{
  char pin[64];

  printf("please input pin:\n");
  scanf("%s", pin);

  ofono_sim_enter_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_reset_pin()
{
  int type;
  char puk[64];
  char new_pin[64];

  printf("please input SIM type(1 - pin, 4 - pin2, 9 - puk, 11 - puk2):\n");
  scanf("%d", &type);

  printf("please input puk:\n");
  scanf("%s", puk);

  printf("please input new pin:\n");
  scanf("%s", new_pin);

  ofono_sim_reset_pin(g_modem, (enum pin_lock_type)type, puk, new_pin, NULL, NULL);
}

static void test_sim_change_pin()
{
  int type;
  char pin[64];
  char new_pin[64];

  printf("please input SIM type(1 - pin, 4 - pin2):\n");
  scanf("%d", &type);

  printf("please input old pin:\n");
  scanf("%s", pin);

  printf("please input new pin:\n");
  scanf("%s", new_pin);

  ofono_sim_change_pin(g_modem, (enum pin_lock_type)type, pin, new_pin, NULL, NULL);
}

static void test_sim_get_info()
{
  struct sim_info info;

  ofono_sim_get_info(g_modem, &info);
}