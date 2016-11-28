/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
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
#include "ofono-netmon.h"

static void _parse_cell_info(struct cell_info *info, GVariantIter *iter)
{
  char *key;
  GVariant *val;

  while(g_variant_iter_loop(iter, "{sv}", &key, &val)) {
    if (g_strcmp0(key, "registered") == 0) {
      info->registered = g_variant_get_boolean(val);
      tapi_debug("registered: %d", info->registered);
    } else if (g_strcmp0(key, "Technology") == 0) {
      const char *tech = g_variant_get_string(val, NULL);
      if (strcasecmp(tech, "umts") == 0)
        info->type = CELL_TYPE_3G;
      if (strcasecmp(tech, "lte") == 0)
        info->type = CELL_TYPE_4G;
      else
        info->type = CELL_TYPE_2G;
    } else if (g_strcmp0(key, "LocationAreaCode") == 0) {
      g_variant_get(val, "q", &info->lac);
      tapi_debug("lac: %X", info->lac);
    } else if (g_strcmp0(key, "CellId") == 0) {
      g_variant_get(val, "u", &info->cid);
      tapi_debug("cid: %X", info->cid);
    } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
      const char *mnc = g_variant_get_string(val, NULL);
      g_strlcpy(info->mnc, mnc, MAX_MNC_LEN + 1);
      tapi_debug("MNC: %s", mnc);
    } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
      const char *mcc = g_variant_get_string(val, NULL);
      g_strlcpy(info->mcc, mcc, MAX_MCC_LEN + 1);
      tapi_debug("MCC: %s", mcc);
    } else if (g_strcmp0(key, "ARFCN") == 0) {
      g_variant_get(val, "q", &info->arfcn);
      tapi_debug("arfcn: %X", info->arfcn);
    } else if (g_strcmp0(key, "BSIC") == 0) {
      g_variant_get(val, "y", &info->bsid);
      tapi_debug("bsid: %X", info->bsid);
    } else if (g_strcmp0(key, "BitErrorRate") == 0) {
      g_variant_get(val, "y", &info->ber);
      tapi_debug("ber: %X", info->ber);
    } else if (g_strcmp0(key, "PrimaryScramblingCode") == 0) {
      g_variant_get(val, "q", &info->psc);
      tapi_debug("psc: %X", info->psc);
    } else if (g_strcmp0(key, "TimingAdvance") == 0) {
      g_variant_get(val, "y", &info->ta);
      tapi_debug("TimingAdvance: %X", info->ta);
    } else if (g_strcmp0(key, "Strength") == 0) {
      g_variant_get(val, "y", &info->rssi);
      tapi_debug("rssi: %d", info->rssi);
    }
  }
}

static void _on_response_get_serving_cell_info(GObject *obj,
      GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  struct cell_info ci;
  GVariantIter *iter;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(a{sv))", &iter);
  _parse_cell_info(&ci, iter);

  CALL_RESP_CALLBACK(ret, &ci, cbd);

  g_variant_iter_free(iter);
  g_variant_unref(resp);
}

static void _on_response_get_cells_info(GObject *obj,
      GAsyncResult *result,
      gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;

  struct cells_info cs_info;
  int count;
  struct cell_info *p_ci;
  GVariantIter *iter, *iter_cells;
  int i;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);
  tapi_debug("%s", g_variant_print(resp, TRUE));

  g_variant_get(resp, "(a(a{sv}))", &iter);
  count = g_variant_iter_n_children(iter);
  if (count == 0) {
    CALL_RESP_CALLBACK(ret, NULL, cbd);
    g_variant_iter_free(iter);
    g_variant_unref(resp);
    return;
  }

  cs_info.cells = g_malloc0(sizeof(struct cell_info) * count);
  cs_info.count = count;
  p_ci = cs_info.cells;

  while (g_variant_iter_loop(iter, "(a{sv})", &iter_cells)) {
    _parse_cell_info(p_ci++, iter_cells);
	g_variant_iter_free(iter_cells);
  }

  CALL_RESP_CALLBACK(ret, &cs_info, cbd);
  g_variant_iter_free(iter);
  g_variant_unref(resp);
  g_free(cs_info.cells);
}

EXPORT_API tapi_bool ofono_netmon_get_serving_cell_info(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_NETMON_INTERFACE, "GetServingCellInformation", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_serving_cell_info, cbd);
}

EXPORT_API tapi_bool ofono_netmon_get_cells_info(struct ofono_modem *modem,
      response_cb cb,
      void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_NETMON_INTERFACE, "GetCellsInformation", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_get_cells_info, cbd);
}
