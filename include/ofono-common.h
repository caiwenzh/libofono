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

#ifndef __OFONO_COMMON__H
#define __OFONO_COMMON__H

#ifdef  __cplusplus
extern "C" {
#endif

typedef int tapi_bool;

#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif


struct str_list {
  char **data;
  int count;
};

enum ofono_noti {
  /* modem */
  OFONO_NOTI_MODEM_REMOVED, /* Modem removed: object path (char*) */
  OFONO_NOTI_MODEM_STATUS_CHAANGED, /* Modem status changed: enum modem_status */
  OFONO_NOTI_INTERFACES_CHANGED, /* Modem interface changed: Interfaces
        bit map (guint32*) */

  /* network */
  OFONO_NOTI_SIGNAL_STRENTH_CHANGED, /* Signal strength changed: Signal
        strength (unsigned char*), range:0 - 100 */
  OFONO_NOTI_REGISTRATION_STATUS_CHANGED, /* Registration status changed:
        Registration info (struct registration_info*) */

  /* Call */
  OFONO_NOTI_CALL_ADDED, /* A new call is added: New added call
        information (struct ofono_call_info*) */
  OFONO_NOTI_CALL_REMOVED, /* A call is removed: The id of the removed
        call: (unsigned int *) */
  OFONO_NOTI_CALL_STATUS_CHANGED, /* Call status changed: Call
        information (struct ofono_call_info*) */
  OFONO_NOTI_CALL_DISCONNECT_REASON, /* Call disconnect reason: reason
        (enum ofono_call_disconnect_reason) */

  /* SMS */
  OFONO_NOTI_INCOMING_SMS, /* A normal SMS is received:
        (struct ofono_sms_incoming_noti*) */
  OFONO_NOTI_INCOMING_SMS_CLASS_0, /* A class 0 SMS is received:
        (struct ofono_sms_incoming_noti*) */
  OFONO_NOTI_MSG_STATUS_CHANGED, /* A MO SMS status is changed:
        (struct ofono_sms_sent_staus_noti*) */
  OFONO_NOTI_SMS_DELIVERY_REPORT, /* A delivery report is received:
        (struct ofono_sms_status_report_noti*) */
  OFONO_NOTI_INCOMING_CBS, /* A normal cbs is received:
        (struct ofono_cbs_incoming_noti*) */
  OFONO_NOTI_EMERGENCY_CBS, /* An emergency cbs is received:
        (struct ofono_cbs_emergency_noti*) */

  /* SIM */
  OFONO_NOTI_SIM_STATUS_CHANGED, /* SIM status is changed:
        new sim status (enum sim_status*) */

  /* USSD */
  OFONO_NOTI_USSD_NOTIFICATION, /* USSD info notification: USSD
        information (struct ussd_request_noti*) */
  OFONO_NOTI_USSD_REQ, /* USSD request notification:
        (struct ussd_request_noti*) */
  OFONO_NOTI_USSD_STATUS_CHANGED, /* USSD status changed:
        new ussd status (enum ussd_status*) */

  /* Connman(data connection) */
  OFONO_NOTI_CONNMAN_ATTACHED, /* Packet domain attach status changed:
        attach status (tapi_bool*), TURE: attached */
  OFONO_NOTI_CONNMAN_CONTEXT_ACTIVED, /*  PDP context activation information:
        (struct context_actived_noti*) */

  /* SAT */
  OFONO_NOTI_SAT_IDLE_MODE_TEXT, /* display idle text notification:
        idle text (char *) */
  OFONO_NOTI_SAT_MAIN_MENU, /* Main menu is changed: NULL */
};

enum ofono_api {
  OFONO_API_SIM,
  OFONO_API_NETREG, /* Network Registration */
  OFONO_API_VOICE, /* Voice Call Manager */
  OFONO_API_MSG, /* SMS */
  OFONO_API_MSG_WAITING,
  OFONO_API_SMART_MSG, /* Smart SMS: v-card, v-calendar */
  OFONO_API_STK,
  OFONO_API_CALL_FW,  /* Call Forwarding */
  OFONO_API_CALL_VOL, /* Call Volume */
  OFONO_API_CALL_METER, /* Call Meter */
  OFONO_API_CALL_SET,   /* Call Setting (CLI relative) */
  OFONO_API_CALL_BAR,   /* Call Barring */
  OFONO_API_SUPPL_SERV, /* Supplementary Service */
  OFONO_API_TXT_TEL,    /* Text Telephony */
  OFONO_API_CELL_BROAD, /* Cell Broadcast */
  OFONO_API_CONNMAN,    /* Connection Manager(data connection) */
  OFONO_API_PUSH_NOTIF, /* Push Notification */
  OFONO_API_PHONEBOOK,
  OFONO_API_ASN,        /* Assisted Satellite Navigation  */
  OFONO_API_RADIO_SETTING, /* Radio Setting */
};

typedef enum tapi_result {
  TAPI_RESULT_OK,
  TAPI_RESULT_UNKNOWN_ERROR, /* Unkown (unexpect) error occurs */
  TAPI_RESULT_INVALID_ARGS, /* Arguments are invalid */
  TAPI_RESULT_NOT_SUPPORTED,
  TAPI_RESULT_NO_MEMORY,
  TAPI_RESULT_IN_PROGRESS, /* Busy, request alreay in progress */
  TAPI_RESULT_INTERFACE_NOT_FOUND,
  TAPI_RESULT_TIMEOUT, /* Operation timeout */
  TAPI_RESULT_SIM_NOT_READY, /* SIM isn't ready */
  TAPI_RESULT_PWD_INCORRECT, /* Password is incorrect */
  TAPI_RESULT_NOT_REGISTERED, /* It isn't registered to network */
  TAPI_RESULT_SIM_LOCKED, /* Sim is locked, password required */
  TAPI_RESULT_NETWORK_ERROR, /* Network error */
  TAPI_RESULT_FAIL, /* Fail, reason is unkown */
} TResult;

struct ofono_modem;

typedef void (*destroy_notify)(void *user_data);
typedef void (*modems_changed_cb)(const char *modem, tapi_bool add);
typedef void (*noti_cb) (enum ofono_noti noti, void *data, void *user_data);
typedef void (*response_cb) (TResult result, const void *resp_data, const void *user_data);

/**
 * Create dbus connection to ofono daemon and start to monitor modem changes
 *
 */
tapi_bool ofono_init();

/**
 * Disconnect dbus connection to ofono daemon and release resources
 *
 */
void ofono_deinit();

/**
 * Get modems
 *
 * Return list of modem object path (data: struct str_list, should free it by
 *    ofono_string_list_free).
 *
 */
struct str_list* ofono_get_modems();

/**
 * Set the callback be called when a modem is added or removed
 *
 * If call ofono_get_modems() before ofono deamon is started no modem can be
 * found. This API provide user a way to monitor the presence of modems.
 *
 * 'cb': the callback be called when a modem is added or removed
 */
void ofono_set_modems_changed_callback(modems_changed_cb cb);

/**
 * init a modem
 *
 * "modem": modem object path, if it isn't specified (NULL), the first element
 *    in the modem list will be selected. If no modem with the path 'modem'
 *    exist, return NULL.
 */
struct ofono_modem* ofono_modem_init(const char *modem);

/**
 * finalize a modem
 */
void ofono_modem_deinit(struct ofono_modem *modem);

/**
 * Register notification callback
 *
 * 'noti': the notification to handle
 * 'cb': the callback will be called when 'noti' received
 * 'user_data': the user data will be pass through to the callback
 * 'user_data_free_func': the function be called to release the memory of
 *         'user_data'.
 */
tapi_bool ofono_register_notification_callback(struct ofono_modem *modem,
                enum ofono_noti noti,
                noti_cb cb,
                void *user_data,
                destroy_notify user_data_free_func);

/**
 * Unregister notification callback
 */
void ofono_unregister_notification_callback(struct ofono_modem *modem,
                enum ofono_noti noti,
                noti_cb cb);

/**
 * Get modem power status
 *
 * 'powered': the power status will be saved in it if success
 *
 * Return TURE of call success, otherwise return FALSE
 */
tapi_bool ofono_modem_get_power_status(struct ofono_modem *modem,
                tapi_bool *powered);

/**
 * Check whether an interface exists
 */
tapi_bool ofono_has_interface(struct ofono_modem *modem, enum ofono_api api);

void ofono_string_list_free(struct str_list *list);

#ifdef  __cplusplus
}
#endif

#endif
