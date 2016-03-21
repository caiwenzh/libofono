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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include "common.h"
#include "log.h"
#include "ofono-call.h"

static char *_call_id_to_path(struct ofono_modem *modem, int call_id)
{
  char *path;

  if (modem == NULL || modem->path == NULL) {
    tapi_error("");
    return NULL;
  }

  path = g_malloc(strlen(modem->path) + 32);
  g_sprintf(path, "%s/voicecall%02d", modem->path, call_id);
  return path;
}

static void _on_response_dial(GObject *obj,
                GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *path = NULL;
  unsigned int call_id;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(o)", &path);
  call_id = ofono_get_call_id_from_obj_path(path);
  tapi_debug("path: %s", path);

  CALL_RESP_CALLBACK(ret, &call_id, cbd);
  g_variant_unref(resp);
  g_free(path);
}

EXPORT_API tapi_bool ofono_call_get_ecc(struct ofono_modem *modem, struct str_list** ecc)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  int i = 0;

  tapi_debug("");

  if (modem == NULL || ecc == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_VOICE)) {
    tapi_error("OFONO_API_VOICE doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "EmergencyNumbers") == 0) {
      GVariantIter iter;
      char *em;
      g_variant_iter_init(&iter, var_val);
      *ecc = g_malloc(sizeof(struct str_list));
      (*ecc)->count = g_variant_iter_n_children(&iter);
      (*ecc)->data = g_malloc(sizeof(char *) * (*ecc)->count);
      while(g_variant_iter_next(&iter, "s", &em)) {
      	(*ecc)->data[i++] = em;
        tapi_info("Ecc: %s", em);
      }

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);
  return TRUE;
}

EXPORT_API void ofono_call_dial(struct ofono_modem *modem,
                char *number, enum clir_dev_status clir,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;
  char *str_clir;

  CHECK_PARAMETERS(modem && number, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  switch(clir) {
  case SS_CLIR_DEV_STATUS_ENABLED:
    str_clir = "enabled";
    break;
  case SS_CLIR_DEV_STATUS_DISABLED:
    str_clir = "disabled";
    break;
  default:
    str_clir = "";
    break;
  }

  tapi_debug("Numeber: %s, clir(%d): %s", number, clir, str_clir);

  val = g_variant_new("(ss)", number, str_clir);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "Dial", val, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_dial, cbd);
}

EXPORT_API void ofono_call_answer(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  struct ofono_calls calls;
  char *path;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  ofono_call_get_calls(modem, &calls);

  if (calls.count != 1) {
    tapi_warn("there should be only one incoming call");
    return;
  }

  if (calls.calls[0].status != CALL_STATUS_INCOMING) {
    tapi_warn("there should be one incoming call");
    return;
  }

  path = _call_id_to_path(modem, calls.calls[0].call_id);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path,
      OFONO_VOICECALL_IFACE, "Answer", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);

  g_free(path);
}

EXPORT_API void ofono_call_release_specific(struct ofono_modem *modem,
                unsigned int call_id, response_cb cb,
                void *user_data)
{
  struct response_cb_data *cbd;
  char *path;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  path = _call_id_to_path(modem, call_id);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path,
      OFONO_VOICECALL_IFACE, "Hangup", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);

  g_free(path);
}

EXPORT_API void ofono_call_release_all(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "HangupAll", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_swap(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "SwapCalls", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_release_and_answer(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "ReleaseAndAnswer", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_release_and_swap(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "ReleaseAndSwap", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_hold_and_answer(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "HoldAndAnswer", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_transfer(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{

  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "Transfer", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_deflect(struct ofono_modem *modem, char *number,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  struct ofono_calls calls;
  GVariant *var;
  char *path;
  int call_id = -1;
  int i;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Number: %s", number);

  ofono_call_get_calls(modem, &calls);

  if (calls.count <= 0) {
    tapi_error("no call exists");
    return;
  }

  for (i = 0; i < calls.count; i++) {
    if (calls.calls[i].status != CALL_STATUS_INCOMING ||
        calls.calls[i].status != CALL_STATUS_WAITING) {
      call_id = calls.calls[i].call_id;
      break;
    }
  }

  if (call_id < 0) {
    tapi_error("no incoming or waiting call found");
    return;
  }

  path = _call_id_to_path(modem, call_id);
  var = g_variant_new("(s)", number);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path,
      OFONO_VOICECALL_MANAGER_IFACE, "Deflect", var, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);

  g_free(path);
}

EXPORT_API void ofono_call_create_multiparty(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "CreateMultiparty", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_hangup_multiparty(struct ofono_modem *modem,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "HangupMultiparty", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_private_chat(struct ofono_modem *modem,
                unsigned int call_id, response_cb cb,
                void *user_data)
{
  struct response_cb_data *cbd;
  char *path;
  GVariant *var;

  tapi_debug("call_id: %d", call_id);

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  path = _call_id_to_path(modem, call_id);
  var = g_variant_new("(o)", path);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "PrivateChat", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);

  g_free(path);
}

EXPORT_API void ofono_call_send_tones(struct ofono_modem *modem,
                const gchar *tones, response_cb cb,
                void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem && tones, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("tones: %s", tones);

  val = g_variant_new("(s)", tones);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_VOICECALL_MANAGER_IFACE, "SendTones", val, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_call_get_calls(struct ofono_modem *modem,
                struct ofono_calls *calls)
{
  GError *error = NULL;
  GVariant *result, *val;
  GVariantIter *iter, *iter_val;
  char *path, *key;
  struct ofono_call_info *p_call;

  tapi_debug("");

  if (calls == NULL) {
    tapi_error("error parameter");
    return FALSE;
  }

  memset(calls, 0, sizeof(*calls));
  result = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
      modem->path, OFONO_VOICECALL_MANAGER_IFACE, "GetCalls",
      NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (result == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(result, "(a(oa{sv}))", &iter);

  calls->count = g_variant_iter_n_children(iter);
  if (calls->count == 0) {
    tapi_debug("No call");
    g_variant_iter_free(iter);
    g_variant_unref(result);
    return TRUE;
  }

  if (calls->count > MAX_CALL_PARTIES) {
    tapi_error("too much calls: %d", calls->count);
    calls->count = 0;
    g_variant_iter_free(iter);
    g_variant_unref(result);
    return FALSE;
  }

  p_call = calls->calls;
  while (g_variant_iter_loop(iter, "(oa{sv})", &path, &iter_val)) {
    p_call->call_id = ofono_get_call_id_from_obj_path(path);

    while (g_variant_iter_loop(iter_val, "{sv}", &key, &val)) {
      if (g_strcmp0(key, "LineIdentification") == 0) {
        const char *num;
        num = g_variant_get_string(val, NULL);
        g_strlcpy(p_call->line_id, num, sizeof(p_call->line_id));
        tapi_debug("cli: %s", num);
      } else if (g_strcmp0(key, "State") == 0) {
        const char *str;
        str = g_variant_get_string(val, NULL);
        p_call->status = ofono_str_to_call_status(str);
        tapi_debug("status: %s", str);
      } else if (g_strcmp0(key, "Name") == 0) {
        const char *name;
        name = g_variant_get_string(val, NULL);
        g_strlcpy(p_call->name, name, sizeof(p_call->name));
        tapi_debug("name: %s", name);
      } else if (g_strcmp0(key, "Multiparty") == 0) {
        g_variant_get(val, "b", &p_call->multiparty);
      } else if (g_strcmp0(key, "Emergency") == 0) {
        g_variant_get(val, "b", &p_call->emergency);
      }
    }

    tapi_debug("id: %d, status: %d, multiparty: %d, Emergency: %d",
        p_call->call_id, p_call->status,
        p_call->multiparty, p_call->emergency);

    p_call++;
  }
  g_variant_iter_free(iter);
  g_variant_unref(result);

  return TRUE;
}

EXPORT_API tapi_bool ofono_call_get_call_info(struct ofono_modem *modem,
                unsigned int call_id, struct ofono_call_info *info)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *path;
  const char *val;

  tapi_debug("");

  if (info == NULL) {
    tapi_error("error parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_VOICE)) {
    tapi_error("OFONO_API_VOICE doesn't exist");
    return FALSE;
  }

  path = _call_id_to_path(modem, call_id);
  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, path,
      OFONO_VOICECALL_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  g_free(path);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "LineIdentification") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->line_id, val, sizeof(info->line_id));
      tapi_debug("LineIdentification: %s", val);
    } else if (g_strcmp0(key, "State") == 0) {
        val = g_variant_get_string(var_val, NULL);
        info->status = ofono_str_to_call_status(val);
    } else if (g_strcmp0(key, "Name") == 0) {
        val = g_variant_get_string(var_val, NULL);
        g_strlcpy(info->name, val, sizeof(info->name));
        tapi_debug("Name %s", info->name);
    } else if (g_strcmp0(key, "Multiparty") == 0) {
        g_variant_get(var_val, "b", &info->multiparty);
        tapi_debug("Multiparty: %d", info->multiparty);
    } else if (g_strcmp0(key, "Emergency") == 0) {
        g_variant_get(var_val, "b", &info->emergency);
        tapi_debug("Emergency %d", info->emergency);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  return TRUE;
}

EXPORT_API tapi_bool ofono_call_get_mute_status(struct ofono_modem *modem,
                tapi_bool *muted)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (muted == NULL) {
    tapi_error("error parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_CALL_VOL)) {
    tapi_error("OFONO_API_CALL_VOL doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_CALL_VOLUME_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Muted") == 0) {
      g_variant_get(var_val, "b", muted);
      tapi_debug("Muted: %d", *muted);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  return TRUE;
}

EXPORT_API void ofono_call_set_mute_status(struct ofono_modem *modem,
                tapi_bool mute, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Mute: %d", mute);

  val = g_variant_new("(sv)", "Muted", g_variant_new_boolean(mute));
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_VOLUME_IFACE, "SetProperty", val, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static tapi_bool ofono_call_get_volume(struct ofono_modem *modem,
                const char *vol_name, unsigned char *vol)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (vol == NULL) {
    tapi_error("error parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_CALL_VOL)) {
    tapi_error("OFONO_API_CALL_VOL doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_CALL_VOLUME_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, vol_name) == 0) {
      g_variant_get(var_val, "y", vol);
      tapi_debug("Vol: %d", *vol);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  return TRUE;
}

EXPORT_API tapi_bool ofono_call_get_speaker_volume(struct ofono_modem *modem,
                unsigned char *vol)
{
  tapi_debug("");

  return ofono_call_get_volume(modem, "SpeakerVolume", vol);
}

EXPORT_API void ofono_call_set_speaker_volume(struct ofono_modem *modem,
                unsigned char vol, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Speaker vol: %d", vol);

  val = g_variant_new("(sv)", "SpeakerVolume", g_variant_new("y", vol));

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_VOLUME_IFACE, "SetProperty", val, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_call_get_microphone_volume(struct ofono_modem *modem,
                unsigned char *vol)
{
  tapi_debug("");

  return ofono_call_get_volume(modem, "MicrophoneVolume", vol);
}

EXPORT_API void ofono_call_set_microphone_volume(struct ofono_modem *modem,
                unsigned char vol, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Microphone vol: %d", vol);

  val = g_variant_new("(sv)", "MicrophoneVolume", g_variant_new("y", vol));

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_VOLUME_IFACE, "SetProperty", val, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_set_volume_by_alsa(struct ofono_modem *modem,
                unsigned char vol, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Volume(alsa): %d", vol);

  val = g_variant_new("(y)", vol);

  g_dbus_connection_call(modem->conn, OFONO_SERVER_SERVICE, "/",
      "org.ofono.server.AudioSettings", "SetVolumeLev", val,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_call_set_sound_path(struct ofono_modem *modem,
                const char *path, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  CHECK_PARAMETERS(modem && path, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("sound path: %s", path);

  val = g_variant_new("(s)", path);
  g_dbus_connection_call(modem->conn, OFONO_SERVER_SERVICE, "/",
      "org.ofono.server.AudioSettings", "EnablePCM", val,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}
