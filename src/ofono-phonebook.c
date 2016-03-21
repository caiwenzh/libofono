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

#include "common.h"
#include "log.h"
#include "ofono-phonebook.h"

static void _on_response_import(GObject *obj,
        GAsyncResult *result,
        gpointer user_data)
{
  TResult ret;
  GVariant *resp;
  GError *error = NULL;
  struct response_cb_data *cbd = user_data;
  char *vcards = NULL;

  resp = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), result, &error);

  CHECK_RESULT(ret, error, cbd, resp);

  g_variant_get(resp, "(s)", &vcards);

  tapi_debug("vcards: %s", vcards);

  CALL_RESP_CALLBACK(ret, &vcards, cbd);
  g_variant_unref(resp);
  g_free(vcards);
}

EXPORT_API void ofono_phonebook_import(struct ofono_modem *modem,
        response_cb cb,
        void *user_data)
{
  struct response_cb_data *cbd;

  tapi_debug("");

  CHECK_PARAMETERS(modem, cb, user_data);
  NEW_RSP_CB_DATA(cbd, cb, user_data);

  g_dbus_connection_call(modem->conn, OFONO_SERVICE, modem->path,
      OFONO_PHONEBOOK_IFACE, "Import", NULL,
      NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
      _on_response_import, cbd);
}

