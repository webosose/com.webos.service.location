/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework Geoclue Service
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : gps_cfg.h
 * Purpose      : Define structure and enums configuration Reading
 * Platform     : RedRose
 * Author(s)    : Mallikarjuna Reddy
 * Email ID.    : mallikarjuna.reddy@lge.com
 * Creation Date: 10-10-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/
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
