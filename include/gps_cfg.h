// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#ifndef GPS_CFG_H
#define GPS_CFG_H

#include <stdint.h>

#define GPS_MAX_PARAM_NAME                 48
#define GPS_MAX_PARAM_STRING               80
#define GPS_MAX_PARAM_LINE                 80

#define GPS_READ_CONF_DEFAULT(filename) \
    gps_read_conf((filename), NULL, 0);

#define GPS_READ_CONF(filename, config_table) \
    gps_read_conf((filename), (config_table), sizeof(config_table) / sizeof(config_table[0]))

/*=============================================================================
 *
 *                        MODULE TYPE DECLARATION
 *
 *============================================================================*/
typedef struct _gps_param_s_type gps_param_s_type ;
struct _gps_param_s_type{
    char        param_name[GPS_MAX_PARAM_NAME];
    void        *param_ptr;
    uint8_t     *param_set;     /* was this value set by config file? */
    char        param_type;     /* 'n' for number,
                                   's' for string,
                                   'f' for float */
};

/*=============================================================================
 *
 *                          MODULE EXTERNAL DATA
 *
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *
 *                       MODULE EXPORTED FUNCTIONS
 *
 *============================================================================*/
extern void gps_read_conf(const char* conf_file_name,
                          gps_param_s_type* config_table,
                          uint32_t table_length);

#ifdef __cplusplus
}
#endif

#endif /* GPS_CFG_H */
