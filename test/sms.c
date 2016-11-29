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
#include "ofono-sms.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_sms_get_sca();
static void test_sms_set_sca();
static void test_sms_get_delivery_report();
static void test_sms_set_delivery_report();
static void test_sms_send_sms();
static void test_sms_get_cbs_config();
static void test_sms_set_cbs_powered();
static void test_sms_set_cbs_topics();

struct menu_info sms_menu[] = {
  {"ofono_sms_get_sca", test_sms_get_sca, main_menu, NULL},
  {"ofono_sms_set_sca", test_sms_set_sca, main_menu, NULL},
  {"ofono_sms_get_delivery_report", test_sms_get_delivery_report, main_menu, NULL},
  {"ofono_sms_set_delivery_report", test_sms_set_delivery_report, main_menu, NULL},
  {"ofono_sms_send_sms", test_sms_send_sms, main_menu, NULL},
  {"ofono_sms_get_cbs_config", test_sms_get_cbs_config, main_menu, NULL},
  {"ofono_sms_set_cbs_powered", test_sms_set_cbs_powered, main_menu, NULL},
  {"ofono_sms_set_cbs_topics", test_sms_set_cbs_topics, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_sms_get_sca()
{
  ofono_sms_get_sca(g_modem, NULL, NULL);
}

static void test_sms_set_sca()
{
  char sca[128];

  printf("please input SMS service center address:\n");
  if (scanf("%s", sca) == EOF)
    return;

  ofono_sms_set_sca(g_modem, sca, NULL, NULL);
}

static void test_sms_get_delivery_report()
{
  ofono_sms_get_delivery_report(g_modem, NULL, NULL);
}

static void test_sms_set_delivery_report()
{
  int report;

  printf("please input SMS delivery report setting (0 - disabled, else enable):\n");
  if (scanf("%d", &report) == EOF)
    return;

  ofono_sms_set_delivery_report(g_modem, report != 0, NULL, NULL);
}

static void test_sms_send_sms()
{
  char num[128];
  char text[256];

  printf("please input number and SMS content (e.g:10010 hi):\n");
  if (scanf("%s %s", num, text) == EOF)
    return;

  ofono_sms_send_sms(g_modem, num, text, NULL, NULL);
}

static void test_sms_get_cbs_config()
{
  ofono_sms_get_cbs_config(g_modem, NULL, NULL);
}

static void test_sms_set_cbs_powered()
{
  int powered;

  printf("please input Cell broadcast message setting (0 - disabled, else enable):\n");
  if (scanf("%d", &powered) == EOF)
    return;

  ofono_sms_set_cbs_powered(g_modem, powered != 0, NULL, NULL);
}

static void test_sms_set_cbs_topics()
{
  char topics[128];

  printf("please input Cell broadcast message topics (e.g: 0,1,5,320-478,922 ):\n");
  if (scanf("%s", topics) == EOF)
    return;

  ofono_sms_set_cbs_topics(g_modem, topics, NULL, NULL);
}