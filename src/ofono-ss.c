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

#include "common.h"
#include "log.h"
#include "ofono-sim.h"
#include "ofono-ss.h"

static tapi_bool _check_call_forwarding_setting(struct call_forward_setting *s)
{
  if (s == NULL)
    return FALSE;

  if (s->enable == TRUE && strlen(s->num) == 0)
    return FALSE;

  if (s->condition == SS_CF_CONDITION_CFNRY &&
    (s->timeout > 30 || s->timeout < 1))
    return FALSE;

  return TRUE;
}

static const char *_condition_to_str(enum call_forward_condition cond)
{
  switch(cond) {
  case SS_CF_CONDITION_CFU:
    return "VoiceUnconditional";
  case SS_CF_CONDITION_CFB:
    return "VoiceBusy";
  case SS_CF_CONDITION_CFNRC:
    return "VoiceNotReachable";
  case SS_CF_CONDITION_CFNRY:
    return "VoiceNoReply";
  default: return "";
  }
}

static enum cli_status _str_to_cli_status(char *status)
{
  if (status == NULL) {
    tapi_warn("");
    return SS_CLI_STATUS_UNKNOWN;
  }

  if (g_strcmp0(status, "enabled") == 0)
    return SS_CLI_STATUS_ENABLED;

  if (g_strcmp0(status, "disabled") == 0)
    return SS_CLI_STATUS_DISABLED;

  tapi_warn("cli status is unknown: %s", status);
  return SS_CLI_STATUS_UNKNOWN;
}

static void _on_response_get_call_waiting(GObject *source_object,
    GAsyncResult *result, void *user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  tapi_bool status;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *str_status = NULL;

  dbus_result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object),
      result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "VoiceCallWaiting") == 0) {
      g_variant_get(var_val, "s", &str_status);
      tapi_debug("VoiceCallWaiting: %s", str_status);

      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  if (str_status == NULL) {
    CALL_RESP_CALLBACK(TAPI_RESULT_FAIL, NULL, cbd);
    goto done;
  }

  if (g_strcmp0(str_status, "disabled") == 0)
    status = FALSE;
  else if(g_strcmp0(str_status, "enabled") == 0)
    status = TRUE;
  else {
    CALL_RESP_CALLBACK(TAPI_RESULT_FAIL, NULL, cbd);
    goto done;
  }

  CALL_RESP_CALLBACK(ret, &status, cbd);
done:
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
  g_free(str_status);
}

EXPORT_API void ofono_ss_get_call_waiting(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_call_waiting, cbd);
}

EXPORT_API void ofono_ss_set_call_waiting(struct ofono_modem *modem,
      tapi_bool enable, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;
  const char *str;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  if(enable)
    str = "enabled";
  else
    str = "disabled";

  var = g_variant_new("(sv)", "VoiceCallWaiting", g_variant_new_string(str));

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static void _on_response_get_call_forwarding(GObject *source_object,
    GAsyncResult *result, void * user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  struct call_forward_setting settings[SS_CF_CONDITION_CFNRC + 1];
  struct call_forward_setting *ps = NULL;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *number = NULL;

  memset(settings, 0, sizeof(settings));
  dbus_result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object),
      result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    number = NULL;
    if (g_strcmp0(key, "VoiceUnconditional") == 0) {
      ps = settings + SS_CF_CONDITION_CFU;
      g_variant_get(var_val, "s", &number);
      ps->condition = SS_CF_CONDITION_CFU;
      if (number != NULL && strlen(number) > 0) {
        ps->enable = TRUE;
        g_strlcpy(ps->num, number, CALL_DIGIT_LEN_MAX);
      } else {
        ps->enable = FALSE;
      }
    } else if (g_strcmp0(key, "VoiceBusy") == 0) {
      ps = settings + SS_CF_CONDITION_CFB;
      g_variant_get(var_val, "s", &number);
      ps->condition = SS_CF_CONDITION_CFB;
      if (number != NULL && strlen(number) > 0) {
        ps->enable = TRUE;
        g_strlcpy(ps->num, number, CALL_DIGIT_LEN_MAX);
      } else {
        ps->enable = FALSE;
      }
    } else if (g_strcmp0(key, "VoiceNotReachable") == 0) {
      ps = settings + SS_CF_CONDITION_CFNRC;
      g_variant_get(var_val, "s", &number);
      ps->condition = SS_CF_CONDITION_CFNRC;
      if (number != NULL && strlen(number) > 0) {
        ps->enable = TRUE;
        g_strlcpy(ps->num, number, CALL_DIGIT_LEN_MAX);
      } else {
        ps->enable = FALSE;
      }
    } else if (g_strcmp0(key, "VoiceNoReply") == 0) {
      ps = settings + SS_CF_CONDITION_CFNRY;
      g_variant_get(var_val, "s", &number);
      ps->condition = SS_CF_CONDITION_CFNRY;
      if (number != NULL && strlen(number) > 0) {
        ps->enable = TRUE;
        g_strlcpy(ps->num, number, CALL_DIGIT_LEN_MAX);
      } else {
        ps->enable = FALSE;
      }
    }  else if (g_strcmp0(key, "VoiceNoReplyTimeout") == 0) {
      unsigned short timeout;
      ps = settings + SS_CF_CONDITION_CFNRY;
      g_variant_get(var_val, "q", &timeout);
      ps->timeout = (unsigned char)timeout;
    }

    if (ps)
      tapi_debug("%s(%d): %s %d %d", key, ps->condition, ps->num, ps->enable,
          ps->timeout);

    g_free(number);
    g_free(key);
    g_variant_unref(var_val);
  }

  CALL_RESP_CALLBACK(ret, settings, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
}

EXPORT_API void ofono_ss_get_call_forward(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_FORWARDING_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_call_forwarding, cbd);
}

static void _on_response_set_call_forward_noreply(GObject *source_object,
      GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *dbus_result, *var;
  GError *error = NULL;
  struct interm_response_cb_data *icbd = user_data;
  guint16 timeout = *(unsigned char*)icbd->user_data;

  dbus_result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object),
        result, &error);
  g_variant_unref(dbus_result);

  ret = ofono_error_parse(error);
  if (error)
    g_error_free(error);
  g_free(icbd->user_data);
  if (ret != TAPI_RESULT_OK) {
    struct response_cb_data *cbd = icbd->cbd;
    if (cbd->cb != NULL)
      cbd->cb(ret, NULL, cbd->user_data);

    g_free(cbd);
    g_free(icbd);
    return;
  }

  /* set no reply timeout */
  var = g_variant_new("(sv)", "VoiceNoReplyTimeout", g_variant_new("q", timeout));

  g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE,
        icbd->modem->path,
        OFONO_CALL_FORWARDING_IFACE, "SetProperty", var,
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
        on_response_common, icbd->cbd);
  g_free(icbd);
}

EXPORT_API void ofono_ss_set_call_forward(struct ofono_modem *modem,
      struct call_forward_setting *setting,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;
  struct interm_response_cb_data *icbd;
  unsigned char *timeout;

  tapi_debug("");

  CHECK_PARAMETERS(modem && _check_call_forwarding_setting(setting), cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(sv)", _condition_to_str(setting->condition),
      g_variant_new_string(setting->num));

  if (setting->condition != SS_CF_CONDITION_CFNRY || !setting->enable) {
    g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_FORWARDING_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
    return;
  }

  timeout = g_memdup(&setting->timeout, sizeof(setting->timeout));
  NEW_INTERM_RSP_CB_DATA(icbd, cbd, modem, timeout);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_FORWARDING_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_set_call_forward_noreply, icbd);
}

static void _on_response_get_call_barring(GObject *source_object,
    GAsyncResult *result, void *user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  struct call_barring_setting setting;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *type;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    type = NULL;
    if (g_strcmp0(key, "VoiceIncoming") == 0) {
      g_variant_get(var_val, "s", &type);
      if (g_strcmp0(type, "always") == 0)
        setting.incoming = SS_CB_TYPE_BAIC;
      else if (g_strcmp0(type, "whenroaming") == 0)
        setting.incoming = SS_CB_TYPE_BIC_ROAM;
      else
        setting.incoming = SS_CB_TYPE_NONE;
    } else if (g_strcmp0(key, "VoiceOutgoing") == 0) {
      g_variant_get(var_val, "s", &type);
      if (g_strcmp0(type, "all") == 0)
        setting.outgoing = SS_CB_TYPE_BAOC;
      else if (g_strcmp0(type, "international") == 0)
        setting.outgoing = SS_CB_TYPE_BOIC;
      else if (g_strcmp0(type, "internationalnothome") == 0)
        setting.outgoing = SS_CB_TYPE_BOIC_NOT_HC;
      else
        setting.outgoing = SS_CB_TYPE_NONE;
    }

    tapi_debug("%s: %s", key, type);

    g_free(type);
    g_free(key);
    g_variant_unref(var_val);
  }

  CALL_RESP_CALLBACK(ret, &setting, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
}

EXPORT_API void ofono_ss_get_call_barring(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_BARRING_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_call_barring, cbd);
}

EXPORT_API void ofono_ss_set_call_barring(struct ofono_modem *modem,
      tapi_bool enable,
      enum call_barring_type type,
      const char *pwd,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;
  const char *method = NULL;
  const char *prop;
  const char *value;

  tapi_debug("");

  CHECK_PARAMETERS(modem && pwd && strlen(pwd) == SS_PW_LEN_MAX, cb, user_data);

  if (enable == FALSE) {
    if (type == SS_CB_TYPE_AB)
      method = "DisableAll";
    else if (type == SS_CB_TYPE_BAOC)
      method = "DisableAllOutgoing";
    else if (type == SS_CB_TYPE_BAIC)
      method = "DisableAllIncoming";
  }

  if (method != NULL)
    var = g_variant_new("(s)", pwd);
  else {
    method = "SetProperty";

    switch (type) {
    case SS_CB_TYPE_BAOC:
      prop = "VoiceOutgoing";
      value = "all";
      break;
    case SS_CB_TYPE_BOIC:
      prop = "VoiceOutgoing";
      value = "international";
      break;
    case SS_CB_TYPE_BOIC_NOT_HC:
      prop = "VoiceOutgoing";
      value = "internationalnothome";
      break;
    case SS_CB_TYPE_BAIC:
      prop = "VoiceIncoming";
      value = "always";
      break;
    case SS_CB_TYPE_BIC_ROAM:
      prop = "VoiceIncoming";
      value = "whenroaming";
      break;
    case SS_CB_TYPE_AG:
      prop = "VoiceOutgoing";
      value = "disabled";
      break;
    case SS_CB_TYPE_AC:
      prop = "VoiceIncoming";
      value = "disabled";
      break;
    default: /* invaid input */
      CHECK_PARAMETERS(FALSE, cb, user_data);
      break;
    }

    var = g_variant_new("(svs)", prop, g_variant_new_string(value), pwd);
  }

  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, method, var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_ss_change_barring_password(struct ofono_modem *modem,
      const char *old_pwd,
      const char *new_pwd,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("");

  CHECK_PARAMETERS(modem && old_pwd && new_pwd, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(ss)", old_pwd, new_pwd);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_BARRING_IFACE, "ChangePassword", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static void _on_response_get_cli_status(GObject *source_object,
    GAsyncResult *result, void * user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  enum cli_status status[5];

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *val;

  memset(status, 0, sizeof(status));
  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    val = NULL;
    if (g_strcmp0(key, "CallingLinePresentation") == 0) {
      g_variant_get(var_val, "s", &val);
      status[SS_CLI_CLIP] = _str_to_cli_status(val);
      tapi_debug("%s: %s", key, val);
    } else if (g_strcmp0(key, "CalledLinePresentation") == 0) {
      g_variant_get(var_val, "s", &val);
      status[SS_CLI_CDIP] = _str_to_cli_status(val);
      tapi_debug("%s: %s", key, val);
    } else if (g_strcmp0(key, "CallingNamePresentation") == 0) {
      g_variant_get(var_val, "s", &val);
      status[SS_CLI_CNAP] = _str_to_cli_status(val);
      tapi_debug("%s: %s", key, val);
    } else if (g_strcmp0(key, "ConnectedLinePresentation") == 0) {
      g_variant_get(var_val, "s", &val);
      status[SS_CLI_COLP] = _str_to_cli_status(val);
      tapi_debug("%s: %s", key, val);
    }  else if (g_strcmp0(key, "ConnectedLineRestriction") == 0) {
      g_variant_get(var_val, "s", &val);
      status[SS_CLI_COLR] = _str_to_cli_status(val);
      tapi_debug("%s: %s", key, val);
    }

    g_free(val);
    g_free(key);
    g_variant_unref(var_val);
  }

  CALL_RESP_CALLBACK(ret, status, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
}

EXPORT_API void ofono_ss_get_cli_status(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_cli_status, cbd);
}

static void _on_response_get_clir(GObject *source_object,
    GAsyncResult *result, void * user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  enum clir_network_status status;

  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  char *val;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    val = NULL;
    if (g_strcmp0(key, "CallingLineRestriction") == 0) {
      g_variant_get(var_val, "s", &val);
      if (g_strcmp0(val, "disabled") == 0)
        status = SS_CLIR_NW_STATUS_DISABLED;
      else if (g_strcmp0(val, "permanent") == 0)
        status = SS_CLIR_NW_STATUS_PERMANENT;
      else if (g_strcmp0(val, "unknown") == 0)
        status = SS_CLIR_NW_STATUS_UNKOWN;
      else if (g_strcmp0(val, "on") == 0)
        status = SS_CLIR_NW_STATUS_ON;
      else if (g_strcmp0(val, "off") == 0)
        status = SS_CLIR_NW_STATUS_OFF;

      tapi_debug("%s: %s", key, val);

      g_free(val);
      g_free(key);
      g_variant_unref(var_val);
      break;
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  CALL_RESP_CALLBACK(ret, &status, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(dbus_result);
}

EXPORT_API void ofono_ss_get_clir(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, "GetProperties", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_clir, cbd);
}

EXPORT_API void ofono_ss_set_clir(struct ofono_modem *modem,
      enum clir_dev_status status, response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;
  const char *str;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  switch (status) {
  case SS_CLIR_DEV_STATUS_DEFAULT:
    str = "default";
    break;
  case SS_CLIR_DEV_STATUS_ENABLED:
    str = "enabled";
    break;
  case SS_CLIR_DEV_STATUS_DISABLED:
    str = "disabled";
  default:
    str = "default";
    break;
  }

  var = g_variant_new("(sv)", "HideCallerId", g_variant_new_string(str));

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CALL_SETTINGS_IFACE, "SetProperty", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static void _on_response_initiate_ussd_request(GObject *source_object,
    GAsyncResult *result, void *user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *req_type;
  GVariant *var_data;
  char *rsp = NULL;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(sv)", &req_type, &var_data);
  if (g_strcmp0(req_type, "USSD") == 0)
    g_variant_get(var_data, "s", &rsp);
  else
    tapi_warn("Not ussd request");

  CALL_RESP_CALLBACK(ret, rsp, cbd);
  g_variant_unref(dbus_result);
  g_variant_unref(var_data);
  g_free(rsp);
}

EXPORT_API void ofono_ss_initiate_ussd_request(struct ofono_modem *modem,
      char *str, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var = NULL;

  tapi_debug("");

  CHECK_PARAMETERS(modem && str, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  //#TODO: forbid MMI string here
  var = g_variant_new("(s)", str);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE, "Initiate", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_initiate_ussd_request, cbd);
}

static void _on_response_send_ussd_response(GObject *source_object,
      GAsyncResult *result, void *user_data)
{
  TResult ret;
  GVariant *dbus_result;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *rsp = NULL;

  dbus_result = g_dbus_connection_call_finish(
      G_DBUS_CONNECTION(source_object), result, &error);

  CHECK_RESULT(ret, error, cbd, dbus_result);

  g_variant_get(dbus_result, "(s)", &rsp);

  CALL_RESP_CALLBACK(ret, &rsp, cbd);
  g_variant_unref(dbus_result);
  g_free(rsp);
}

EXPORT_API void ofono_ss_send_ussd_response(struct ofono_modem *modem,
      char *str, response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && str, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(s)", str);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE, "Respond", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_send_ussd_response, cbd);
}

EXPORT_API void ofono_ss_cancel_ussd_session(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SUPPLEMENTARY_SERVICES_IFACE, "Cancel", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}
