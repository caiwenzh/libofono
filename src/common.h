/*
 *  Copyright (C) 2013 Intel Corporation. All rights reserved.
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
#ifndef __COMMON__H__
#define __COMMON__H__

#include "ofono-common.h"
#include "ofono-call.h"
#include "ofono-sim.h"
#include "ofono-network.h"

#include <glib.h>
#include <gio/gio.h>
#include <malloc.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define OFONO_SERVICE "org.ofono"
#define OFONO_SERVER_SERVICE "org.ofono.server"
#define OFONO_PREFIX_ERROR OFONO_SERVICE ".Error."
#define OFONO_MANAGER_PATH "/"

#define OFONO_MANAGER_IFACE "org.ofono.Manager"
#define OFONO_MODEM_IFACE "org.ofono.Modem"
#define OFONO_CALL_BARRING_IFACE "org.ofono.CallBarring"
#define OFONO_CALL_FORWARDING_IFACE "org.ofono.CallForwarding"
#define OFONO_CALL_METER_IFACE "org.ofono.CallMeter"
#define OFONO_CALL_SETTINGS_IFACE "org.ofono.CallSettings"
#define OFONO_CALL_VOLUME_IFACE "org.ofono.CallVolume"
#define OFONO_CELL_BROADCAST_IFACE "org.ofono.CellBroadcast"
#define OFONO_CONTEXT_IFACE "org.ofono.ConnectionContext"
#define OFONO_CONNMAN_IFACE "org.ofono.ConnectionManager"
#define OFONO_MESSAGE_MANAGER_IFACE "org.ofono.MessageManager"
#define OFONO_MESSAGE_IFACE "org.ofono.Message"
#define OFONO_SMART_MESSAGE_IFACE "org.ofono.SmartMessaging"
#define OFONO_MESSAGE_WAITING_IFACE "org.ofono.MessageWaiting"
#define OFONO_SUPPLEMENTARY_SERVICES_IFACE "org.ofono.SupplementaryServices"
#define OFONO_NETWORK_REGISTRATION_IFACE "org.ofono.NetworkRegistration"
#define OFONO_NETWORK_OPERATOR_IFACE "org.ofono.NetworkOperator"
#define OFONO_PHONEBOOK_IFACE "org.ofono.Phonebook"
#define OFONO_RADIO_SETTINGS_IFACE "org.ofono.RadioSettings"
#define OFONO_AUDIO_SETTINGS_IFACE "org.ofono.AudioSettings"
#define OFONO_TEXT_TELEPHONY_IFACE "org.ofono.TextTelephony"
#define OFONO_SIM_MANAGER_IFACE "org.ofono.SimManager"
#define OFONO_VOICECALL_IFACE "org.ofono.VoiceCall"
#define OFONO_VOICECALL_MANAGER_IFACE "org.ofono.VoiceCallManager"
#define OFONO_STK_IFACE "org.ofono.SimToolkit"
#define OFONO_SIM_APP_IFACE "org.ofono.SimToolkitAgent"
#define OFONO_LOCATION_REPORTING_IFACE "org.ofono..LocationReporting"
#define OFONO_GNSS_IFACE "org.ofono.AssistedSatelliteNavigation"
#define OFONO_GNSS_POSR_AGENT_IFACE "org.ofono.PositioningRequestAgent"
#define OFONO_HANDSFREE_IFACE  "org.ofono.Handsfree"
#define OFONO_CDMA_VOICECALL_MANAGER_IFACE "org.ofono.cdma.VoiceCallManager"
#define OFONO_CDMA_MESSAGE_MANAGER_IFACE "org.ofono.cdma.MessageManager"
#define OFONO_CDMA_CONNECTION_MANAGER_IFACE "org.ofono.cdma.ConnectionManager"
#define OFONO_PUSH_NOTIFICATION_IFACE "org.ofono.PushNotification"
#define OFONO_CDMA_NETWORK_REGISTRATION_IFACE \
  "org.ofono.cdma.NetworkRegistration"

struct ofono_modem {
  GDBusConnection *conn;
  gchar *path; /* modem object path */

  guint32 interfaces;
  tapi_bool powered;
  tapi_bool online;

  guint prop_changed_watch;

  GList *noti_list; /* notification handle data (struct ofono_noti_data) list */
};

struct response_cb_data {
  response_cb cb;
  void *user_data;
};

struct interm_response_cb_data {
  struct response_cb_data *cbd; /* final response callback data */
  void *user_data; /* intermediate user data */
  struct ofono_modem *modem;
};

TResult ofono_error_parse(GError *err);

#define CHECK_PARAMETERS(cond, cb, user_data) \
  if(!(cond)) { \
    tapi_error("invalid parameter"); \
    if(cb) \
      cb(TAPI_RESULT_INVALID_ARGS, NULL, user_data); \
    return; \
  } \

#define NEW_RSP_CB_DATA(_cbd, _cb, _user_data) \
  _cbd = g_new0(struct response_cb_data, 1); \
  _cbd->cb = _cb; \
  _cbd->user_data = _user_data; \

#define NEW_INTERM_RSP_CB_DATA(_icbd, _cbd, _modem, _user_data) \
  _icbd = g_new0(struct interm_response_cb_data, 1); \
  _icbd->cbd = _cbd; \
  _icbd->modem = _modem; \
  _icbd->user_data = _user_data; \


#define CALL_RESP_CALLBACK(_ret, _resp_data, _cbd) \
  if (_cbd->cb) \
    _cbd->cb(_ret, _resp_data, _cbd->user_data); \
  g_free(_cbd);


#define CHECK_RESULT(_ret, _error, _cbd, _resp) \
  do { \
    _ret = ofono_error_parse(_error); \
    if (_ret != TAPI_RESULT_OK) { \
      if (_cbd->cb) \
        _cbd->cb(_ret, NULL, _cbd->user_data); \
      g_free(_cbd); \
      g_error_free(_error); \
      if (_resp != NULL) \
        g_variant_unref(_resp); \
      return; \
    } \
  } while (0)

tapi_bool has_interface(guint32 interfaces, enum ofono_api api);

void on_response_common(GObject *source_object,
                GAsyncResult *result,
                gpointer user_data);

void ofono_set_property(struct ofono_modem *modem, const char *iface,
                char *path, const char *key, GVariant *value,
                response_cb cb, void *user_data);

unsigned int ofono_get_call_id_from_obj_path(char *obj_path);
enum ofono_call_status ofono_str_to_call_status(const char *str);
enum access_tech ofono_str_to_tech(const char *tech);

#ifdef  __cplusplus
}
#endif

#endif
