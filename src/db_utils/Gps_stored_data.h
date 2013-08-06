/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework Geoclue Service
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Gps_stored_data.h
 * Purpose  : Stroing of GPS data in DB
 * Platform  : RedRose
 * Author(s) : Abhishek Srivastava
 * Email ID. : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/

#ifndef _GPS_STORED_DATA_H_
#define _GPS_STORED_DATA_H_

#include <nyx/nyx_client.h>
#include <nyx/module/nyx_device_internal.h>
#include <Location.h>

#define ERROR_NOT_AVAILABLE     4
#define ERROR_NONE              0
#define LOCATION_DB_PREF_PATH   "usr/share/location/location_position.xml"
#define LOCATION_DB_PREF_PATH_CELL "usr/share/location/location_wifi.xml"
void gps_service_get_stored_position(position_data *last_pos);
void gps_service_store_position(position_data *last_pos);
void wifi_plugin_set_stored_position(guint64 timestamp, gdouble longitude,
                                     gdouble latitude, gdouble hor_accuracy,
                                     gdouble ver_accuracy);
int cell_get_stored_position(Position *position, Accuracy *accuracy);
void cell_set_stored_position(guint64 timestamp, gdouble longitude,
                              gdouble latitude, gdouble hor_accuracy,
                              gdouble ver_accuracy);
void initializeLocationDB();

#endif

