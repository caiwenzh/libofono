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
#include "ofono-network.h"

static enum registration_status _str_to_registaration_status(const char *status)
{
  if (status == NULL) {
    tapi_error("registration status string is null");
    return REG_STATUS_UNKNOWN;
  }

  if (g_strcmp0(status, "unregistered") == 0)
    return REG_STATUS_NOT_REGISTERED;

  if (g_strcmp0(status, "registered") == 0)
    return REG_STATUS_REGISTERED_HOME;

  if (g_strcmp0(status, "searching") == 0)
    return REG_STATUS_SEARCHING;

  if (g_strcmp0(status, "denied") == 0)
    return REG_STATUS_DENIED;

  if (g_strcmp0(status, "roaming") == 0)
    return REG_STATUS_REGISTERED_ROAMING;

  tapi_warn("Unknown registration status: %s", status);
  return REG_STATUS_UNKNOWN;
}

static enum operator_status _str_to_operator_status(const char *status)
{
  if (status == NULL) {
    tapi_error("operator status string is null");
    return REG_STATUS_UNKNOWN;
  }

  if (g_strcmp0(status, "available") == 0)
    return OPERATOR_STATUS_AVAILABLE;

  if (g_strcmp0(status, "current") == 0)
    return OPERATOR_STATUS_CURRENT;

  if (g_strcmp0(status, "forbidden") == 0)
    return OPERATOR_STATUS_FORBIDDEN;

  if (g_strcmp0(status, "unknown") == 0)
    return REG_STATUS_DENIED;

  tapi_warn("Unknown registration status: %s", status);
  return REG_STATUS_UNKNOWN;
}

static enum access_tech _str_to_tech(const char *tech)
{
  if (tech == NULL) {
    tapi_error("access technology stirng is null");
    return ACCESS_TECH_UNKNOWN;
  }

  if (g_strcmp0(tech, "gsm") == 0)
    return ACCESS_TECH_GSM;

  if (g_strcmp0(tech, "umts") == 0)
    return ACCESS_TECH_UTRAN;

  if (g_strcmp0(tech, "edge") == 0)
    return ACCESS_TECH_EDGE;

  if (g_strcmp0(tech, "hspa") == 0)
    return ACCESS_TECH_UTRAN_HSDPA_HSUPA;

  if (g_strcmp0(tech, "lte") == 0)
    return ACCESS_TECH_EUTRAN;

  tapi_warn("Unknown access technology: %s", tech);
  return ACCESS_TECH_UNKNOWN;
}

static enum network_selection_mode _str_to_selection_mode(const char *mode)
{
  if (mode == NULL) {
    tapi_error("selection mode string is null");
    return NETWORK_SELECTION_MODE_UNKNOWN;
  }

  if (g_strcmp0(mode, "auto") == 0)
    return NETWORK_SELECTION_MODE_AUTO;

  if (g_strcmp0(mode, "auto-only") == 0)
    return NETWORK_SELECTION_MODE_AUTO_ONLY;

  if (g_strcmp0(mode, "manual") == 0)
    return NETWORK_SELECTION_MODE_MANUAL;

  tapi_warn("Unknown network selection mode: %s", mode);
  return NETWORK_SELECTION_MODE_UNKNOWN;
}

static enum network_mode _str_to_network_mode(const char *mode)
{
  if (mode == NULL) {
    tapi_error("network mode string is null");
    return NETWORK_MODE_UNKNOWN;
  }

  if (g_strcmp0(mode, "any") == 0)
    return NETWORK_MODE_AUTO;

  if (g_strcmp0(mode, "gsm") == 0)
    return NETWORK_MODE_2G;

  if (g_strcmp0(mode, "umts") == 0)
    return NETWORK_MODE_3G;

  if (g_strcmp0(mode, "lte") == 0)
    return NETWORK_MODE_4G;

  return NETWORK_MODE_UNKNOWN;
}

static const char *_network_mode_to_str(enum network_mode mode)
{
  switch (mode) {
  case NETWORK_MODE_AUTO: return "any";
  case NETWORK_MODE_2G: return "gsm";
  case NETWORK_MODE_3G: return "umts";
  case NETWORK_MODE_4G: return "lte";
  case NETWORK_MODE_UNKNOWN: return "";
  default:
    tapi_error("Unknown network mode: %d", mode);
    return "";
  }
}

static void _get_registration_info(struct ofono_modem *modem,
      GVariant *properties, struct registration_info *info)
{
  GVariantIter *iter;
  char *key;
  const char *value;
  GVariant *var;

  g_variant_get(properties, "(a{sv})", &iter);
  while (g_variant_iter_loop(iter, "{sv}", &key, &var)) {
    if (g_strcmp0(key, "Status") == 0) {
      value = g_variant_get_string(var, NULL);
      info->status = _str_to_registaration_status(value);
      tapi_debug("status(%d): %s", info->status, value);
    } else if (g_strcmp0(key, "LocationAreaCode") == 0) {
      g_variant_get(var, "q", &info->lac);
      tapi_debug("lac: %X", info->lac);
    } else if (g_strcmp0(key, "CellId") == 0) {
      g_variant_get(var, "u", &info->cid);
      tapi_debug("lac: %X", info->cid);
    } else if (g_strcmp0(key, "Technology") == 0) {
      value = g_variant_get_string(var, NULL);
      info->act = _str_to_tech(value);
      tapi_debug("act(%d): %s", info->act, value);
    } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
      value = g_variant_get_string(var, NULL);
      g_strlcpy(info->mcc, value, sizeof(info->mcc));
      tapi_debug("mcc: %s", info->mcc);
    } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
      value = g_variant_get_string(var, NULL);
      g_strlcpy(info->mnc, value, sizeof(info->mnc));
      tapi_debug("mnc: %s", info->mnc);
    }
  }

  g_variant_iter_free(iter);
}

EXPORT_API tapi_bool ofono_network_get_registration_info(
      struct ofono_modem *modem,
      struct registration_info *info)
{
  GError *error = NULL;
  GVariant *var;

  tapi_debug("");

  if (modem == NULL || info == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  memset(info, 0, sizeof(struct registration_info));
  info->status = REG_STATUS_UNKNOWN;
  info->act = ACCESS_TECH_UNKNOWN;

  if (!has_interface(modem->interfaces, OFONO_API_NETREG)) {
    tapi_error("OFONO_API_NETREG doesn't exist");
    return FALSE;
  }

  var = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
      modem->path, OFONO_NETWORK_REGISTRATION_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  _get_registration_info(modem, var, info);

  g_variant_unref(var);
  return TRUE;
}

EXPORT_API tapi_bool ofono_network_get_operator_name(struct ofono_modem *modem,
      char **name)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (modem == NULL || name == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  *name = NULL;

  if (!has_interface(modem->interfaces, OFONO_API_NETREG)) {
    tapi_error("OFONO_API_NETREG doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_NETWORK_REGISTRATION_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Name") == 0) {
      g_variant_get(var_val, "s", name);
      tapi_info("Operator name: %s", *name);

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

EXPORT_API tapi_bool ofono_network_get_signal_strength(
      struct ofono_modem *modem, unsigned char *signal)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (modem == NULL || signal == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_NETREG)) {
    tapi_error("OFONO_API_NETREG doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_NETWORK_REGISTRATION_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Strength") == 0) {
      g_variant_get(var_val, "y", signal);
      tapi_info("Signal strength: %d", *signal);

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

EXPORT_API tapi_bool ofono_network_get_network_selection_mode(
      struct ofono_modem *modem,
      enum network_selection_mode *mode)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (modem == NULL || mode == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_NETREG)) {
    tapi_error("OFONO_API_NETREG doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_NETWORK_REGISTRATION_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Mode") == 0) {
      const char *str_mode =
          g_variant_get_string(var_val, NULL);
      *mode = _str_to_selection_mode(str_mode);
      tapi_debug("Selection mode: %s", str_mode);

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

static void _on_response_get_mode(GObject *obj, GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  enum network_mode mode;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *str_mode;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "TechnologyPreference") == 0) {
      g_variant_get(var_val, "s", &str_mode);
      tapi_debug("Mode: %s", str_mode);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  mode = _str_to_network_mode(str_mode);
  g_free(str_mode);

  CALL_RESP_CALLBACK(ret, &mode, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(resp);
}

EXPORT_API void ofono_network_get_mode(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_RADIO_SETTINGS_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_mode, cbd);
}

EXPORT_API void ofono_network_set_mode(struct ofono_modem *modem,
      enum network_mode mode,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;
  const char *str_mode;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  str_mode = _network_mode_to_str(mode);
  tapi_debug("Mode(%d): %s", mode, str_mode);

  var = g_variant_new("(sv)", "TechnologyPreference",
      g_variant_new_string(str_mode));

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_RADIO_SETTINGS_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_network_register(struct ofono_modem *modem,
      const char *plmn,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  CHECK_PARAMETERS(modem && plmn, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Plmn: %s", plmn);

  char *path = g_strdup_printf("%s/operator/%s", modem->path, plmn);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path,
      OFONO_NETWORK_OPERATOR_IFACE, "Register", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);

  g_free(path);
}

EXPORT_API void ofono_network_auto_register(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_NETWORK_REGISTRATION_IFACE, "Register", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static void _on_response_scan_operators(GObject *obj, GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;

  struct operators_info ops;
  int count;
  struct operator_info *p_op;
  GVariantIter *iter, *iter_properites;
  char *path;
  char *key;
  GVariant *val;
  int i;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);
  tapi_debug("%s", g_variant_print(resp, TRUE));

  g_variant_get(resp, "(a(oa{sv}))", &iter);
  count = g_variant_iter_n_children(iter);
  if (count == 0) {
    CALL_RESP_CALLBACK(ret, NULL, cbd);
    g_variant_iter_free(iter);
    g_variant_unref(resp);
    return;
  }

  ops.ops = g_malloc0(sizeof(struct operator_info) * count);
  ops.count = count;
  p_op = ops.ops;

  while (g_variant_iter_loop(iter, "(oa{sv})", &path, &iter_properites)) {
      p_op->path = g_strdup(path);
    while(g_variant_iter_loop(iter_properites, "{sv}", &key, &val)) {
      if (g_strcmp0(key, "Name") == 0) {
        p_op->name = g_variant_dup_string(val, NULL);
        tapi_debug("Name: %s", p_op->name);
      } else if (g_strcmp0(key, "Status") == 0) {
        const char *s = g_variant_get_string(val, NULL);
        p_op->status = _str_to_operator_status(s);
        tapi_debug("Staus(%d): %s", p_op->status, s);
      } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
        const char *mcc = g_variant_get_string(val, NULL);
        g_strlcpy(p_op->plmn, mcc, MAX_MCC_LEN + 1);
        tapi_debug("MCC: %s", mcc);
      } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
        const char *mnc = g_variant_get_string(val, NULL);
        g_strlcpy(p_op->plmn + MAX_MCC_LEN, mnc,
            MAX_MNC_LEN + 1);
        tapi_debug("MNC: %s", mnc);
      } else if (g_strcmp0(key, "Technologies") == 0) {
        char *tech;
        GVariantIter iter_tech;
        g_variant_iter_init(&iter_tech, val);
        while (g_variant_iter_loop(&iter_tech, "s", &tech)) {
          p_op->techs |= 1 << _str_to_tech(tech);
          tapi_debug("ACT: %s", tech);
        }
      }
    }
    p_op++;
  }

  CALL_RESP_CALLBACK(ret, &ops, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(resp);
  for (i = 0; i < count; i++) {
    g_free(ops.ops[i].name);
    g_free(ops.ops[i].path);
  }
  g_free(ops.ops);
}

EXPORT_API void ofono_network_scan_operators(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_NETWORK_REGISTRATION_IFACE, "Scan", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, 100000, NULL,
      _on_response_scan_operators, cbd);
}
