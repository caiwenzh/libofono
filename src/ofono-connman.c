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

#include "ofono-connman.h"
#include "common.h"
#include "log.h"

struct set_context_interm_info {
  char *path;
  struct pdp_context context;
};

static const char *_context_type_to_str(enum context_type type)
{
  switch (type) {
  case CONTEXT_TYPE_MMS:
    return "mms";
  case CONTEXT_TYPE_INTERNET:
    return "internet";
  case CONTEXT_TYPE_WAP:
    return "wap";
  case CONTEXT_TYPE_IMS:
    return "ims";
  default:
    tapi_warn("unknown type");
    return "";
  }
}

static enum context_type _str_to_context_type(const char *type)
{
  if (type == NULL) {
    tapi_error("context type string is null");
    return CONTEXT_TYPE_UNKNOWN;
  }

  if (g_strcmp0(type, "mms") == 0)
    return CONTEXT_TYPE_MMS;

  if (g_strcmp0(type, "internet") == 0)
    return CONTEXT_TYPE_INTERNET;

  if (g_strcmp0(type, "wap") == 0)
    return CONTEXT_TYPE_WAP;

  if (g_strcmp0(type, "ims") == 0)
    return CONTEXT_TYPE_IMS;

  tapi_warn("unknown context type: %s", type);
  return CONTEXT_TYPE_UNKNOWN;
}

static const char *_context_ip_type_to_str(enum ip_protocol protocol)
{
  switch(protocol) {
  case IP_PROTOCAL_IPV4:
    return "ip";
  case IP_PROTOCAL_IPV6:
    return "ipv6";
  case IP_PROTOCAL_IPV4_IPV6:
    return "dual";
  default:
    tapi_warn("Unknown ip protocol type");
    return "";
  }
}

static void _on_response_add_context(GObject *obj, GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *path = NULL;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(o)", &path);
  tapi_debug("Path: %s", path);

  CALL_RESP_CALLBACK(ret, path, cbd);
  g_variant_unref(resp);
  g_free(path);
}

EXPORT_API void ofono_connman_add_context(struct ofono_modem *modem,
      enum context_type type, response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);
  tapi_debug("Type: %d", type);

  var = g_variant_new("(s)", _context_type_to_str(type));
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CONNMAN_IFACE, "AddContext", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_add_context, cbd);
}

EXPORT_API void ofono_connman_remove_context(struct ofono_modem *modem,
      const char *path, response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && path, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Path: %s", path);

  var = g_variant_new("(o)", path);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CONNMAN_IFACE, "RemoveContext", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static void _on_response_set_context(GObject *obj, GAsyncResult *result,
    gpointer user_data)
{
  TResult ret;
  GVariant *dbus_result, *var, *val;
  GError *error = NULL;
  struct interm_response_cb_data *icbd = user_data;
  struct response_cb_data *cbd = icbd->cbd;
  struct set_context_interm_info *iinfo = icbd->user_data;

  dbus_result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result,
       &error);
  g_variant_unref(dbus_result);

  ret = ofono_error_parse(error);
  if (error)
    g_error_free(error);
  if (ret != TAPI_RESULT_OK)
    goto done;

  if (iinfo->context.apn != NULL) {
    tapi_debug("APN: %s", iinfo->context.apn);
    var = g_variant_new_string(iinfo->context.apn);
    val = g_variant_new("(sv)", "AccessPointName", var);
    g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE, iinfo->path,
        OFONO_CONTEXT_IFACE, "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, _on_response_set_context, icbd);
    g_free(iinfo->context.apn);
    iinfo->context.apn = NULL;
    return;
  }

  if (iinfo->context.user_name != NULL) {
    tapi_debug("User: %s", iinfo->context.user_name);
    var = g_variant_new_string(iinfo->context.user_name);
    val = g_variant_new("(sv)", "Username", var);
    g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE, iinfo->path,
        OFONO_CONTEXT_IFACE, "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, _on_response_set_context, icbd);
    g_free(iinfo->context.user_name);
    iinfo->context.user_name = NULL;
    return;
  }

  if (iinfo->context.pwd != NULL) {
    tapi_debug("Password: %s", iinfo->context.pwd);
    var = g_variant_new_string(iinfo->context.pwd);
    val = g_variant_new("(sv)", "Password", var);
    g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE, iinfo->path,
        OFONO_CONTEXT_IFACE, "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, _on_response_set_context, icbd);
    g_free(iinfo->context.pwd);
    iinfo->context.pwd = NULL;
    return;
  }

  if (iinfo->context.proxy != NULL) {
    tapi_debug("MessageProxy: %s", iinfo->context.proxy);
    var = g_variant_new_string(iinfo->context.proxy);
    val = g_variant_new("(sv)", "MessageProxy", var);
    g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE, iinfo->path,
        OFONO_CONTEXT_IFACE, "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, _on_response_set_context, icbd);
    g_free(iinfo->context.proxy);
    iinfo->context.proxy = NULL;
    return;
  }

  if (iinfo->context.mmsc != NULL) {
    tapi_debug("MessageCenter: %s", iinfo->context.mmsc);
    var = g_variant_new_string(iinfo->context.mmsc);
    val = g_variant_new("(sv)", "MessageCenter", var);
    g_dbus_connection_call(icbd->modem->conn, OFONO_SERVICE, iinfo->path,
        OFONO_CONTEXT_IFACE, "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, _on_response_set_context, icbd);
    g_free(iinfo->context.mmsc);
    iinfo->context.mmsc = NULL;
    return;
  }

done:
  if (cbd->cb != NULL)
    cbd->cb(ret, NULL, cbd->user_data);

  g_free(cbd);
  g_free(icbd);
  g_free(iinfo->path);
  g_free(iinfo->context.apn);
  g_free(iinfo->context.user_name);
  g_free(iinfo->context.pwd);
  g_free(iinfo->context.proxy);
  g_free(iinfo->context.mmsc);
  g_free(iinfo);
}

EXPORT_API void ofono_connman_set_context(struct ofono_modem *modem,
      char *path, struct pdp_context *context,
      response_cb cb, void *user_data)
{
  GVariant *var, *val;
  struct response_cb_data *cbd;
  struct interm_response_cb_data *icbd;
  struct set_context_interm_info *iinfo;

  CHECK_PARAMETERS(modem && path && context, cb, user_data);
  tapi_debug("Path: %s", path);

  iinfo = g_new0(struct set_context_interm_info, 1); 
  memset(iinfo, 0, sizeof(struct set_context_interm_info));
  iinfo->path = g_strdup(path);
  if (context->apn)
    iinfo->context.apn = g_strdup(context->apn);
  if (context->user_name)
    iinfo->context.user_name = g_strdup(context->user_name);
  if (context->pwd)
    iinfo->context.pwd = g_strdup(context->pwd);
  if (context->proxy)
    iinfo->context.proxy = g_strdup(context->proxy);
  if (context->mmsc)
    iinfo->context.mmsc = g_strdup(context->mmsc);
  NEW_RSP_CB_DATA(cbd, cb, user_data);
  NEW_INTERM_RSP_CB_DATA(icbd, cbd, modem, iinfo);

  tapi_debug("Protocol: %d", context->protocol);

  var = g_variant_new_string(_context_ip_type_to_str(context->protocol));
  val = g_variant_new("(sv)", "Protocol", var);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, path, OFONO_CONTEXT_IFACE,
      "SetProperty", val, NULL, G_DBUS_CALL_FLAGS_NONE, -1,
      NULL, _on_response_set_context, icbd);
}

EXPORT_API tapi_bool ofono_connman_get_context_info(struct ofono_modem *modem,
      char *path, struct pdp_context_info *info)
{
  GError *error = NULL;
  GVariant *var_properties, *var_val;
  GVariantIter *iter;
  char *key;

  if (modem == NULL || info == NULL || path == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_CONNMAN)) {
    tapi_warn("OFONO_API_CONNMAN doesn't exist");
    return FALSE;
  }

  tapi_debug("Path: %s", path);

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, path, OFONO_CONTEXT_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  memset(info, 0, sizeof(struct pdp_context_info));
  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Type") == 0) {
      const char *type = g_variant_get_string(var_val, NULL);
      info->type = _str_to_context_type(type);
      tapi_debug("Type(%d): %s", info->type, type);
    } else if (g_strcmp0(key, "Active") == 0) {
      g_variant_get(var_val, "b", &info->actived);
      tapi_debug("Actived: %d", info->actived);
    } else if (g_strcmp0(key, "Settings") == 0) {
      char *k;
      GVariant *v;
      GVariantIter *v4_iter;

      g_variant_get(var_val, "a{sv}", &v4_iter);
      while(g_variant_iter_next(v4_iter, "{sv}", &k, &v)) {
        if (g_strcmp0(k, "Interface") == 0) {
          g_variant_get(v, "s", &info->ipv4.iface);
          tapi_debug("Interface: %s", info->ipv4.iface);
        } else if (g_strcmp0(k, "Address") == 0) {
          g_variant_get(v, "s", &info->ipv4.ip);
          tapi_debug("Address: %s", info->ipv4.ip);
        } else if (g_strcmp0(k, "Netmask") == 0) {
          g_variant_get(v, "s", &info->ipv4.netmask);
          tapi_debug("Netmask: %s", info->ipv4.netmask);
        } else if (g_strcmp0(k, "Gateway") == 0) {
          g_variant_get(v, "s", &info->ipv4.gateway);
          tapi_debug("Gateway: %s", info->ipv4.gateway);
        } else if (g_strcmp0(k, "Proxy") == 0) {
          g_variant_get(v, "s", &info->ipv4.proxy);
          tapi_debug("Proxy: %s", info->ipv4.proxy);
        } else if (g_strcmp0(k, "DomainNameServers") == 0) {
          GVariantIter *dns_iter;
          tapi_bool next;
          g_variant_get(v, "as", &dns_iter);
          next = g_variant_iter_next(dns_iter, "s", &info->ipv4.dns[0]);
          tapi_debug("DNS: %s", info->ipv4.dns[0]);
          if (next) {
            g_variant_iter_next(dns_iter, "s", &info->ipv4.dns[1]);
            tapi_debug("DNS: %s", info->ipv4.dns[1]);
          }
          g_variant_iter_free(dns_iter);
        }

        g_free(k);
        g_variant_unref(v);
      }
      g_variant_iter_free(v4_iter);
    } else if (g_strcmp0(key, "IPv6.Settings") == 0) {
      char *k;
      GVariant *v;
      GVariantIter *v6_iter;

      g_variant_get(var_val, "a{sv}", &v6_iter);
      while(g_variant_iter_next(v6_iter, "{sv}", &k, &v)) {
        if (g_strcmp0(k, "Interface") == 0) {
          g_variant_get(v, "s", &info->ipv6.iface);
          tapi_debug("Interface: %s", info->ipv6.iface);
        } else if (g_strcmp0(k, "Address") == 0) {
          g_variant_get(v, "s", &info->ipv6.ip);
          tapi_debug("Address: %s", info->ipv6.ip);
        } else if (g_strcmp0(k, "PrefixLength") == 0) {
          g_variant_get(v, "y", &info->ipv6.prefix_len);
          tapi_debug("PrefixLength: %d", info->ipv6.prefix_len);
        } else if (g_strcmp0(k, "Gateway") == 0) {
          g_variant_get(v, "s", &info->ipv6.gateway);
          tapi_debug("Gateway: %s", info->ipv6.gateway);
        } else if (g_strcmp0(k, "DomainNameServers") == 0) {
          GVariantIter *dns_iter;
          tapi_bool next;
          g_variant_get(v, "as", &dns_iter);
          next = g_variant_iter_next(dns_iter, "s", &info->ipv6.dns[0]);
          tapi_debug("DNS: %s", info->ipv6.dns[0]);
          if (next) {
            g_variant_iter_next(dns_iter, "s", &info->ipv6.dns[1]);
            tapi_debug("DNS: %s", info->ipv6.dns[1]);
          }
          g_variant_iter_free(dns_iter);
        }
        g_free(k);
        g_variant_unref(v);
      }
      g_variant_iter_free(v6_iter);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  return TRUE;
}

EXPORT_API tapi_bool ofono_connman_get_contexts(struct ofono_modem *modem,
      struct str_list **contexts)
{
  GError *error = NULL;
  GVariant *var, *var_props;
  GVariantIter *iter;
  char *path;
  int i = 0;

  if (modem == NULL || contexts == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  if (*contexts != NULL) {
    tapi_warn("should initialize to NULL");
    *contexts = NULL;
  }

  if (!has_interface(modem->interfaces, OFONO_API_CONNMAN)) {
    tapi_warn("OFONO_API_CONNMAN doesn't exist");
    return FALSE;
  }

  var = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path, OFONO_CONNMAN_IFACE,
      "GetContexts", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  g_variant_get(var, "(a(oa{sv}))", &iter);
  *contexts = g_malloc(sizeof(struct str_list));
  (*contexts)->count = g_variant_iter_n_children(iter);
  (*contexts)->data = g_malloc(sizeof(char *) * (*contexts)->count);
  while (g_variant_iter_next(iter, "(o@a{sv})", &path, &var_props)) {
    tapi_debug("Path: %s", path);
    (*contexts)->data[i++] = path;

    g_variant_unref(var_props);
  }
  g_variant_iter_free(iter);
  g_variant_unref(var);

  return TRUE;
}

EXPORT_API void ofono_connman_activate_context(struct ofono_modem *modem,
      char *path, response_cb cb, void *user_data)
{
  GVariant *var;

  tapi_debug("");
  CHECK_PARAMETERS(modem && path, cb, user_data);
  tapi_debug("Path: %s", path);

  var = g_variant_new_boolean(TRUE);
  ofono_set_property(modem, OFONO_CONTEXT_IFACE, path, "Active",
        var, cb, user_data);
}

EXPORT_API void ofono_connman_deactivate_context(struct ofono_modem *modem,
      char *path, response_cb cb, void *user_data)
{
  GVariant *var;

  CHECK_PARAMETERS(modem && path, cb, user_data);
  tapi_debug("Path: %s", path);

  var = g_variant_new_boolean(FALSE);
  ofono_set_property(modem, OFONO_CONTEXT_IFACE, path, "Active",
        var, cb, user_data);
}

EXPORT_API void ofono_connman_deactivate_all_contexts(struct ofono_modem *modem,
      response_cb cb, void *user_data)
{
  struct response_cb_data *cbd;

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("");

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_CONNMAN_IFACE, "DeactivateAll", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

static tapi_bool _get_bool(struct ofono_modem *modem, char *property,
      tapi_bool *b)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  if (modem == NULL || property == NULL || b == NULL) {
    tapi_error("Invalid parmeter");
    return FALSE;
  }

  if (!has_interface(modem->interfaces, OFONO_API_CONNMAN)) {
    tapi_error("OFONO_API_CONNMAN doesn't exist");
    return FALSE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path,
      OFONO_CONNMAN_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, property) == 0) {
      g_variant_get(var_val, "b", b);
      tapi_info("%s: %d", property, *b);

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

EXPORT_API tapi_bool ofono_connman_get_powered(struct ofono_modem *modem,
      tapi_bool *powered)
{
  tapi_debug("");
  return _get_bool(modem, "Powered", powered);
}

EXPORT_API void ofono_connman_set_powered(struct ofono_modem *modem,
      tapi_bool powered, response_cb cb,
      void *user_data)
{
  GVariant *var;

  CHECK_PARAMETERS(modem, cb, user_data);
  tapi_debug("Powered: %d", powered);

  var = g_variant_new_boolean(powered);
  ofono_set_property(modem, OFONO_CONNMAN_IFACE, modem->path, "Powered",
        var, cb, user_data);
}

EXPORT_API tapi_bool ofono_connman_get_roaming_allowed(struct ofono_modem *modem,
      tapi_bool *allowed)
{
  tapi_debug("");
  return _get_bool(modem, "RoamingAllowed", allowed);
}

EXPORT_API void ofono_connman_set_roaming_allowed(struct ofono_modem *modem,
      tapi_bool allowed, response_cb cb,
      void *user_data)
{
  GVariant *var;

  CHECK_PARAMETERS(modem, cb, user_data);
  tapi_debug("Allowed: %d", allowed);

  var = g_variant_new_boolean(allowed);
  ofono_set_property(modem, OFONO_CONNMAN_IFACE, modem->path,
      "RoamingAllowed", var, cb, user_data);
}

EXPORT_API tapi_bool ofono_connman_get_attached(struct ofono_modem *modem,
      tapi_bool *attached)
{
  tapi_debug("");
  return _get_bool(modem, "Attached", attached);
}

