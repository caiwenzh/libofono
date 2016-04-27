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

#include "common.h"
#include "log.h"

static const struct Error_Map {
  TResult ret;
  const char *name;
  size_t namelen;
} error_map[] = {
#define MAP(ret, name) {ret, name, sizeof(name) - 1}
  MAP(TAPI_RESULT_INVALID_ARGS, "org.ofono.Error.InvalidArguments"),
  MAP(TAPI_RESULT_NOT_SUPPORTED, "org.ofono.Error.NotImplemented"),
  MAP(TAPI_RESULT_IN_PROGRESS, "org.ofono.Error.InProgress"),
  MAP(TAPI_RESULT_TIMEOUT, "org.ofono.Error.Timedout"),
  MAP(TAPI_RESULT_SIM_NOT_READY, "org.ofono.Error.SimNotReady"),
  MAP(TAPI_RESULT_PWD_INCORRECT, "org.ofono.Error.IncorrectPassword"),
  MAP(TAPI_RESULT_NOT_REGISTERED, "org.ofono.Error.NotRegistered"),
  MAP(TAPI_RESULT_FAIL, "org.ofono.Error.NotAvailable"),
#undef MAP
  {0, NULL, 0}
};

TResult ofono_error_parse(GError *err)
{
  const struct Error_Map *itr;
  if (NULL == err)
    return TAPI_RESULT_OK;

  tapi_error("dbus error = %d (%s)", err->code, err->message);

  for (itr = error_map; itr->name != NULL; itr++) {
    if (g_strrstr(err->message, itr->name))
      return itr->ret;
  }

  return TAPI_RESULT_UNKNOWN_ERROR;
}

tapi_bool has_interface(guint32 interfaces, enum ofono_api api)
{
  if ((interfaces & (1 << api)) != 0)
    return TRUE;

  return FALSE;
}

void on_response_common(GObject *obj, GAsyncResult *result, gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;

  tapi_debug("");

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  CALL_RESP_CALLBACK(ret, NULL, cbd);
  g_variant_unref(resp);
}

void ofono_set_property(struct ofono_modem *modem, const char *iface,
                char *path, const char *key, GVariant *value,
                response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *val;

  NEW_RSP_CB_DATA(cbd, cb, user_data);

  val = g_variant_new("(sv)", key, value);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path, iface,
      "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE, -1,
      NULL, on_response_common, cbd);
}

unsigned int ofono_get_call_id_from_obj_path(char *obj_path)
{
  char *p;

  if (obj_path == NULL) {
    tapi_error("");
    return 0;
  }

  p = g_strrstr(obj_path, "/voicecall");
  if (p == NULL) {
    tapi_error("");
    return 0;
  }

  return atoi(p + strlen("/voicecall"));
}

enum ofono_call_status ofono_str_to_call_status(const char *str)
{
  if (g_strcmp0(str, "active") == 0)
    return CALL_STATUS_ACTIVE;

  else if (g_strcmp0(str, "held") == 0)
    return CALL_STATUS_HELD;

  else if (g_strcmp0(str, "dialing") == 0)
    return CALL_STATUS_DIALING;

  else if (g_strcmp0(str, "alerting") == 0)
    return CALL_STATUS_ALERTING;

  else if (g_strcmp0(str, "incoming") == 0)
    return CALL_STATUS_INCOMING;

  else if (g_strcmp0(str, "waiting") == 0)
    return CALL_STATUS_WAITING;

  else if (g_strcmp0(str, "disconnected") == 0)
    return CALL_STATUS_DISCONNECTED;

  tapi_warn("unknown call state: %s", str);
  return CALL_STATUS_DISCONNECTED;
}

enum access_tech ofono_str_to_tech(const char *tech)
{
  if (tech == NULL) {
    tapi_error("access technology stirng is null");
    return ACCESS_TECH_UNKNOWN;
  }

  if (g_strcmp0(tech, "gsm") == 0 || g_strcmp0(tech, "gprs") == 0)
    return ACCESS_TECH_GSM;

  if (g_strcmp0(tech, "umts") == 0)
    return ACCESS_TECH_UTRAN;

  if (g_strcmp0(tech, "edge") == 0)
    return ACCESS_TECH_EDGE;

  if (g_strcmp0(tech, "hsdpa") == 0)
    return ACCESS_TECH_UTRAN_HSDPA;

  if (g_strcmp0(tech, "hsupa") == 0)
    return ACCESS_TECH_UTRAN_HSUPA;

  if (g_strcmp0(tech, "hspa") == 0)
    return ACCESS_TECH_UTRAN_HSDPA_HSUPA;

  if (g_strcmp0(tech, "lte") == 0)
    return ACCESS_TECH_EUTRAN;

  tapi_warn("Unknown access technology: %s", tech);
  return ACCESS_TECH_UNKNOWN;
}
