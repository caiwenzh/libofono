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

#include "common.h"
#include "log.h"
#include "ofono-modem.h"

EXPORT_API tapi_bool ofono_modem_get_online(struct ofono_modem *modem)
{
  if (modem == NULL) {
    tapi_error("");
    return FALSE;
  }

  tapi_debug("Online : %d", modem->online);
  return modem->online;
}

EXPORT_API void ofono_modem_set_online(struct ofono_modem *modem,
        tapi_bool online,
        response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("Online: %d", online);

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(sv)", "Online", g_variant_new_boolean(online));
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_MODEM_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_modem_get_powered(struct ofono_modem *modem)
{
  if (modem == NULL) {
    tapi_error("");
    return FALSE;
  }

  tapi_debug("Powered : %d", modem->powered);
  return modem->powered;
}

EXPORT_API void ofono_modem_set_powered(struct ofono_modem *modem,
        tapi_bool powered,
        response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("powered: %d", powered);

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(sv)", "Powered", g_variant_new_boolean(powered));
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_MODEM_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_modem_get_info(struct ofono_modem *modem,
      struct modem_info* info)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  const char* val;

  tapi_debug("");

  if (modem == NULL || info == NULL)
    return FALSE;

  memset(info, 0, sizeof(*info));
  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path, OFONO_MODEM_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Manufacturer") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->manufacturer, val,
          sizeof(info->manufacturer));
    } else if (g_strcmp0(key, "Model") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->model, val, sizeof(info->model));
    } else if (g_strcmp0(key, "Revision") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->revision, val,
          sizeof(info->revision));
    } else if (g_strcmp0(key, "Serial") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->serial, val, sizeof(info->serial));
    } else if (g_strcmp0(key, "Type") == 0) {
      val = g_variant_get_string(var_val, NULL);
      if (g_strcmp0(val, "test") == 0)
        info->type = MODEM_TYPE_TEST;
      else if (g_strcmp0(val, "hfp") == 0)
        info->type = MODEM_TYPE_HFP;
      else if (g_strcmp0(val, "sap") == 0)
        info->type = MODEM_TYPE_SAP;
      else if (g_strcmp0(val, "hardware") == 0)
        info->type = MODEM_TYPE_HARDWARE;
      else
        tapi_error("Unknown modem type: %s", val);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  tapi_debug("Manufacturer: %s", info->manufacturer);
  tapi_debug("Model: %s", info->model);
  tapi_debug("Revision: %s", info->revision);
  tapi_debug("Serial: %s", info->serial);

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  return TRUE;
}
