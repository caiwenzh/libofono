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
#include <stdlib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "common.h"
#include "ofono-sms.h"
#include "log.h"

#define SMS_PROPERTY_SCA  "ServiceCenterAddress"
#define SMS_PROPERTY_UDR  "UseDeliveryReports"
#define SMS_PROPERTY_BEARER  "Bearer"
#define SMS_PROPERTY_ALPHABET  "Alphabet"
#define CBS_PROPERTY_POWERED  "Powered"
#define CBS_PROPERTY_TOPICS  "Topics"

static void _on_response_get_sca(GObject *obj,
      GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *sca = NULL;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, SMS_PROPERTY_SCA) == 0) {
      g_variant_get(var_val, "s", &sca);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  tapi_debug("sca: %s", sca);

  CALL_RESP_CALLBACK(ret, sca, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(resp);
  g_free(sca);
}

EXPORT_API void ofono_sms_get_sca(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_MESSAGE_MANAGER_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_sca, cbd);
}

EXPORT_API void ofono_sms_set_sca(struct ofono_modem *modem,
      const char *sca, response_cb cb, void *user_data)
{
  GVariant *var;

  CHECK_PARAMETERS(modem && sca, cb, user_data);

  tapi_debug("sca: %s", sca);

  var = g_variant_new_string(sca);
  ofono_set_property(modem, OFONO_MESSAGE_MANAGER_IFACE, modem->path,
      SMS_PROPERTY_SCA, var, cb, user_data);
}

static void _on_response_get_delivery_report(GObject *source_object,
      GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  tapi_bool on;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, SMS_PROPERTY_UDR) == 0) {
      g_variant_get(var_val, "b", &on);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  tapi_debug("Delivery report: %d", on);

  CALL_RESP_CALLBACK(ret, &on, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
}

EXPORT_API void ofono_sms_get_delivery_report(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_MESSAGE_MANAGER_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_delivery_report, cbd);
}

EXPORT_API void ofono_sms_set_delivery_report(struct ofono_modem *modem,
      tapi_bool on, response_cb cb, void *user_data)
{
  GVariant *var;

  CHECK_PARAMETERS(modem, cb, user_data);
  tapi_debug("Delivery report: %d", on);

  var = g_variant_new_boolean(on);
  ofono_set_property(modem, OFONO_MESSAGE_MANAGER_IFACE, modem->path,
      SMS_PROPERTY_UDR, var, cb, user_data);
}

static void _on_response_send_sms(GObject *source_object,
    GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *path = NULL;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(o)", &path);

  tapi_debug("path: %s", path);

  CALL_RESP_CALLBACK(ret, path, cbd);
  g_variant_unref(dbus_result);
  g_free(path);
}

EXPORT_API void ofono_sms_send_sms(struct ofono_modem *modem,
      const char *number, const char *msg,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && number && msg, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Number: %s, Content: %s", number, msg);

  var = g_variant_new("(ss)", number, msg);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_MESSAGE_MANAGER_IFACE, "SendMessage", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_send_sms, cbd);
}

EXPORT_API void ofono_sms_send_vcard(struct ofono_modem *modem,
      const char *number, const unsigned char *msg,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("");

  CHECK_PARAMETERS(modem && number && msg, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(s^ay)", number, msg);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SMART_MESSAGE_IFACE, "SendBusinessCard", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_send_sms, cbd);
}

EXPORT_API void ofono_sms_send_vcalendar(struct ofono_modem *modem,
      const char *number, const unsigned char *msg,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("");

  CHECK_PARAMETERS(modem && number && msg, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(s^ay)", number, msg);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SMART_MESSAGE_IFACE, "SendAppointment", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_send_sms, cbd);
}

static void _on_response_get_cbs_config(GObject *source_object,
    GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  struct ofono_sms_cbs_config config;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, CBS_PROPERTY_TOPICS) == 0) {
      g_variant_get(var_val, "s", &config.topics);
    } else if (g_strcmp0(key, CBS_PROPERTY_POWERED) == 0) {
      g_variant_get(var_val, "b", &config.powered);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  tapi_debug("topics: %s, powered: %d", config.topics, config.powered);

  CALL_RESP_CALLBACK(ret, &config, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
  g_free(config.topics);
}

EXPORT_API void ofono_sms_get_cbs_config(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CELL_BROADCAST_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_cbs_config, cbd);
}

EXPORT_API void ofono_sms_set_cbs_powered(struct ofono_modem *modem,
      tapi_bool powered, response_cb cb, void *user_data)
{
  GVariant *var;
  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);

  var = g_variant_new_boolean(powered);
  ofono_set_property(modem, OFONO_CELL_BROADCAST_IFACE, modem->path,
      CBS_PROPERTY_POWERED, var, cb, user_data);
}

EXPORT_API void ofono_sms_set_cbs_topics(struct ofono_modem *modem,
      const char *topics, response_cb cb, void *user_data)
{
  GVariant *var;
  tapi_debug("");

  CHECK_PARAMETERS(modem && topics, cb, user_data);

  var = g_variant_new_string(topics);
  ofono_set_property(modem, OFONO_CELL_BROADCAST_IFACE, modem->path,
      CBS_PROPERTY_TOPICS, var, cb, user_data);
}
