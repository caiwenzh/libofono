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
#ifndef __OFONO_NETMON_H_
#define __OFONO_NETMON_H_

#ifdef  __cplusplus
extern "C" {
#endif

enum cell_type {
  CELL_TYPE_2G,
  CELL_TYPE_3G,
  CELL_TYPE_4G,
};

struct cell_info {
  tapi_bool registered;
  enum cell_type type;
  unsigned short lac;
  unsigned int cid;
  char mcc[MAX_MCC_LEN + 1];
  char mnc[MAX_MNC_LEN + 1];
  unsigned char bsid; /* Base Station Identity Code, 0 - 63 */
  unsigned char rssi; /* signal strength in dBm, 0 - 31 */
  unsigned char ber;  /* Bit Error Rate, 0 - 7 */
  unsigned char ta;   /* Timing Advance, 0 - 219 */
  union {
    unsigned short arfcn; /* Absolute Radio Frequency Channel Number, 0 - 1023 */
    unsigned short psc; /* Primary Scrambling Code, 0 - 512 */
  };
};

struct cells_info {
  int count; /* cell count */
  struct cell_info *cells; /* information of cells */
};


/**
 * get the information of current serving cell
 *
 * Async response data: struct cell_info*, (information of serving cell)
 */
tapi_bool ofono_netmon_get_serving_cell_info(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);


/**
 * get the information of current serving cell
 *
 * Async response data: struct cells_info* (information of cells)
 */
tapi_bool ofono_netmon_get_cells_info(struct ofono_modem *modem,
      response_cb cb,
      void *user_data);


#ifdef  __cplusplus
}
#endif

#endif
