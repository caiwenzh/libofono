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
#include "pthread.h"

struct ofono_modem *g_modem;
struct menu_info *cur_menu;

extern struct menu_info common_menu[];
extern struct menu_info modem_menu[];
extern struct menu_info call_menu[];
extern struct menu_info sms_menu[];
extern struct menu_info network_menu[];
extern struct menu_info ss_menu[];
extern struct menu_info connman_menu[];
extern struct menu_info sim_menu[];
extern struct menu_info sat_menu[];
extern struct menu_info phonebook_menu[];

struct menu_info main_menu[] = {
  {"Common", NULL, NULL, common_menu},
  {"Modem", NULL, NULL, modem_menu},
  {"Call", NULL, NULL, call_menu},
  {"SMS", NULL, NULL, sms_menu},
  {"Network", NULL, NULL, network_menu},
  {"SS", NULL, NULL, ss_menu},
  {"Connman", NULL, NULL, connman_menu},
  {"SIM", NULL, NULL, sim_menu},
  {"STK", NULL, NULL, sat_menu},
  {"Phonebook", NULL, NULL, phonebook_menu},
  {NULL, NULL, NULL, NULL}
};

void show_menu(struct menu_info *menu)
{
  int index = -1;
  int i;
  struct menu_info *menu_item = menu;

  printf("\n");
  i = 0;
  while (menu_item != NULL && menu_item->display_info != NULL) {
    printf(" %d.%s\n", ++i, menu_item->display_info);
    menu_item++;
  }

  printf("\nPlease input the menu index, '0' back to previous menu\n");
  if (scanf("%d", &index) <= 0 || index < 0) {
    getchar();

    printf("<** please input valid menu item index **!>");
    show_menu(menu);
  }

  if(index == 0) {
    if (menu->previous != NULL)
      show_menu(menu->previous);
    else
      show_menu(menu);
    return;
  }

  menu_item = menu;
  i = 0;
  while (menu_item != NULL && menu_item->display_info != NULL) {
    if(++i == index) {
      if(menu_item->next != NULL) {
        show_menu(menu_item->next);
        return;
      } else if(menu_item->cb != NULL) {
        menu_item->cb();
        show_menu(menu);
        return;
      }
    }
    menu_item++;
  }

  printf("<** please input valid menu item index **!>");
  show_menu(menu);
}

void *menu_thread(void *user_data)
{
  show_menu(main_menu);
  return NULL;
}

static void on_modems_changed(const char *modem, tapi_bool add)
{
  if (!g_modem && add)
		g_modem = ofono_modem_init(modem);

	if (!add) {
		struct str_list *modems = ofono_get_modems();
    if (modems->count <= 0) {
			ofono_modem_deinit(g_modem);
			g_modem = NULL;
    }

		ofono_string_list_free(modems);
	}
}

int main(int argc, char** argv)
{
  struct str_list *modems;
  pthread_t pid;
  GMainLoop *mainloop;

  /* Auto init a modem for use */
  ofono_init();
  modems = ofono_get_modems();
  if (modems->count > 0)
    g_modem = ofono_modem_init(modems->data[0]);
  ofono_string_list_free(modems);

  ofono_set_modems_changed_callback(on_modems_changed);

#if !GLIB_CHECK_VERSION(2,35,0)
  g_type_init();
#endif

  mainloop = g_main_loop_new(NULL, FALSE);

  pthread_create(&pid, NULL, menu_thread, NULL);

  g_main_loop_run(mainloop);
  return 0;
}

