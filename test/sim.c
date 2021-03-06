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
static void test_sim_io();

struct menu_info sim_menu[] = {
  {"ofono_sim_enable_pin", test_sim_enable_pin, main_menu, NULL},
  {"ofono_sim_disable_pin", test_sim_disable_pin, main_menu, NULL},
  {"ofono_sim_enter_pin", test_sim_enter_pin, main_menu, NULL},
  {"ofono_sim_reset_pin", test_sim_reset_pin, main_menu, NULL},
  {"ofono_sim_change_pin", test_sim_change_pin, main_menu, NULL},
  {"ofono_sim_get_info", test_sim_get_info, main_menu, NULL},
  {"ofono_sim_io", test_sim_io, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_sim_enable_pin()
{
  char pin[64];

  printf("please input pin:\n");
  if (scanf("%s", pin) == EOF)
    return;

  ofono_sim_enable_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_disable_pin()
{
  char pin[64];

  printf("please input pin:\n");
  if (scanf("%s", pin) == EOF)
    return;

  ofono_sim_disable_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_enter_pin()
{
  char pin[64];

  printf("please input pin:\n");
  if (scanf("%s", pin) == EOF)
    return;

  ofono_sim_enter_pin(g_modem, PIN_LOCK_SIM_PIN, pin, NULL, NULL);
}

static void test_sim_reset_pin()
{
  int type;
  char puk[64];
  char new_pin[64];

  printf("please input SIM type(1 - pin, 4 - pin2, 9 - puk, 11 - puk2):\n");
  if (scanf("%d", &type) == EOF)
    return;

  printf("please input puk:\n");
  if (scanf("%s", puk) == EOF)
    return;

  printf("please input new pin:\n");
  if(scanf("%s", new_pin) == EOF)
    return;

  ofono_sim_reset_pin(g_modem, (enum pin_lock_type)type, puk, new_pin, NULL, NULL);
}

static void test_sim_change_pin()
{
  int type;
  char pin[64];
  char new_pin[64];

  printf("please input SIM type(1 - pin, 4 - pin2):\n");
  if (scanf("%d", &type) == EOF)
    return;

  printf("please input old pin:\n");
  if (scanf("%s", pin) == EOF)
    return;

  printf("please input new pin:\n");
  if (scanf("%s", new_pin) == EOF)
    return;

  ofono_sim_change_pin(g_modem, (enum pin_lock_type)type, pin, new_pin, NULL, NULL);
}

static void test_sim_get_info()
{
  struct sim_info info;

  ofono_sim_get_info(g_modem, &info);
}

static void test_sim_io()
{
  struct sim_io_req req;
  char content[1024];

  printf("please input cmd, file_id, p1, p2, p3 (sperate by comma):\n");
  if (scanf("%d,%d,%hhd,%hhd,%hhd", (int *)&req.cmd, &req.fid, &req.p1, &req.p2, &req.p3) == EOF)
    return;

  req.data = "";
  if (req.cmd == 214 || req.cmd == 220) {
  	 printf("please input content to write (hex):\n");
     if (scanf("%s", content) == EOF)
       return;
	 req.data = content;
  }

  req.path = "";

  printf("%d %d %d %d %d\n", req.cmd, req.fid, req.p1, req.p2, req.p3);
  ofono_sim_io(g_modem, &req, NULL, NULL);
}
