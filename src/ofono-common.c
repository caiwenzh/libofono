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


#include "log.h"
#include "common.h"
#include "ofono-common.h"
#include "ofono-modem.h"
#include "ofono-call.h"
#include "ofono-sms.h"
#include "ofono-network.h"
#include "ofono-connman.h"

static GDBusConnection *s_bus_conn = NULL;
static guint s_modem_added_watch = 0;
static guint s_modem_removed_watch = 0;

static modems_changed_cb s_modems_changed_cb = NULL;

struct ofono_api_map {
  enum ofono_api api_bit;
  const gchar *api_name;
};

static const struct ofono_api_map OFONO_API_MAPS[] = {
  {OFONO_API_SIM, OFONO_SIM_MANAGER_IFACE},
  {OFONO_API_NETREG, OFONO_NETWORK_REGISTRATION_IFACE},
  {OFONO_API_VOICE, OFONO_VOICECALL_MANAGER_IFACE},
  {OFONO_API_MSG, OFONO_MESSAGE_MANAGER_IFACE},
  {OFONO_API_MSG_WAITING, OFONO_MESSAGE_WAITING_IFACE},
  {OFONO_API_SMART_MSG, OFONO_SMART_MESSAGE_IFACE},
  {OFONO_API_STK, OFONO_STK_IFACE},
  {OFONO_API_CALL_FW, OFONO_CALL_FORWARDING_IFACE},
  {OFONO_API_CALL_VOL, OFONO_CALL_VOLUME_IFACE},
  {OFONO_API_CALL_METER, OFONO_CALL_METER_IFACE},
  {OFONO_API_CALL_SET, OFONO_CALL_SETTINGS_IFACE},
  {OFONO_API_CALL_BAR, OFONO_CALL_BARRING_IFACE},
  {OFONO_API_SUPPL_SERV, OFONO_SUPPLEMENTARY_SERVICES_IFACE},
  {OFONO_API_TXT_TEL, OFONO_TEXT_TELEPHONY_IFACE},
  {OFONO_API_CELL_BROAD, OFONO_CELL_BROADCAST_IFACE},
  {OFONO_API_CONNMAN, OFONO_CONNMAN_IFACE},
  {OFONO_API_PUSH_NOTIF, OFONO_PUSH_NOTIFICATION_IFACE},
  {OFONO_API_PHONEBOOK, OFONO_PHONEBOOK_IFACE},
  {OFONO_API_ASN, OFONO_GNSS_IFACE},
  {OFONO_API_RADIO_SETTING, OFONO_RADIO_SETTINGS_IFACE},
  {0, NULL},
};

struct noti_cb_data {
  noti_cb cb;
  void *user_data;
  destroy_notify user_data_free_func;
};

struct ofono_noti_data {
  struct ofono_modem *modem;
  enum ofono_noti noti;
  guint watch;
  GList *cb_list;
};

static GDBusConnection *_get_dbus_connection()
{
  GError *error = NULL;
  char *addr;

  if (s_bus_conn != NULL)
    return s_bus_conn;

  g_type_init();

  addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
  if (addr == NULL) {
    tapi_error("fail to get dbus addr: %s\n", error->message);
    g_free(error);
    return NULL;
  }

  s_bus_conn = g_dbus_connection_new_for_address_sync(addr,
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
        G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
        NULL, NULL, &error);

  if (s_bus_conn == NULL) {
    tapi_error("fail to create dbus connection: %s\n", error->message);
    g_free(error);
  }

  return s_bus_conn;
}

EXPORT_API struct str_list *ofono_get_modems()
{
  struct str_list *modems;
  GError *error = NULL;
  GVariant *var_resp, *var_val;
  GVariantIter *iter;
  char *path;
  int i = 0;

  modems = g_malloc(sizeof(struct str_list));
  modems->count = 0;
  modems->data = NULL;

  var_resp = g_dbus_connection_call_sync(s_bus_conn,
          OFONO_SERVICE, OFONO_MANAGER_PATH,
          OFONO_MANAGER_IFACE, "GetModems", NULL, NULL,
          G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_resp == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return modems;
  }

  g_variant_get(var_resp, "(a(oa{sv}))", &iter);
  modems->count = g_variant_iter_n_children(iter);
  modems->data = g_malloc(sizeof(char *) * modems->count);
  while (g_variant_iter_next(iter, "(o@a{sv})", &path, &var_val)) {
    modems->data[i++] = path;
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_resp);

  return modems;
}

EXPORT_API void ofono_string_list_free(struct str_list *list)
{
  int i;

  if (!list)
    return;

  for (i = 0; i < list->count; i++)
    g_free(list->data[i]);

  g_free(list->data);
  g_free(list);
}

static guint32 _modem_interfaces_extract(GVariant *array)
{
  guint32 interfaces = 0;
  GVariant *value;
  GVariantIter iter;

  tapi_debug("");

  g_variant_iter_init(&iter, array);
  while ((value = g_variant_iter_next_value(&iter)) != NULL) {
    const struct ofono_api_map *maps;
    const gchar *iface = g_variant_get_string(value, NULL);

    tapi_info("Interface: %s", iface);

    for (maps = OFONO_API_MAPS; maps->api_name != NULL; maps++) {
      if (g_strcmp0(iface, maps->api_name) == 0) {
        interfaces |= (1 << maps->api_bit);
        break;
      }
    }
    g_variant_unref(value);
  }

  return interfaces;
}

static void _update_modem_property(struct ofono_modem *modem,
     const gchar *key, GVariant *value)
{
  if (g_strcmp0(key, "Powered") == 0) {
    modem->powered = g_variant_get_boolean(value);
    tapi_debug("modem: %s Powered %hhu", modem->path, modem->powered);
  } else if (g_strcmp0(key, "Online") == 0) {
    modem->online = g_variant_get_boolean(value);
    tapi_debug("modem: %s Online %hhu", modem->path, modem->online);
  } else if (g_strcmp0(key, "Interfaces") == 0) {
    modem->interfaces = _modem_interfaces_extract(value);
    tapi_debug("modem: %s Interfaces 0x%02x", modem->path, modem->interfaces);
  }
}

static void _modem_property_changed(GDBusConnection *conn,
     const gchar *sender_name,
     const gchar *object_path, const gchar *iface,
     const gchar *signal_name, GVariant *parameters,
     gpointer data)
{
  gchar *key;
  GVariant *value;
  struct ofono_modem *modem = data;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &value);
  _update_modem_property(modem, key, value);

  g_variant_unref(value);
  g_free(key);
}

static void _modem_update_properties(struct ofono_modem *modem)
{
  GError *error = NULL;

  GVariantIter *iter;
  GVariant *ret, *value;
  gchar *key;

  tapi_debug("");

  ret = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
        modem->path,
        OFONO_MODEM_IFACE,
        "GetProperties",
        NULL,
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_SEND_MESSAGE_FLAGS_NONE,
        -1, NULL, &error);

  if (ret == NULL) {
    tapi_error("Message call failed (%s)", error->message);
    g_error_free(error);
    return;
  }

  g_variant_get(ret, "(a{sv})", &iter);
  while (g_variant_iter_loop(iter, "{sv}", &key, &value))
    _update_modem_property(modem, key, value);

  g_variant_iter_free(iter);
  g_variant_unref(ret);
}

EXPORT_API struct ofono_modem *ofono_modem_init(const char *obj_path)
{
  struct str_list *modems = ofono_get_modems();
  struct ofono_modem *modem;

  tapi_debug("");
  if (modems->count <= 0) {
    tapi_warn("There is no modem");
    return NULL;
  }

  if (s_bus_conn == NULL) {
    tapi_error("Fail to get dbus connection");
    return NULL;
  }

  if (obj_path == NULL)
    obj_path = modems->data[0];
  else {
    int i;
    tapi_bool found;
    for (i = 0; i < modems->count; i++) {
      if (modems->data[i] == NULL) {
        tapi_error("it's abnormal");
        continue;
      }

      if (g_strcmp0(modems->data[i], obj_path) == 0) {
      	found = TRUE;
        break;
      }
    }
    if (!found) {
      tapi_error("Don't find modem: %s", obj_path);
      return NULL;
    }
  }

  modem = g_new0(struct ofono_modem, 1);

  modem->path = g_strdup(obj_path);
  modem->conn = s_bus_conn;

  modem->prop_changed_watch = g_dbus_connection_signal_subscribe(
        modem->conn,
        OFONO_SERVICE,
        OFONO_MODEM_IFACE,
        "PropertyChanged",
        modem->path,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        _modem_property_changed,
        modem,
        NULL);

  _modem_update_properties(modem);

  return modem;
}

static void _noti_data_free(gpointer data)
{
  GList *list;
  struct ofono_noti_data *nd = data;

  tapi_debug("");

  if (nd == NULL)
    return;

  for (list = nd->cb_list; list; list = g_list_next(list)) {
    struct noti_cb_data *cbd = list->data;

    if (cbd != NULL && cbd->user_data_free_func)
      cbd->user_data_free_func(cbd->user_data);
  }

  g_list_free(nd->cb_list);
  g_free(nd);
}

EXPORT_API void ofono_modem_deinit(struct ofono_modem *modem)
{
  GList *list;

  tapi_debug("");

  if (modem == NULL)
    return;

  g_dbus_connection_signal_unsubscribe(s_bus_conn, modem->prop_changed_watch);

  for (list = modem->noti_list; list; list = g_list_next(list)) {
    struct ofono_noti_data *nd = list->data;

    if (nd == NULL)
      continue;

    g_dbus_connection_signal_unsubscribe(modem->conn, nd->watch);
  }

  g_free(modem->path);
  g_list_free_full(modem->noti_list, _noti_data_free);
  g_free(modem);
}

static struct ofono_noti_data *_find_noti_data(GList *list,
     enum ofono_noti noti)
{
  struct ofono_noti_data *noti_data;

  for(; list; list = g_list_next(list)) {
    noti_data = list->data;

    if (noti_data == NULL)
      continue;

    if (noti_data->noti == noti)
      return noti_data;
  }

  return NULL;
}

static struct noti_cb_data *_find_noti_cb_data(
     struct ofono_noti_data *data,
     noti_cb cb)
{
  struct noti_cb_data *cb_data;
  GList *list;

  tapi_debug("");

  if (data == NULL)
    return NULL;

  for (list = data->cb_list; list; list = g_list_next(list)) {
    cb_data = list->data;

    if (cb_data == NULL)
      continue;

    if (cb_data->cb == cb)
      return cb_data;
  }

  return NULL;
}

static void _notify(struct ofono_modem *modem, void *data,
     enum ofono_noti noti)
{
  GList *list;
  struct ofono_noti_data *nd;
  struct noti_cb_data *ncbd;

  tapi_debug("");

  nd = _find_noti_data(modem->noti_list, noti);
  if (nd == NULL)
    return;

  for (list = nd->cb_list; list; list = g_list_next(list)) {
    ncbd = list->data;

    if (ncbd == NULL) {
      tapi_warn("there is an empty callback data record");
      continue;
    }

    if (ncbd->cb)
      ncbd->cb(noti, data, ncbd->user_data);
  }
}

static void _modem_added_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  const gchar *path;
  GVariant *value;
  GVariantIter iter;

  tapi_debug("");

  g_variant_iter_init(&iter, parameters);
  value = g_variant_iter_next_value(&iter);
  path = g_variant_get_string(value, NULL);

  if (s_modems_changed_cb)
	  s_modems_changed_cb(path, TRUE);

  g_variant_unref(value);
}

static void _modem_removed_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *path;

  tapi_debug("");

  g_variant_get(parameters, "(o)", &path);
  _notify(modem, path, OFONO_NOTI_MODEM_REMOVED);

	if (s_modems_changed_cb)
	  s_modems_changed_cb(path, FALSE);

  g_free(path);
}

EXPORT_API void ofono_set_modems_changed_callback(modems_changed_cb cb)
{
	if (!cb)
	  return;

  s_modems_changed_cb = cb;

  if (!s_modem_added_watch)
    s_modem_added_watch = g_dbus_connection_signal_subscribe(
        s_bus_conn,
        OFONO_SERVICE,
        OFONO_MANAGER_IFACE,
        "ModemAdded",
        OFONO_MANAGER_PATH,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        _modem_added_notify,
        NULL,
        NULL);
	if (!s_modem_removed_watch)
    s_modem_removed_watch = g_dbus_connection_signal_subscribe(
        s_bus_conn,
        OFONO_SERVICE,
        OFONO_MANAGER_IFACE,
        "ModemRemoved",
        OFONO_MANAGER_PATH,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        _modem_removed_notify,
        NULL,
        NULL);
}

static void _modem_status_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var;
  gchar *key;
  enum modem_status status;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &var);
  if (g_strcmp0(key, "Online") == 0) {
    modem->online = g_variant_get_boolean(var);

    tapi_debug("modem: %s Online %hhu",
        modem->path, modem->online);

    if (modem->online)
      status = MODEM_STATUS_ONLINE;
    else
      status = MODEM_STATUS_OFFLINE;

    _notify(modem, &status, OFONO_NOTI_MODEM_STATUS_CHAANGED);
  }

  if (g_strcmp0(key, "Powered") == 0) {
    modem->powered = g_variant_get_boolean(var);
    tapi_debug("modem: %s Online %hhu",
        modem->path, modem->online);

    if (modem->powered)
      status = MODEM_STATUS_OFFLINE;
    else
      status = MODEM_STATUS_OFF;

    _notify(modem, &status, OFONO_NOTI_MODEM_STATUS_CHAANGED);
  }

  g_variant_unref(var);
  g_free(key);
}

static void _modem_interfaces_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var;
  gchar *key;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &var);
  if (g_strcmp0(key, "Interfaces") == 0) {
    modem->interfaces = _modem_interfaces_extract(var);

    tapi_debug("modem: %s Interfaces 0x%02x", modem->path, modem->interfaces);

    _notify(modem, &modem->interfaces,
        OFONO_NOTI_INTERFACES_CHANGED);
  }

  g_variant_unref(var);
  g_free(key);
}

static void _network_signal_strength_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var;
  char *key;
  struct registration_info info;
  unsigned signal;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &var);

  if (g_strcmp0(key, "Strength") == 0) {
    g_variant_get(var, "y", &signal);
    _notify(modem, &signal,
        OFONO_NOTI_SIGNAL_STRENTH_CHANGED);
  }

  g_free(key);
  g_variant_unref(var);
}

static void _network_status_notify(GDBusConnection *connection,
     const gchar *sender_name,
     const gchar *object_path,
     const gchar *interface_name,
     const gchar *signal_name,
     GVariant *parameters,
     gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var;
  char *key;
  struct registration_info info;
  unsigned signal;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &var);

  /* Signal strength is handled in another callback */
  if (g_strcmp0(key, "Strength") != 0) {
    ofono_network_get_registration_info(modem, &info);
    _notify(modem, &info,
        OFONO_NOTI_REGISTRATION_STATUS_CHANGED);
  }

  g_free(key);
  g_variant_unref(var);
}

static void _sim_status_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *val;
  gchar *key;
  struct sim_info info;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &val);

  if (g_strcmp0(key, "Present") == 0 ||
      g_strcmp0(key, "PinRequired") == 0) {
    ofono_sim_get_info(modem, &info);
    _notify(modem, &info.status,
        OFONO_NOTI_SIM_STATUS_CHANGED);
  }

  g_variant_unref(val);
  g_free(key);
}

static void _call_added_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariantIter *info_iter;
  char *path;
  struct ofono_call_info call_info;
  unsigned int call_id;

  tapi_debug("");

  memset(&call_info, 0, sizeof(call_info));

  g_variant_get(parameters, "(oa{sv})", &path, &info_iter);
  call_id = ofono_get_call_id_from_obj_path(path);
  ofono_call_get_call_info(modem, call_id, &call_info);
  call_info.call_id = call_id;

  g_variant_iter_free(info_iter);
  g_free(path);

  _notify(modem, &call_info, OFONO_NOTI_CALL_ADDED);
}

static void _call_removed_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *path;
  unsigned int call_id;

  tapi_debug("");

  g_variant_get(parameters, "(o)", &path);
  call_id = ofono_get_call_id_from_obj_path(path);
  _notify(modem, &call_id, OFONO_NOTI_CALL_REMOVED);
  g_free(path);
}

static void _call_status_changed_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var_val;
  char *key;
  struct ofono_call_info call_info;
  unsigned int call_id;
  enum ofono_call_status status;

  tapi_debug("");

  memset(&call_info, 0, sizeof(call_info));

  /* ofono report property one by one, we'd like get all once otherwise
     may trouble UI layer */
  call_id = ofono_get_call_id_from_obj_path((char*)object_path);

  g_variant_get(parameters, "(sv)", &key, &var_val);
  if (g_strcmp0(key, "State") == 0) {
    char *str;
    g_variant_get(var_val, "s", &str);
    status = ofono_str_to_call_status(str);
    g_free(str);
  }

  g_free(key);
  g_variant_unref(var_val);

  /* if call is disconnected, the call may have been removed, so can't
     get its information */
  if (status != CALL_STATUS_DISCONNECTED)
    ofono_call_get_call_info(modem, call_id, &call_info);

  call_info.call_id = call_id;
  call_info.status = status;

  _notify(modem, &call_info, OFONO_NOTI_CALL_STATUS_CHANGED);
}

static void _call_disconnect_reason_cb(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *reason;
  struct ofono_call_disconnect_reason cdr;

  memset(&cdr, 0, sizeof(struct ofono_call_disconnect_reason));
  cdr.call_id = ofono_get_call_id_from_obj_path((gchar *)object_path);
  g_variant_get(parameters, "(s)", &reason);

  if (reason == NULL) {
    tapi_error("");
    return;
  }

  if (g_strcmp0(reason, "local") == 0)
    cdr.reason = CALL_DISCONNECT_REASON_LOCAL_HANGUP;
  else if (g_strcmp0(reason, "remote") == 0)
    cdr.reason = CALL_DISCONNECT_REASON_REMOTE_HANGUP;
  else
    cdr.reason = CALL_DISCONNECT_REASON_UNKNOWN;

  _notify(modem, &cdr, OFONO_NOTI_CALL_DISCONNECT_REASON);

  g_free(reason);
}

static void _stk_idle_mode_text_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *key;
  GVariant *val;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &val);
  if (g_strcmp0(key, "IdleModeText") == 0) {
    char *text;
    g_variant_get(val, "s", &text);
    _notify(modem, text, OFONO_NOTI_SAT_IDLE_MODE_TEXT);
    g_free(text);
  }

  g_free(key);
  g_variant_unref(val);
}

static void _stk_main_menu_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *key;
  GVariant *val;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &val);
  if (strncmp(key, "MainMenu", strlen("MainMenu")) == 0)
    _notify(modem, NULL, OFONO_NOTI_SAT_MAIN_MENU);

  g_free(key);
  g_variant_unref(val);
}

static gchar *_ofono_sms_get_uuid_from_path(const gchar *path)
{
  gchar **token;
  gchar *uuid;

  tapi_debug("");

  token = g_strsplit(path, "message_", 2);

  if (token[1] == NULL) {
    tapi_error("Invalid UUID in path");
    return NULL;
  }

  uuid = g_strdup(token[1]);

  g_strfreev(token);

  return uuid;
}

static time_t _sms_time_parse(const gchar *str)
{
  struct tm tm;
  time_t zonediff;

  memset(&tm, 0, sizeof(tm));

  strptime(str, "%Y-%m-%dT%H:%M:%S%z", &tm);
  zonediff = tm.tm_gmtoff; /* mktime reset it */

  return mktime(&tm) - zonediff - timezone;
}

static void _sms_sending_status_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  char *uuid;
  char *key;
  GVariant *val;
  struct ofono_sms_sent_staus_noti noti;

  tapi_debug("");

  memset(&noti, 0, sizeof(noti));

  g_variant_get(parameters, "(sv)", &key, &val);
  if (g_strcmp0(key, "State") == 0) {
    const char *state = g_variant_get_string(val, NULL);

    tapi_debug("SMS State: %s", state);

    if (g_strcmp0(state, "pending") == 0)
      noti.state =  OFONO_SMS_SENT_STATE_PENDING;
    else if (g_strcmp0(state, "failed") == 0)
      noti.state =  OFONO_SMS_SENT_STATE_FAILED;
    else if (g_strcmp0(state, "sent") == 0)
      noti.state =  OFONO_SMS_SENT_STATE_SENT;
    else {
      tapi_error("unknown message state: %s", state);

      g_free(key);
      g_variant_unref(val);
      return;
    }
  }
  g_free(key);
  g_variant_unref(val);

  uuid = _ofono_sms_get_uuid_from_path(object_path);
  if (uuid == NULL) {
    tapi_error("get uuid failed");
    return;
  }

  tapi_debug("uuid=%s", uuid);
  noti.uuid = uuid;

  _notify(modem, &noti, OFONO_NOTI_MSG_STATUS_CHANGED);

  g_free(uuid);
}

static void _sms_notify(tapi_bool class0, GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariantIter *iter;
  GVariant *var;
  char *key;
  struct ofono_sms_incoming_noti noti;

  tapi_debug("");

  memset(&noti, 0, sizeof(noti));
  noti.class0 = class0;

  g_variant_get(parameters, "(sa{sv})", &noti.message, &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var)) {
    if (g_strcmp0(key, "Sender") == 0) {
      noti.sender = g_variant_dup_string(var, NULL);
      if (noti.sender == NULL) {
        g_variant_unref(var);
        g_free(key);
        goto error;
      }
      tapi_debug("Sender: %s", noti.sender);
    } else if (g_strcmp0(key, "LocalSentTime") == 0) {
      const char *val = g_variant_get_string(var, NULL);
      if (val == NULL) {
        g_variant_unref(var);
        g_free(key);
        goto error;
      }
      tapi_debug("LocalSentTime: %s", val);
      noti.timestamp = _sms_time_parse(val);
    } else if (g_strcmp0(key, "SentTime") == 0) {
      tapi_debug("SentTime: %s", g_variant_get_string(var, NULL));
    }

    g_variant_unref(var);
    g_free(key);
  }

  _notify(modem, &noti, OFONO_NOTI_INCOMING_SMS);

error:
  g_free(noti.message);
  g_free(noti.sender);
  g_variant_iter_free(iter);
}

static void _sms_immediate_msg_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  tapi_debug("");
  _sms_notify(TRUE, parameters, user_data);
}

static void _sms_incoming_msg_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  tapi_debug("");
  _sms_notify(FALSE, parameters, user_data);
}

static void _msg_delivery_report_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariantIter *iter;
  gchar *key;
  GVariant *val;
  struct ofono_sms_status_report_noti noti;

  tapi_debug("");

  memset(&noti, 0, sizeof(noti));

  g_variant_get(parameters, "(sa{sv})", &noti.message, &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
    if (g_strcmp0(key, "LocalSentTime") == 0) {
      const char *lsTime = g_variant_get_string(val, NULL);
      if (lsTime == NULL) {
        tapi_error("local send time is null.");
        g_variant_unref(val);
        g_free(key);
        goto erorr;
      }
      tapi_debug("LocalSentTime: %s", lsTime);
      noti.timestamp = _sms_time_parse(lsTime);
    } else if (g_strcmp0(key, "UUID") == 0) {
      noti.uuid = g_variant_dup_string(val, NULL);
    }

    g_variant_unref(val);
    g_free(key);
  }

  _notify(modem, &noti, OFONO_NOTI_SMS_DELIVERY_REPORT);

erorr:
  g_variant_iter_free(iter);
  g_free(noti.message);
  g_free(noti.uuid);
}

static void _cbs_incoming_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  guint16 channel;
  gchar *message;
  struct ofono_cbs_incoming_noti noti;

  tapi_debug("");

  memset(&noti, 0, sizeof(noti));

  g_variant_get(parameters, "(sq)", &message, &channel);
  noti.message = message;
  noti.channel = channel;

  _notify(modem, &noti, OFONO_NOTI_INCOMING_CBS);

  g_free(noti.message);
}

static void _cbs_emergency_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariantIter *iter;
  gchar *key;
  GVariant *var;
  struct ofono_cbs_emergency_noti noti;

  tapi_debug("");

  memset(&noti, 0, sizeof(noti));

  g_variant_get(parameters, "(sa{sv})", &noti.message, &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var)) {
    if (g_strcmp0("EmergencyType", key) == 0) {
      const char *type = g_variant_get_string(var, NULL);
      tapi_debug("cbs emergency type [%s]", type);

      if (g_strcmp0(type, "Earthquake") == 0)
        noti.type = OFONO_CBS_EMERG_TYPE_EARTHQUAKE;
      else if (g_strcmp0(type, "Tsunami") == 0)
        noti.type = OFONO_CBS_EMERG_TYPE_TSUNAMI;
      else if (g_strcmp0(type, "Earthquake+Tsunami") == 0)
        noti.type = OFONO_CBS_EMERG_TYPE_EARTHQUAKE_TSUNAMI;
      else if (g_strcmp0(type, "Other") == 0)
        noti.type = OFONO_CBS_EMERG_TYPE_OTHER;
      else {
        tapi_error("Unknown cbs emergency type");
        noti.type = OFONO_CBS_EMERG_TYPE_UNKNOWN;
      }
    } else if (g_strcmp0("Popup", key) == 0) {
      noti.popup= g_variant_get_boolean(var);
      tapi_debug("cbs emergency popup [%d]", noti.popup);
    } else if (g_strcmp0("EmergencyAlert", key) == 0) {
      noti.alert = g_variant_get_boolean(var);
      tapi_debug("cbs emergency alert [%d]", noti.alert);
    }

    g_variant_unref(var);
    g_free(key);
  }
  g_variant_iter_free(iter);

  _notify(modem, &noti, OFONO_NOTI_EMERGENCY_CBS);

  g_free(noti.message);
}

static void _ussd_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  struct ussd_request_noti noti;
  enum ofono_noti noti_id;

  tapi_debug("");

  g_variant_get(parameters, "s", &noti.message);

  if (g_strcmp0(signal_name, "NotificationReceived") == 0) {
    noti.response_required = FALSE;
    noti_id = OFONO_NOTI_USSD_NOTIFICATION;
  } else {
    noti.response_required = TRUE;
    noti_id = OFONO_NOTI_USSD_REQ;
  }

  _notify(modem, &noti, noti_id);

  g_free(noti.message);
}

static void _ussd_status_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *var;
  char *key;
  enum ussd_status status;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &var);
  if (g_strcmp0(key, "State") == 0) {
    const char *state = g_variant_get_string(var, NULL);
    if (g_strcmp0(state, "idle") == 0)
      status = SS_USSD_STATUS_IDLE;
    else if (g_strcmp0(state, "active") == 0)
      status = SS_USSD_STATUS_ACTIVE;
    else if (g_strcmp0(state, "user-response") == 0)
      status = SS_USSD_STATUS_ACTION_REQUIRE;
    else {
      tapi_error("Unknown USSD status");
      g_free(key);
      g_variant_unref(var);
      return;
    }
  }

  _notify(modem, &status, OFONO_NOTI_USSD_STATUS_CHANGED);

  g_free(key);
  g_variant_unref(var);
}

static void _connman_attached_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *val;
  gchar *key;
  tapi_bool attached;

  tapi_debug("");

  g_variant_get(parameters, "(sv)", &key, &val);

  if (g_strcmp0(key, "Attached") == 0) {
    attached = g_variant_get_boolean(val);
    _notify(modem, &attached, OFONO_NOTI_CONNMAN_ATTACHED);
  }

  g_variant_unref(val);
  g_free(key);
}

static void _connman_context_actived_notify(GDBusConnection *connection,
      const gchar *sender_name,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *signal_name,
      GVariant *parameters,
      gpointer user_data)
{
  struct ofono_modem *modem = user_data;
  GVariant *val;
  gchar *key;
  struct context_actived_noti noti;

  tapi_debug("%s", object_path);

  noti.path = g_strdup(object_path);
  g_variant_get(parameters, "(sv)", &key, &val);

  if (g_strcmp0(key, "Active") == 0) {
    noti.actived = g_variant_get_boolean(val);
    _notify(modem, &noti, OFONO_NOTI_CONNMAN_CONTEXT_ACTIVED);
  }

  g_variant_unref(val);
  g_free(key);
  g_free(noti.path);
}

static guint _subscribe_notification(struct ofono_modem *modem,
          enum ofono_noti noti)
{
  guint watch = 0;

  tapi_debug("");

  switch (noti) {
  case OFONO_NOTI_MODEM_REMOVED:
    watch = g_dbus_connection_signal_subscribe(
      s_bus_conn,
      OFONO_SERVICE,
      OFONO_MANAGER_IFACE,
      "ModemRemoved",
      OFONO_MANAGER_PATH,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _modem_removed_notify,
      modem,
      NULL);
    break;

  case OFONO_NOTI_MODEM_STATUS_CHAANGED:
    watch = g_dbus_connection_signal_subscribe(
      modem->conn,
      OFONO_SERVICE,
      OFONO_MODEM_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _modem_status_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_INTERFACES_CHANGED:
    watch = g_dbus_connection_signal_subscribe(
      modem->conn,
      OFONO_SERVICE,
      OFONO_MODEM_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _modem_interfaces_notify,
      modem,
      NULL);
    break;

  /* network */
  case OFONO_NOTI_SIGNAL_STRENTH_CHANGED:
    watch = g_dbus_connection_signal_subscribe(
      modem->conn,
      OFONO_SERVICE,
      OFONO_NETWORK_REGISTRATION_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _network_signal_strength_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_REGISTRATION_STATUS_CHANGED:
    watch = g_dbus_connection_signal_subscribe(
      modem->conn,
      OFONO_SERVICE,
      OFONO_NETWORK_REGISTRATION_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _network_status_notify,
      modem,
      NULL);
    break;
  /* Call */
  case OFONO_NOTI_CALL_ADDED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_VOICECALL_MANAGER_IFACE,
      "CallAdded",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _call_added_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_CALL_REMOVED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_VOICECALL_MANAGER_IFACE,
      "CallRemoved",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _call_removed_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_CALL_STATUS_CHANGED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_VOICECALL_IFACE,
      "PropertyChanged",
      NULL,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _call_status_changed_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_CALL_DISCONNECT_REASON:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_VOICECALL_IFACE,
      "DisconnectReason",
      NULL,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _call_disconnect_reason_cb,
      modem,
      NULL);
    break;

  /* SMS */
  case OFONO_NOTI_INCOMING_SMS_CLASS_0:
    /* class 0 sms arrives */
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_MESSAGE_MANAGER_IFACE,
      "ImmediateMessage",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _sms_immediate_msg_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_INCOMING_SMS:
    /* normal sms arrives */
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_MESSAGE_MANAGER_IFACE,
      "IncomingMessage",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _sms_incoming_msg_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_MSG_STATUS_CHANGED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_MESSAGE_IFACE,
      "PropertyChanged",
      NULL,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _sms_sending_status_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_SMS_DELIVERY_REPORT:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_MESSAGE_MANAGER_IFACE,
      "SendStatusReport",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _msg_delivery_report_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_INCOMING_CBS:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_CELL_BROADCAST_IFACE,
      "IncomingBroadcast",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _cbs_incoming_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_EMERGENCY_CBS:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_CELL_BROADCAST_IFACE,
      "EmergencyBroadcast",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _cbs_emergency_notify,
      modem,
      NULL);
    break;

  /* SIM */
  case OFONO_NOTI_SIM_STATUS_CHANGED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_SIM_MANAGER_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _sim_status_notify,
      modem,
      NULL);
    break;

  /* USSD */
  case OFONO_NOTI_USSD_NOTIFICATION:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE,
      "NotificationReceived",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _ussd_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_USSD_REQ:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE,
      "RequestReceived",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _ussd_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_USSD_STATUS_CHANGED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _ussd_status_notify,
      modem,
      NULL);
    break;
  /* connman */
  case OFONO_NOTI_CONNMAN_ATTACHED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_CONNMAN_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _connman_attached_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_CONNMAN_CONTEXT_ACTIVED:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_CONTEXT_IFACE,
      "PropertyChanged",
      NULL,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _connman_context_actived_notify,
      modem,
      NULL);
    break;
  /* STK */
  case OFONO_NOTI_SAT_IDLE_MODE_TEXT:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_STK_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _stk_idle_mode_text_notify,
      modem,
      NULL);
    break;
  case OFONO_NOTI_SAT_MAIN_MENU:
    watch = g_dbus_connection_signal_subscribe(modem->conn,
      OFONO_SERVICE,
      OFONO_STK_IFACE,
      "PropertyChanged",
      modem->path,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      _stk_main_menu_notify,
      modem,
      NULL);
    break;
  }

  return watch;
}

EXPORT_API tapi_bool ofono_register_notification_callback(struct ofono_modem *modem,
      enum ofono_noti noti,
      noti_cb cb,
      void *user_data,
      destroy_notify user_data_free_func)
{
  struct noti_cb_data *cb_data;
  struct ofono_noti_data *nd;
  GList *list;
  guint watch;

  tapi_debug("");

  if (modem == NULL || cb == NULL) {
    tapi_error("Invalid parameters");
    return FALSE;
  }

  cb_data = g_new0(struct noti_cb_data, 1);

  cb_data->cb = cb;
  cb_data->user_data = user_data;
  cb_data->user_data_free_func = user_data_free_func;

  list = modem->noti_list;
  nd = _find_noti_data(list, noti);
  if (nd != NULL) {
    if (_find_noti_cb_data(nd, cb) != NULL) {
      g_free(cb_data);

      tapi_warn("callback alreay exist");
      return TRUE;
    }

    nd->cb_list = g_list_append(nd->cb_list, cb_data);
    return TRUE;
  }

  watch = _subscribe_notification(modem, noti);
  if (watch == 0) {
    tapi_error("fail to subscribe notification");
    g_free(cb_data);

    return FALSE;
  }

  nd = g_new0(struct ofono_noti_data, 1);

  nd->modem = modem;
  nd->noti = noti;
  nd->cb_list = g_list_append(nd->cb_list, cb_data);
  nd->watch = watch;

  modem->noti_list = g_list_append(modem->noti_list, nd);

  return TRUE;
}

EXPORT_API void ofono_unregister_notification_callback(struct ofono_modem *modem,
            enum ofono_noti noti,
            noti_cb cb)
{
  struct ofono_noti_data *nd;
  struct noti_cb_data *ncbd;

  tapi_debug("");

  if (modem == NULL) {
    tapi_error("Invalid parameter");
    return;
  }

  nd = _find_noti_data(modem->noti_list, noti);
  if (nd == NULL) {
    tapi_warn("Don't find notification data");
    return;
  }

  ncbd = _find_noti_cb_data(nd, cb);
  if (ncbd != NULL) {
    nd->cb_list = g_list_remove(nd->cb_list, ncbd);

    if (ncbd->user_data_free_func)
      ncbd->user_data_free_func(ncbd->user_data);

    g_free(ncbd);
  }

  if (nd->cb_list == NULL) {
    g_dbus_connection_signal_unsubscribe(modem->conn, nd->watch);
    modem->noti_list = g_list_remove(modem->noti_list, nd);
    g_free(nd);
  }
}

EXPORT_API tapi_bool ofono_init()
{
  tapi_debug("");

  if (_get_dbus_connection() == NULL) {
    tapi_error("fail to get dbus connection");
    return FALSE;
  }

  return TRUE;
}

EXPORT_API void ofono_deinit()
{
  tapi_debug("");

  if (!s_bus_conn)
    return;

  if (s_modem_added_watch > 0) {
    g_dbus_connection_signal_unsubscribe(s_bus_conn, s_modem_added_watch);
		s_modem_added_watch = 0;
  }

	if (s_modem_removed_watch > 0) {
    g_dbus_connection_signal_unsubscribe(s_bus_conn, s_modem_removed_watch);
		s_modem_removed_watch = 0;
  }

  g_dbus_connection_close_sync(s_bus_conn, NULL, NULL);
  s_bus_conn = NULL;
}

EXPORT_API tapi_bool ofono_modem_get_power_status(struct ofono_modem *modem, tapi_bool *powered)
{
  if (modem == NULL || powered == NULL)
    return FALSE;

  *powered = modem->powered;
  return TRUE;
}

EXPORT_API tapi_bool ofono_has_interface(struct ofono_modem *modem, enum ofono_api api)
{
  return has_interface(modem->interfaces, api);
}
