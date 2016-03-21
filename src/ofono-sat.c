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
#include <stdio.h>

#include "log.h"
#include "common.h"
#include "ofono-sat.h"

#define SAT_ERROR_PREFIX  "Error.SimToolkit"
#define SAT_GOBACK_ERROR SAT_ERROR_PREFIX ".GoBack"
#define SAT_ENDSESSION_ERROR SAT_ERROR_PREFIX ".EndSession"
#define SAT_BUSY_ERROR SAT_ERROR_PREFIX ".Busy"

#define INVOKE_CALLBACK(_cb, _data, _agent) \
  do { \
    if (_cb == NULL) { \
      g_dbus_method_invocation_return_dbus_error( \
        _agent->invocation, SAT_GOBACK_ERROR, "Not supported"); \
      _agent->invocation = NULL; \
    } \
    else \
      _cb(_data, _agent->user_data); \
  } while(0)

/* The agent dbus interfaces:
  "service": system assigned
  "path": the same as the path of the modem (modem->path)
  "API": as below xml
*/
static const gchar introspection_xml[] =
"<node>"
"  <interface name='org.ofono.SimToolkitAgent'>"
"    <method name='DisplayText'>"
"      <arg type='s' name='text' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='b' name='urgent' direction='in'/>"
"    </method>"
"    <method name='RequestSelection'>"
"      <arg type='s' name='title' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='a(sy)' name='items' direction='in'/>"
"      <arg type='n' name='default' direction='in'/>"
"      <arg type='y' name='ret' direction='out'/>"
"    </method>"
"    <method name='RequestInput'>"
"      <arg type='s' name='alpha' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='s' name='default' direction='in'/>"
"      <arg type='y' name='min' direction='in'/>"
"      <arg type='y' name='max' direction='in'/>"
"      <arg type='b' name='hide_typing' direction='in'/>"
"      <arg type='s' name='str' direction='out'/>"
"    </method>"
"    <method name='RequestDigits'>"
"      <arg type='s' name='alpha' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='s' name='default' direction='in'/>"
"      <arg type='y' name='min' direction='in'/>"
"      <arg type='y' name='max' direction='in'/>"
"      <arg type='b' name='hide_typing' direction='in'/>"
"      <arg type='s' name='str' direction='out'/>"
"    </method>"
"    <method name='RequestKey'>"
"      <arg type='s' name='alpha' direction='in' />"
"      <arg type='y' name='icon_id' direction='in' />"
"      <arg type='s' name='str' direction='out'/>"
"    </method>"
"    <method name='RequestDigit'>"
"      <arg type='s' name='alpha' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='s' name='str' direction='out'/>"
"    </method>"
"    <method name='RequestQuickDigit'>"
"      <arg type='s' name='alpha' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='s' name='str' direction='out'/>"
"    </method>"
"    <method name='RequestConformation'>"
"      <arg type='s' name='alpha' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='b' name ='ret' direction='out'/>"
"    </method>"
"    <method name='ConfirmCallSetup'>"
"      <arg type='s' name='information' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='b' name ='ret' direction='out'/>"
"    </method>"
"    <method name='PlayTone'>"
"      <arg type='s' name='tone' direction='in'/>"
"      <arg type='s' name='text' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"    </method>"
"    <method name='LoopTone'>"
"      <arg type='s' name='tone' direction='in'/>"
"      <arg type='s' name='text' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"    </method>"
"    <method name='DisplayActionInformation'>"
"      <arg type='s' name='text' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"    </method>"
"    <method name='ConfirmLaunchBrowser'>"
"      <arg type='s' name='information' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"      <arg type='s' name='url' direction='in'/>"
"    </method>"
"    <method name='DisplayAction'>"
"      <arg type='s' name='text' direction='in'/>"
"      <arg type='y' name='icon_id' direction='in'/>"
"    </method>"
"    <method name='ConfirmOpenChannel'>"
"      <arg type='s' name ='information' direction='in'/>"
"      <arg type='y' name ='icon_id' direction='in'/>"
"      <arg type='b' name ='ret' direction='out'/>"
"    </method>"
"    <method name='Cancel'>"
"    </method>"
"    <method name='Release'>"
"    </method>"
"  </interface>"
"</node>";

struct sat_dbus_method {
  const gchar *name;
  void (*method) (GVariant*, gpointer, GDBusMethodInvocation*);
};

struct ofono_sat_agent {
  struct ofono_modem *modem;
  guint registration_id;
  GDBusMethodInvocation *invocation;/* invocation waiting for response */
  gpointer user_data;
  struct sat_agent_callbacks *callbacks;
};

static void _get_inkey(GVariant *value, gpointer user_data,
      GDBusMethodInvocation* invocation, tapi_bool is_num,
      tapi_bool quick_resp, tapi_bool yes_no)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_get_inkey *inkey;

  tapi_info("...");

  agent->invocation = invocation;
  inkey = g_new0(struct sat_command_get_inkey, 1);

  g_variant_get(value, "(sy)", &inkey->text, &inkey->icon_id);
  inkey->is_num = is_num;
  inkey->quick_resp = quick_resp;
  inkey->yes_no = yes_no;

  INVOKE_CALLBACK(agent->callbacks->request_key, inkey, agent);

  g_free(inkey->text);
  g_free(inkey);
}

static void _do_play_tone(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation, tapi_bool loop)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_play_tone *playtone;

  tapi_info("...");

  agent->invocation = invocation;
  playtone = g_new0(struct sat_command_play_tone, 1);
  g_variant_get(value, "(ssy)", &playtone->tone, &playtone->text,
        &playtone->icon_id);
  playtone->loop = loop;

  INVOKE_CALLBACK(agent->callbacks->play_tone, playtone, agent);

  g_free(playtone->text);
  g_free(playtone->tone);
  g_free(playtone);
}

static void _display_text(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_display_text *display;

  tapi_info("...");

  agent->invocation = invocation;
  display = g_new0(struct sat_command_display_text, 1);

  g_variant_get(value, "(syb)", &display->text, &display->icon_id,
      &display->urgent);

  tapi_debug("dispaly text: %s urgent: %d", display->text, display->urgent);

  INVOKE_CALLBACK(agent->callbacks->display_text, display, agent);

  g_free(display->text);
  g_free(display);
}

static void _request_selection(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_select_item *si;
  GVariantIter *val_iter = NULL;
  int i ;

  tapi_info("...");
  tapi_debug("variant type = %s",  g_variant_get_type_string(value));

  agent->invocation = invocation;
  si = g_new0(struct sat_command_select_item, 1);
  g_variant_get(value, "(sya(sy)n)", &si->title, &si->icon_id,
      &val_iter, &si->def_sel);

  si->item_count = g_variant_iter_n_children(val_iter);
  tapi_debug("there are %d items", si->item_count);

  si->items = g_new0(struct sat_menu_item, si->item_count);

  i = 0;
  while (g_variant_iter_next(val_iter, "(sy)", &si->items[i].text,
      &si->items[i].icon_id)) {
    i++;
  }

  INVOKE_CALLBACK(agent->callbacks->request_selection, si, agent);

  if (si->items) {
    for (i = 0; i < si->item_count; i++)
      g_free(si->items[i].text);
    g_free(si->items);
    g_free(si->title);
    g_free(si);
  }
}

static void _request_input(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_get_input *gi;

  tapi_info("...");

  gi = g_new0(struct sat_command_get_input, 1);

  g_variant_get(value, "(sysyyb)", &gi->alpha, &gi->icon_id,
      &gi->default_str, &gi->min,
      &gi->max, &gi->hide);

  gi->is_num = FALSE;

  tapi_debug("text: %s, icon:%d default: %s, min:%d, max:%d, hide:%d",
      gi->alpha, gi->icon_id, gi->default_str, gi->min, gi->max, gi->hide);

  agent->invocation = invocation;

  INVOKE_CALLBACK(agent->callbacks->request_input, gi, agent);

  g_free(gi->alpha);
  g_free(gi->default_str);
  g_free(gi);
}

static void _request_key(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _get_inkey(value, user_data, invocation, FALSE, FALSE, FALSE);
}

static void _request_digit(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _get_inkey(value, user_data, invocation, TRUE, FALSE, FALSE);
}

static void _request_quick_digit(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _get_inkey(value, user_data, invocation, TRUE, TRUE, FALSE);
}

static void _request_digits(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_get_input *gi;

  tapi_info("...");

  agent->invocation = invocation;
  gi = g_new0(struct sat_command_get_input, 1);

  g_variant_get(value, "(sysyyb)", &gi->alpha, &gi->icon_id,
      &gi->default_str, &gi->min, &gi->max, &gi->hide);
  gi->is_num = TRUE;

  INVOKE_CALLBACK(agent->callbacks->request_input, gi, agent);

  g_free(gi->alpha);
  g_free(gi->default_str);
  g_free(gi);
}

static void _request_confirmation(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _get_inkey(value, user_data, invocation, FALSE, FALSE, TRUE);
}

static void _play_tone(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _do_play_tone(value, user_data, invocation, FALSE);
}

static void _loop_tone(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _do_play_tone(value, user_data, invocation, TRUE);
}

static void _confirm_call_setup(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  _get_inkey(value, user_data, invocation, FALSE, FALSE, TRUE);
}

static void _display_action_information(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_display_info *info;

  tapi_info("...");

  agent->invocation = invocation;
  info = g_new0(struct sat_display_info, 1);

  g_variant_get(value, "(sy)", &info->text, &info->icon_id);

  INVOKE_CALLBACK(agent->callbacks->display_action_information, info, agent);

  g_free(info->text);
  g_free(info);
}

static void _confirm_launch_browser(GVariant *value, gpointer user_data,
      GDBusMethodInvocation *invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_command_launch_browser *lb;

  tapi_info("...");

  lb = g_new0(struct sat_command_launch_browser, 1);
  g_variant_get(value, "(sys)", &lb->info, &lb->icon_id, &lb->url);

  INVOKE_CALLBACK(agent->callbacks->confirm_launch_browser, lb, agent);

  g_free(lb->info);
  g_free(lb->url);
  g_free(lb);
}

static void _display_action(GVariant *value, gpointer user_data,
      GDBusMethodInvocation* invocation)
{
  struct ofono_sat_agent *agent = user_data;
  struct sat_display_info *info;

  tapi_info("...");

  agent->invocation = invocation;
  info = g_new0(struct sat_display_info, 1);

  g_variant_get(value, "(sy)", &info->text, &info->icon_id);

  INVOKE_CALLBACK(agent->callbacks->display_action, info, agent);

  g_free(info->text);
  g_free(info);

}
static void _confirm_open_channel(GVariant *value, gpointer user_data,
      GDBusMethodInvocation* invocation)
{
  _get_inkey(value, user_data, invocation, FALSE, FALSE, TRUE);
}

static void _release(GVariant *value, gpointer user_data,
      GDBusMethodInvocation* invocation)
{
  struct ofono_sat_agent *agent = user_data;

  tapi_info("...");

  if (agent->callbacks && agent->callbacks->release)
    agent->callbacks->release(NULL, agent->user_data);

  g_object_unref(invocation);
  agent->invocation = NULL;
}

static void _cancel(GVariant *value, gpointer user_data,
      GDBusMethodInvocation* invocation)
{
  struct ofono_sat_agent *agent = user_data;

  tapi_info("...");

  if (agent->callbacks && agent->callbacks->cancel)
    agent->callbacks->cancel(NULL, agent->user_data);

  g_object_unref(invocation);
  agent->invocation = NULL;
}

static const struct sat_dbus_method agent_methods[] = {
  {"DisplayText",      _display_text},
  {"RequestSelection", _request_selection},
  {"RequestInput",     _request_input},
  {"RequestKey",       _request_key},
  {"RequestDigit",     _request_digit},
  {"RequestDigits",    _request_digits},
  {"RequestQuickDigit",  _request_quick_digit},
  {"RequestConfirmation",_request_confirmation},
  {"PlayTone", _play_tone},
  {"ConfirmCallSetup", _confirm_call_setup},
  {"LoopTone", _loop_tone},
  {"DisplayActionInformation", _display_action_information},
  {"ConfirmLaunchBrowser", _confirm_launch_browser},
  {"DisplayAction", _display_action},
  {"ConfirmOpenChannel", _confirm_open_channel},
  {"Release", _release},
  {"Cancel", _cancel},
};

static void _handle_iface_ofono_methods(GDBusConnection *connection,
      const gchar *sender,
      const gchar *object_path,
      const gchar *interface_name,
      const gchar *method_name,
      GVariant *parameters,
      GDBusMethodInvocation *invocation,
      gpointer user_data)
{
  int i = 0;
  tapi_debug("sender: %s", sender);
  tapi_debug("object path: %s", object_path);
  tapi_debug("interface name: %s", interface_name);
  tapi_debug("method name: %s", method_name);
  for (; i < sizeof(agent_methods)/sizeof(agent_methods[0]); i++) {
    const struct sat_dbus_method *iface = &agent_methods[i];
    if (g_strcmp0(method_name, iface->name) == 0) {
      iface->method(parameters, user_data, invocation);
      return;
    }
  }

  tapi_error("unknown agent method name");
}

static const GDBusInterfaceVTable iface_ofono_sat_vtable = {
  _handle_iface_ofono_methods,
  NULL,
  NULL
};

EXPORT_API struct ofono_sat_agent* ofono_sat_init_agent(
      struct ofono_modem *modem,
      struct sat_agent_callbacks *callbacks,
      void *user_data)
{
  struct ofono_sat_agent *agent;
  GVariant *var, *var_result;
  GError *error = NULL;
  GDBusNodeInfo *introspection_data;
  guint registration_id;

  if (modem == NULL)
    return NULL;

  /* Create agent dbus interfaces, then send object path to ofonod through
     RegisterAgent */
  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
  if (error != NULL) {
    tapi_error("ofono sat xml error : %s", error->message);
    g_error_free(error);
    return NULL;
  }

  agent = g_new0(struct ofono_sat_agent, 1);

  registration_id = g_dbus_connection_register_object(
        modem->conn,
        modem->path,
        introspection_data->interfaces[0],
        &iface_ofono_sat_vtable,
        agent,
        NULL,
        &error
        );
  if (registration_id == 0) {
    tapi_error("register object failed (%s)", error->message);
    g_error_free(error);
    g_dbus_node_info_unref(introspection_data);
    g_free(agent);
    return NULL;
  }

  g_dbus_node_info_unref(introspection_data);

  var = g_variant_new("(o)", modem->path);
  var_result = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
        modem->path,
        OFONO_STK_IFACE, "RegisterAgent", var,
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
        &error);
  if (error != NULL) {
    tapi_error("Error: %s", error->message);
    g_dbus_connection_unregister_object(modem->conn, registration_id);

    g_error_free(error);
    g_free(agent);
    return NULL;
  }
  g_variant_unref(var_result);

  agent->modem = modem;
  agent->user_data = user_data;
  agent->registration_id = registration_id;
  agent->callbacks = g_memdup(callbacks, sizeof(struct sat_agent_callbacks));

  return agent;
}

EXPORT_API void ofono_sat_deinit_agent(struct ofono_sat_agent *agent)
{
  GVariant *var, *var_result;
  GError *error = NULL;
  struct ofono_modem *modem;

  if (agent == NULL || agent->modem == NULL)
    return;

  modem = agent->modem;
  tapi_debug("Agent path: %s", modem->path);

  var = g_variant_new("(o)", modem->path);
  var_result = g_dbus_connection_call_sync(modem->conn,
        OFONO_SERVICE,modem->path,
        OFONO_STK_IFACE, "UnregisterAgent", var,
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
        &error);
  if (error != NULL) {
    tapi_error("Error: %s", error->message);
    g_error_free(error);
  }

  g_variant_unref(var_result);

  if (agent->registration_id > 0)
    g_dbus_connection_unregister_object(modem->conn, agent->registration_id);
  agent->registration_id = 0;

  g_free(agent->callbacks);
  g_free(agent);
}

EXPORT_API void ofono_sat_select_item(struct ofono_modem *modem,
      guchar index, response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  tapi_debug("Index: %d", index);

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  var = g_variant_new("(yo)", index, modem->path);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_STK_IFACE, "SelectItem", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_sat_get_main_menu(struct ofono_modem *modem,
      struct sat_main_menu *menu)
{
  GError *error = NULL;
  GVariant *resp;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;

  tapi_debug("");

  if (modem == NULL || menu == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  resp = g_dbus_connection_call_sync(modem->conn, OFONO_SERVICE,
      modem->path, OFONO_STK_IFACE, "GetProperties",
      NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      &error);

  if (resp == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  memset(menu, 0, sizeof(struct sat_main_menu));
  g_variant_get(resp, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "MainMenuTitle") == 0) {
      g_variant_get(var_val, "s", &menu->title);
      tapi_info("Title: %s", menu->title);
    } else if (g_strcmp0(key, "MainMenuIcon") == 0) {
      g_variant_get(var_val, "y", &menu->icon);
    } else if (g_strcmp0(key, "MainMenu") == 0) {
      GVariantIter *iter_menu;
      struct sat_menu_item *p_item;

      menu->item_count = g_variant_n_children(var_val);
      if (menu->item_count <= 0) {
        tapi_error("Main Menu contain %d items", menu->item_count);

        g_free(key);
        g_variant_unref(var_val);
        goto done;
      }

      menu->items = g_new0(struct sat_menu_item, menu->item_count);
      p_item = menu->items;

      g_variant_get(var_val, "a(sy)", &iter_menu);
      while (g_variant_iter_next(iter_menu, "(sy)",&p_item->text,
            &p_item->icon_id)) {
        tapi_debug("Item: %s(%d)", p_item->text, p_item->icon_id);
        p_item++;
      }

      g_variant_iter_free(iter_menu);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

done:
  g_variant_iter_free(iter);
  g_variant_unref(resp);

  return TRUE;
}

EXPORT_API void ofono_sat_send_response(struct ofono_sat_agent *agent,
      enum sat_result result, enum sat_response_type type,
      void *data)
{
  GVariant *var = NULL;

  if (NULL == agent->invocation) {
    tapi_warn("No invocation exist");
    return;
  }

  switch (result) {
  case SAT_RESP_OK: {
    switch (type) {
    case SAT_RESP_TYPE_BOOL:
      var = g_variant_new("(b)", *(tapi_bool *)(data));
      break;
    case SAT_RESP_TYPE_BYTE:
      var = g_variant_new("(y)", *(unsigned char *)(data));
      break;
    case SAT_RESP_TYPE_STRING:
      var = g_variant_new("(s)", (char *)(data));
      break;
    default:
      break;
    }

    g_dbus_method_invocation_return_value(agent->invocation, var);
    break;
  }

  case SAT_RESP_GO_BACK:
    g_dbus_method_invocation_return_dbus_error(agent->invocation,
        SAT_GOBACK_ERROR, "go back");
    break;
  case SAT_RESP_END_SESSION:
    g_dbus_method_invocation_return_dbus_error(agent->invocation,
        SAT_ENDSESSION_ERROR, "user end session");
    break;
  case SAT_RESP_SCREEN_BUSY:
    g_dbus_method_invocation_return_dbus_error(agent->invocation,
        SAT_BUSY_ERROR, "screen busy");
    break;
  }

  agent->invocation = NULL;
}
