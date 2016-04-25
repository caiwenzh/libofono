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
#include "ofono-sim.h"

static const char *const pinlock_name[] = {
  [PIN_LOCK_NONE] = "none",
  [PIN_LOCK_SIM_PIN] = "pin",
  [PIN_LOCK_SIM_PUK] = "puk",
  [PIN_LOCK_PHSIM_PIN] = "phone",
  [PIN_LOCK_PHFSIM_PIN] = "firstphone",
  [PIN_LOCK_PHFSIM_PUK] = "firstphonepuk",
  [PIN_LOCK_SIM_PIN2] = "pin2",
  [PIN_LOCK_SIM_PUK2] = "puk2",
  [PIN_LOCK_PHNET_PIN] = "network",
  [PIN_LOCK_PHNET_PUK] = "networkpuk",
  [PIN_LOCK_PHNETSUB_PIN] = "netsub",
  [PIN_LOCK_PHNETSUB_PUK] = "netsubpuk",
  [PIN_LOCK_PHSP_PIN] = "service",
  [PIN_LOCK_PHSP_PUK] = "servicepuk",
  [PIN_LOCK_PHCORP_PIN] = "corp",
  [PIN_LOCK_PHCORP_PUK] = "corppuk",
};

static const char *_pin_lock_type_to_str(enum pin_lock_type type)
{
  return pinlock_name[type];
}

static enum pin_lock_type _str_to_pin_lock_type(char *type)
{
  if (type == NULL) {
    tapi_error("");
    return PIN_LOCK_NONE;
  }

  if (g_strcmp0(type, "none") == 0)
    return PIN_LOCK_NONE;
  else if (g_strcmp0(type, "pin") == 0)
    return PIN_LOCK_SIM_PIN;
  else if (g_strcmp0(type, "puk") == 0)
    return PIN_LOCK_SIM_PUK;
  else if (g_strcmp0(type, "phone") == 0)
    return PIN_LOCK_PHSIM_PIN;
  else if (g_strcmp0(type, "firstphone") == 0)
    return PIN_LOCK_PHFSIM_PIN;
  else if (g_strcmp0(type, "firstphonepuk") == 0)
    return PIN_LOCK_PHFSIM_PUK;
  else if (g_strcmp0(type, "pin2") == 0)
    return PIN_LOCK_SIM_PIN2;
  else if (g_strcmp0(type, "puk2") == 0)
    return PIN_LOCK_SIM_PUK2;
  else if (g_strcmp0(type, "network") == 0)
    return PIN_LOCK_PHNET_PIN;
  else if (g_strcmp0(type, "networkpuk") == 0)
    return PIN_LOCK_PHNET_PUK;
  else if (g_strcmp0(type, "netsub") == 0)
    return PIN_LOCK_PHNETSUB_PIN;
  else if (g_strcmp0(type, "netsubpuk") == 0)
    return PIN_LOCK_PHNETSUB_PUK;
  else if (g_strcmp0(type, "service") == 0)
    return PIN_LOCK_PHSP_PIN;
  else if (g_strcmp0(type, "servicepuk") == 0)
    return PIN_LOCK_PHSP_PUK;
  else if (g_strcmp0(type, "corp") == 0)
    return PIN_LOCK_PHCORP_PIN;
  else if (g_strcmp0(type, "corppuk") == 0)
    return PIN_LOCK_PHCORP_PUK;
  else {
    tapi_error("Unknown PIN Lock type");
    return PIN_LOCK_NONE;
  }
}

static tapi_bool _check_pin(char *pin)
{
  int len;
  if (pin == NULL)
    return FALSE;

  len = strlen(pin);
  if (len < 4 || len > 8)
    return FALSE;

  return TRUE;
}

EXPORT_API void ofono_sim_enable_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && _check_pin(pin), cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Type: %d, PIN: %s", type, pin);

  var = g_variant_new("(ss)", _pin_lock_type_to_str(type), pin);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SIM_MANAGER_IFACE, "LockPin", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_sim_disable_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && _check_pin(pin), cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Type: %d, PIN: %s", type, pin);

  var = g_variant_new("(ss)", _pin_lock_type_to_str(type), pin);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SIM_MANAGER_IFACE, "UnlockPin", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_sim_enter_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *pin,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && _check_pin(pin), cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Type: %d, PIN: %s", type, pin);

  var = g_variant_new("(ss)", _pin_lock_type_to_str(type), pin);
  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SIM_MANAGER_IFACE, "EnterPin", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_sim_reset_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *puk,
      char *new_pin,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && _check_pin(puk) && _check_pin(new_pin), cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Type: %d, Old: %s, New: %s", type, puk, new_pin);

  var = g_variant_new("(sss)", _pin_lock_type_to_str(type), puk, new_pin);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SIM_MANAGER_IFACE, "ResetPin", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API void ofono_sim_change_pin(struct ofono_modem *modem,
      enum pin_lock_type type,
      char *old_pin,
      char *new_pin,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;
  GVariant *var;

  CHECK_PARAMETERS(modem && _check_pin(old_pin) && _check_pin(new_pin),cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  tapi_debug("Type: %d, Old: %s, New: %s", type, old_pin, new_pin);

  var = g_variant_new("(sss)", _pin_lock_type_to_str(type), old_pin, new_pin);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_SIM_MANAGER_IFACE, "ChangePin", var,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      on_response_common, cbd);
}

EXPORT_API tapi_bool ofono_sim_get_info(struct ofono_modem *modem,
      struct sim_info *info)
{
  GError *error = NULL;
  GVariant *var_properties;
  GVariantIter *iter;
  char *key;
  GVariant *var_val;
  const char *val;

  tapi_debug("");

  if (modem == NULL || info == NULL) {
    tapi_error("Invalid parameter");
    return FALSE;
  }

  memset(info, 0, sizeof(struct sim_info));
  if (!has_interface(modem->interfaces, OFONO_API_SIM)) {
    info->status = SIM_STATUS_ABSENT;
    return TRUE;
  }

  var_properties = g_dbus_connection_call_sync(modem->conn,
      OFONO_SERVICE, modem->path, OFONO_SIM_MANAGER_IFACE,
      "GetProperties", NULL, NULL,
      G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

  if (var_properties == NULL) {
    tapi_error("dbus call failed (%s)", error->message);
    g_error_free(error);
    return FALSE;
  }

  g_variant_get(var_properties, "(a{sv})", &iter);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Present") == 0) {
      if (g_variant_get_boolean(var_val) == TRUE)
        info->status = SIM_STATUS_INITIALIZING;
      else
        info->status = SIM_STATUS_ABSENT;
    } else if (g_strcmp0(key, "SubscriberIdentity") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->imsi, val, sizeof(info->imsi));
    } else if (g_strcmp0(key, "CardIdentifier") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->iccid, val, sizeof(info->iccid));
    } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->mcc, val, sizeof(info->mcc));
    } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
      val = g_variant_get_string(var_val, NULL);
      g_strlcpy(info->mnc, val, sizeof(info->mnc));
    } else if (g_strcmp0(key, "SubscriberNumbers") == 0) {
      GVariantIter *msisdn_iter;
      char *num = NULL;
      g_variant_get(var_val, "as", &msisdn_iter);
      g_variant_iter_next(msisdn_iter, "s", &num);

      if (num != NULL)
        g_strlcpy(info->msisdn[0], num, sizeof(info->msisdn[0]));
      g_free(num);
      if (g_variant_iter_next(msisdn_iter, "s", &num)) {
        if (num != NULL)
          g_strlcpy(info->msisdn[1], num, sizeof(info->msisdn[1]));
        g_free(num);
      }
      g_variant_iter_free(msisdn_iter);
    } else if (g_strcmp0(key, "Retries") == 0) {
      GVariantIter *retry_iter;
      char *lock_type;
      unsigned char retry;
      enum pin_lock_type plt;

      g_variant_get(var_val, "a{sy}", &retry_iter);
      while (g_variant_iter_next(retry_iter, "{sy}", &lock_type, &retry)) {
        plt = _str_to_pin_lock_type(lock_type);
        info->retries[plt] = retry;

        g_free(lock_type);
      }

      g_variant_iter_free(retry_iter);
    } else if (g_strcmp0(key, "PinRequired") == 0) {
      const char *lock = g_variant_get_string(var_val, NULL);
      info->pin_required = _str_to_pin_lock_type((char *)lock);
    }  else if (g_strcmp0(key, "LockedPins") == 0) {
      GVariantIter *pin_iter;
      char *lock_pin;
      enum pin_lock_type type;

      g_variant_get(var_val, "as", &pin_iter);
      while (g_variant_iter_next(pin_iter, "s", &lock_pin)) {
        type = _str_to_pin_lock_type(lock_pin);
        info->pins[type] = TRUE;
        g_free(lock_pin);
      }

      g_variant_iter_free(pin_iter);
    }

    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);

  tapi_debug("status: %d, pin_required: %d, IMSI: %s, ICCID: %s, MCC: %s, "\
      "MNC: %s, msisdn: %s/%s, pin_lock(%d), retries: %d-%d-%d-%d",
      info->status, info->pin_required, info->imsi, info->iccid,
      info->mcc, info->mnc, info->msisdn[0], info->msisdn[1], info->pins[PIN_LOCK_SIM_PIN],
      info->retries[PIN_LOCK_SIM_PIN], info->retries[PIN_LOCK_SIM_PUK],
      info->retries[PIN_LOCK_SIM_PIN2], info->retries[PIN_LOCK_SIM_PUK2]);

  if(info->status == SIM_STATUS_ABSENT)
    return TRUE;

  info->status = SIM_STATUS_INITIALIZING;
  if (info->pin_required != PIN_LOCK_NONE)
    info->status = SIM_STATUS_LOCKED;
  else if (info->retries[PIN_LOCK_SIM_PIN] != 0 || info->retries[PIN_LOCK_SIM_PUK] != 0)
    info->status = SIM_STATUS_READY;

  return TRUE;
}
