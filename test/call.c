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
#include "ofono-call.h"

extern struct ofono_modem *g_modem;
extern struct menu_info main_menu[];

static void test_call_get_ecc();
static void test_call_dial();
static void test_call_answer();
static void test_call_release_specific();
static void test_call_release_all();
static void test_call_swap();
static void test_call_release_and_answer();
static void test_call_release_and_swap();
static void test_call_hold_and_answer();
static void test_call_transfer();
static void test_call_deflect();
static void test_call_create_multiparty();
static void test_call_hangup_multiparty();
static void test_call_private_chat();
static void test_call_send_tones();
static void test_call_get_calls();
static void test_call_get_call_info();
static void test_call_get_mute_status();
static void test_call_set_mute_status();
static void test_call_get_speaker_volume();
static void test_call_set_speaker_volume();
static void test_call_get_microphone_volume();
static void test_call_set_microphone_volume();
static void test_call_set_volume_by_alsa();
static void test_call_set_sound_path();

struct menu_info call_menu[] = {
  {"ofono_call_get_ecc", test_call_get_ecc, main_menu, NULL},
  {"ofono_call_dial", test_call_dial, main_menu, NULL},
  {"ofono_call_answer", test_call_answer, main_menu, NULL},
  {"ofono_call_release_specific", test_call_release_specific, main_menu, NULL},
  {"ofono_call_release_all", test_call_release_all, main_menu, NULL},
  {"ofono_call_swap", test_call_swap, main_menu, NULL},
  {"ofono_call_release_and_answer", test_call_release_and_answer, main_menu, NULL},
  {"ofono_call_release_and_swap", test_call_release_and_swap, main_menu, NULL},
  {"ofono_call_hold_and_answer", test_call_hold_and_answer, main_menu, NULL},
  {"ofono_call_transfer", test_call_transfer, main_menu, NULL},
  {"ofono_call_deflect", test_call_deflect, main_menu, NULL},
  {"ofono_call_create_multiparty", test_call_create_multiparty, main_menu, NULL},
  {"ofono_call_hangup_multiparty", test_call_hangup_multiparty, main_menu, NULL},
  {"ofono_call_private_chat", test_call_private_chat, main_menu, NULL},
  {"ofono_call_send_tones", test_call_send_tones, main_menu, NULL},
  {"ofono_call_get_calls", test_call_get_calls, main_menu, NULL},
  {"ofono_call_get_call_info", test_call_get_call_info, main_menu, NULL},
  {"ofono_call_get_mute_status", test_call_get_mute_status, main_menu, NULL},
  {"ofono_call_set_mute_status", test_call_set_mute_status, main_menu, NULL},
  {"ofono_call_get_speaker_volume", test_call_get_speaker_volume, main_menu, NULL},
  {"ofono_call_set_speaker_volume", test_call_set_speaker_volume, main_menu, NULL},
  {"ofono_call_get_microphone_volume", test_call_get_microphone_volume, main_menu, NULL},
  {"ofono_call_set_microphone_volume", test_call_set_microphone_volume, main_menu, NULL},
  {"ofono_call_set_volume_by_alsa", test_call_set_volume_by_alsa, main_menu, NULL},
  {"ofono_call_set_sound_path", test_call_set_sound_path, main_menu, NULL},
  {NULL, NULL, NULL, NULL}
};

static void test_call_get_ecc()
{
  struct str_list *list = NULL;

  ofono_call_get_ecc(g_modem, &list);
  ofono_string_list_free(list);
}

static void test_call_dial()
{
  char num[128];
  int clir;
  enum clir_dev_status cds;

  printf("please input number:\n");
  scanf("%s", num);

  printf("please input CLIR setting (0 - disable, else enable):\n");
  scanf("%d", &clir);

  if (clir == 0)
    cds = SS_CLIR_DEV_STATUS_DISABLED;
  else
    cds = SS_CLIR_DEV_STATUS_ENABLED;

  ofono_call_dial(g_modem, num, cds, NULL, NULL);
}

static void test_call_answer()
{
  ofono_call_answer(g_modem, NULL, NULL);
}

static void test_call_release_specific()
{
  int call_id;

  printf("please input the id of the call to release:\n");
  scanf("%d", &call_id);

  ofono_call_release_specific(g_modem, call_id, NULL, NULL);
}

static void test_call_release_all()
{
  ofono_call_release_all(g_modem, NULL, NULL);
}

static void test_call_swap()
{
  ofono_call_swap(g_modem, NULL, NULL);
}

static void test_call_release_and_answer()
{
  ofono_call_release_and_answer(g_modem, NULL, NULL);
}

static void test_call_release_and_swap()
{
  ofono_call_release_and_swap(g_modem, NULL, NULL);
}

static void test_call_hold_and_answer()
{
  ofono_call_hold_and_answer(g_modem, NULL, NULL);
}

static void test_call_transfer()
{
  ofono_call_transfer(g_modem, NULL, NULL);
}

static void test_call_deflect()
{
  char number[64];

  printf("please input the destination number:\n");
  scanf("%s", number);

  ofono_call_deflect(g_modem, number, NULL, NULL);
}

static void test_call_create_multiparty()
{
  ofono_call_create_multiparty(g_modem, NULL, NULL);
}

static void test_call_hangup_multiparty()
{
  ofono_call_hangup_multiparty(g_modem, NULL, NULL);
}

static void test_call_private_chat()
{
  int call_id;

  printf("please input the id of the call will private chat with:\n");
  scanf("%d", &call_id);

  ofono_call_private_chat(g_modem, call_id, NULL, NULL);
}

static void test_call_send_tones()
{
  char tones[64];

  printf("please input the tones to send:\n");
  scanf("%s", tones);

  ofono_call_send_tones(g_modem, tones, NULL, NULL);
}

static void test_call_get_calls()
{
  struct ofono_calls calls;

  ofono_call_get_calls(g_modem, &calls);
}

static void test_call_get_call_info()
{
  struct ofono_call_info call_info;
  int call_id;

  printf("please input call id:\n");
  scanf("%d", &call_id);

  ofono_call_get_call_info(g_modem, call_id, &call_info);
}

static void test_call_get_mute_status()
{
  tapi_bool muted;

  ofono_call_get_mute_status(g_modem, &muted);
}

static void test_call_set_mute_status()
{
  int mute;

  printf("please input mute status (0 - unmute, else mute):\n");
  scanf("%d", &mute);

  ofono_call_set_mute_status(g_modem, mute != 0, NULL, NULL);
}

static void test_call_get_speaker_volume()
{
  unsigned char vol;

  ofono_call_get_speaker_volume(g_modem, &vol);
}

static void test_call_set_speaker_volume()
{
  int vol;

  printf("please input volume value [0 - 100]:\n");
  scanf("%d", &vol);

  ofono_call_set_speaker_volume(g_modem, (unsigned char)vol, NULL, NULL);
}

static void test_call_get_microphone_volume()
{
  unsigned char vol;

  ofono_call_get_microphone_volume(g_modem, &vol);
}

static void test_call_set_microphone_volume()
{
  int vol;

  printf("please input volume value [0 - 100]:\n");
  scanf("%d", &vol);

  ofono_call_set_microphone_volume(g_modem, (unsigned char)vol, NULL, NULL);
}

static void test_call_set_volume_by_alsa()
{
  int vol;

  printf("please input volume value [0 - 100]:\n");
  scanf("%d", &vol);

  ofono_call_set_volume_by_alsa(g_modem, (unsigned char)vol, NULL, NULL);
}

static void test_call_set_sound_path()
{
  char* path;
  int i_path;

  printf("<** Note: should cowork with application layer, it requires setting"
  "audio path through audio system too. **>\n");
  printf("please input sound path (0 - speaker, else earpiece):\n");
  scanf("%d", &i_path);

  if (i_path == 0)
    path = "speaker";
  else
    path = "earpiece";

  ofono_call_set_sound_path(g_modem, path, NULL, NULL);
}