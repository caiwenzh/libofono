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
#include <gio/gio.h>

#include "log.h"
#include "common.h"
#include "ofono-sms-agent.h"

struct ofono_push_noti_agent {
  struct ofono_modem *modem;
  GList *push_noti_cb_list; /* GList<struct push_noti_cb_data*> */
  guint push_registration_id;
};

struct push_noti_cb_data {
  push_notify_cb_t cb;
  void *user_data;
};

/* The agent dbus interfaces:
  "service": system assign
  "path": the same as the path of the modem (modem->path)
  "API": as below xml
*/
static const char introspection_xml[] =
"<node>"
"  <interface name='org.ofono.PushNotificationAgent'>"
"    <method name='ReceiveNotification'>"
"      <arg type='ay' name='notification' direction='in'/>"
"      <arg type='a{sv}' name='info' direction='in'/>"
"    </method>"
"    <method name='Release'>"
"    </method>"
"  </interface>"
"</node>";

static void _handle_notify_call(const char *iface_name, const char *method_name,
      GVariant *parameters, void *user_data);

static void _handle_method_call(GDBusConnection *connection,
      const gchar *sender, const gchar *object_path,
      const gchar *interface_name, const gchar *method_name,
      GVariant *parameters, GDBusMethodInvocation *invocation,
      gpointer user_data)
{
  _handle_notify_call(interface_name, method_name, parameters, user_data);
  g_dbus_method_invocation_return_value(invocation, NULL);
}

static const GDBusInterfaceVTable interface_vtable = {
  _handle_method_call,
  NULL,
  NULL
};

static guint _ofono_register_agent_object(struct ofono_push_noti_agent *agent,
      const gchar *data,
      const gchar *path)
{
  struct ofono_modem *modem;
  guint registration_id;
  GError *error = NULL;
  GDBusNodeInfo *introspection_data;

  modem = agent->modem;
  introspection_data = g_dbus_node_info_new_for_xml(data, NULL);
  if (introspection_data == NULL) {
    tapi_error("Generic Node info from %s failed.", data);
    return 0;
  }

  registration_id = g_dbus_connection_register_object(
        modem->conn,
        path,
        introspection_data->interfaces[0],
        &interface_vtable,
        agent,
        NULL,
        &error);
  if (error != NULL) {
    tapi_error("register object failed!(%s)", error->message);
    g_error_free(error);
  }

  g_dbus_node_info_unref(introspection_data);

  return registration_id;
}

static void _ofono_unregister_agent_object(struct ofono_push_noti_agent *agent)
{
  tapi_debug("");
  if (agent == NULL || agent->modem == NULL) {
    tapi_error("modem is null!");
    return;
  }

  g_dbus_connection_unregister_object(agent->modem->conn,
      agent->push_registration_id);

  agent->push_registration_id = 0;
}

static tapi_bool _handle_push_notification_received(GVariant *content,
      GVariant *info, void *user_data)
{
  char *key;
  gsize len = 0;
  GVariant *val;
  GVariantIter *iter;
  GList *list;
  struct ofono_push_noti_info noti;
  struct push_noti_cb_data *cbd;
  const char *ctx;
  struct ofono_push_noti_agent *agent = user_data;

  memset(&noti, 0, sizeof(noti));
  ctx = g_variant_get_fixed_array(content, &len, sizeof(gchar));
  noti.content = g_malloc0(len);
  memcpy(noti.content, ctx, len);
  noti.length = len;

  g_variant_get(info, "a{sv}", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &val)) {
    if (g_strcmp0(key, "Sender") == 0) {
      g_variant_get(val, "s", &noti.sender);
    } else if (g_strcmp0(key, "LocalSentTime") == 0) {
      g_variant_get(val, "s", &noti.local_senttime);
    } else if (g_strcmp0(key, "SentTime") == 0) {
      g_variant_get(val, "s", &noti.senttime);
    }

    g_free(key);
    g_variant_unref(val);
  }
  g_variant_iter_free(iter);

  for (list = agent->push_noti_cb_list; list; list = g_list_next(list)) {
    cbd = (struct push_noti_cb_data*) list->data;
    if (cbd && cbd->cb)
      cbd->cb(&noti, cbd->user_data);
  }

  g_free(noti.content);
  g_free(noti.local_senttime);
  g_free(noti.sender);
  g_free(noti.senttime);
  return TRUE;
}

static void _handle_notify_call(const char *iface_name, const char *method_name,
      GVariant *parameters, void *user_data)
{
  if (g_strcmp0(iface_name, "org.ofono.PushNotificationAgent") == 0) {
    if (g_strcmp0(method_name, "ReceiveNotification") == 0) {
      GVariant *content = g_variant_get_child_value(parameters, 0);
      GVariant *info = g_variant_get_child_value(parameters, 1);

      _handle_push_notification_received(content, info, user_data);

      g_variant_unref(content);
      g_variant_unref(info);
    } else
      tapi_warn("No match method");
  } else
    tapi_error("Error iface name: %s", iface_name);
}

static tapi_bool _ofono_register_notification_agent(struct ofono_modem *modem)
{
  GVariant *ret, *val;
  GError *error = NULL;

  if (modem == NULL)
    return FALSE;

  val = g_variant_new("(o)", modem->path);
  ret = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
          modem->path,
          OFONO_PUSH_NOTIFICATION_IFACE,
          "RegisterAgent",
          val,
          NULL,
          G_DBUS_SEND_MESSAGE_FLAGS_NONE,
          -1, NULL, &error);
  if (ret == NULL) {
    tapi_debug("Send Message error: %s", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_unref(ret);
  return TRUE;
}

static tapi_bool _ofono_unregister_notification_agent(struct ofono_modem *modem)
{
  GVariant *ret, *val;
  GError *error = NULL;

  if (modem == NULL)
    return FALSE;

  val = g_variant_new("(o)", modem->path);
  ret = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
          modem->path,
          OFONO_PUSH_NOTIFICATION_IFACE,
          "UnregisterAgent",
          val,
          NULL,
          G_DBUS_SEND_MESSAGE_FLAGS_NONE,
          -1, NULL, &error);
  if (ret == NULL) {
    tapi_debug("Send Message error: %s", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_unref(ret);
  return TRUE;
}

static void _ofono_register_push_agent(struct ofono_push_noti_agent *agent)
{
  struct ofono_modem *modem;
  if (agent == NULL || agent->modem == NULL) {
    tapi_error("");
    return;
  }

  if (agent->push_registration_id > 0)
    return;

  modem = agent->modem;
  /* Create agent d-bus APIs */
  agent->push_registration_id = _ofono_register_agent_object(agent,
            introspection_xml,
            modem->path);
  if (agent->push_registration_id == 0) {
    tapi_error("register agent(%s) object failed.", modem->path);
    return;
  }

  /* register agent to ofonod */
  if (!_ofono_register_notification_agent(modem))
    tapi_error("register_notification_agent failed.");
}

static void _ofono_interfaces_changed_cb(enum ofono_noti noti,
      void *data,
      void *user_data)
{
  unsigned int ifaces;
  struct ofono_push_noti_agent *agent = user_data;

  ifaces = *((unsigned int *) data);
  if (noti != OFONO_NOTI_INTERFACES_CHANGED)
    return;

  if (NULL == agent || NULL == data) {
    tapi_error("");
    return;
  }

  /* interface of org.ofono.PushNotification become available */
  if (has_interface(ifaces, OFONO_API_PUSH_NOTIF)) {
    if (agent->push_registration_id == 0)
      _ofono_register_push_agent(agent);
  } else {
    if (agent->push_registration_id != 0) {
      _ofono_unregister_agent_object(agent);
      agent->push_registration_id = 0;
    }
  }
}

EXPORT_API struct ofono_push_noti_agent* ofono_new_push_agent(
      struct ofono_modem *modem)
{
  struct ofono_push_noti_agent *agent;

  if (modem == NULL)
    return NULL;

  agent = g_new0(struct ofono_push_noti_agent, 1);

  agent->modem = modem;
  return agent;
}

EXPORT_API void ofono_free_push_agent(struct ofono_push_noti_agent* agent)
{
  if (agent == NULL)
    return;

  g_list_free_full(agent->push_noti_cb_list, g_free);
  g_free(agent);
}

EXPORT_API TResult ofono_register_push_notification_callback(
      struct ofono_push_noti_agent* agent,
      push_notify_cb_t cb,
      void *user_data)
{
  struct ofono_modem *modem;
  struct push_noti_cb_data *cbd;
  GList *list;
  struct push_noti_cb_data *item;

  if (agent == NULL || cb == NULL)
    return TAPI_RESULT_INVALID_ARGS;

  modem = agent->modem;

  list = agent->push_noti_cb_list;
  while (list != NULL) {
    item = list->data;
    if (item->cb == cb) {
      tapi_warn("push_notify_callback already exists.");
      return TAPI_RESULT_OK;
    }

    list = list->next;
  }

  cbd = g_new0(struct push_noti_cb_data, 1);
  cbd->cb = cb;
  cbd->user_data = user_data;
  agent->push_noti_cb_list = g_list_append(agent->push_noti_cb_list, cbd);

  if (agent->push_registration_id != 0)
    return TAPI_RESULT_OK;

  ofono_register_notification_callback(modem,
        OFONO_NOTI_INTERFACES_CHANGED,
        _ofono_interfaces_changed_cb,
        agent, NULL);

  if (has_interface(modem->interfaces, OFONO_API_PUSH_NOTIF))
    _ofono_register_push_agent(agent);
  else
    tapi_warn("Push Notification interface isn't ready yet");

  return TAPI_RESULT_OK;
}

EXPORT_API TResult ofono_unregister_push_notification_callback(
      struct ofono_push_noti_agent* agent, push_notify_cb_t cb)
{
  struct ofono_modem *modem;
  GList *list;
  struct push_noti_cb_data *item;

  if (agent == NULL || cb == NULL)
    return TAPI_RESULT_INVALID_ARGS;

  modem = agent->modem;
  if (modem == NULL) {
    tapi_error("");
    return TAPI_RESULT_UNKNOWN_ERROR;
  }

  list = agent->push_noti_cb_list;
  while (item = list->data) {
    list = list->next;
    if (item->cb != cb)
      continue;

    agent->push_noti_cb_list = g_list_remove(agent->push_noti_cb_list, item);
    g_free(item);

    if (g_list_length(agent->push_noti_cb_list) == 0) {
      ofono_unregister_notification_callback(modem,
          OFONO_NOTI_INTERFACES_CHANGED,
          _ofono_interfaces_changed_cb);
      _ofono_unregister_notification_agent(modem);
      _ofono_unregister_agent_object(agent);
    }

    return TAPI_RESULT_OK;
  }

  tapi_warn("Don't find the notification callback");
  return TAPI_RESULT_OK;
}
