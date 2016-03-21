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

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_get_modems();
static void test_modem_init();
static void test_modem_deinit();
static void test_reg_notfication_callback();
static void test_un_notfication_callback();
static void test_has_interface();
static void test_deinit();

struct menu_info common_menu[] = {
  {"ofono_init", (menu_cb)ofono_init, main_menu, NULL},
  {"ofono_get_modems", test_get_modems, main_menu, NULL},
  {"ofono_modem_init", test_modem_init, main_menu, NULL},
  {"ofono_modem_deinit", test_modem_deinit, main_menu, NULL},
  {"ofono_register_notification_callback", test_reg_notfication_callback, main_menu, NULL},
  {"ofono_unregister_notification_callback", test_un_notfication_callback, main_menu, NULL},
  {"ofono_has_interface", test_has_interface, main_menu, NULL},
  {"ofono_deinit", test_deinit, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_get_modems()
{
  struct str_list *list = ofono_get_modems();
  int i;

  for (i = 0; i < list->count; i++)
    printf("Modem path: %s\n", list->data[i]);

  ofono_string_list_free(list);
}

static void test_modem_init()
{
  char buf[128];
  printf("please input modem path:\n");
  scanf("%s", buf);

  if (g_modem != NULL)
    ofono_modem_deinit(g_modem);

  g_modem = ofono_modem_init(buf);
}
static void test_modem_deinit()
{
  ofono_modem_deinit(g_modem);
  g_modem = NULL;
}

static void common_noti_cb(enum ofono_noti noti, void *data, void *user_data)
{
  printf("common_noti_cb: notification (%d) received ...\n", noti);
}

static void test_reg_notfication_callback()
{
  unsigned int noti;

  if (g_modem == NULL) {
    printf("please init modem first\n");
    return;
  }

  printf("please input notification id (0 for all):\n");
  scanf("%d", &noti);

  if (noti == 0) {
    int i = 0;
    while (i <= OFONO_NOTI_SAT_MAIN_MENU) {
      ofono_register_notification_callback(g_modem, i, common_noti_cb,
            NULL, NULL);
      i++;
    }
    return;
  }

  ofono_register_notification_callback(g_modem, noti, common_noti_cb,
          NULL, NULL);
}

static void test_un_notfication_callback()
{
  unsigned int noti;

  if (g_modem == NULL) {
    printf("please init modem first\n");
    return;
  }

  printf("please input notification id (0 for all):\n");
  scanf("%d", &noti);

  if (noti == 0) {
    int i = 0;
    while (i <= OFONO_NOTI_SAT_MAIN_MENU) {
      ofono_unregister_notification_callback(g_modem, i, common_noti_cb);
      i++;
    }
    return;
  }

  ofono_unregister_notification_callback(g_modem, noti, common_noti_cb);
}

static void test_has_interface()
{
    unsigned int api;

    if (g_modem == NULL) {
      printf("please init modem first\n");
      return;
    }

    printf("please input API id:\n");
    scanf("%d", &api);

    if ( ofono_has_interface(g_modem, api) )
      printf("API (%d) exists\n", api);
    else
      printf("API (%d) dosen't exist\n", api);
}

static void test_deinit()
{
  ofono_deinit();
  g_modem = NULL;
}
