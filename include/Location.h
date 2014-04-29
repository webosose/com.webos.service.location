/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Location.h
 * Purpose      : Common location variables & functions
 * Platform     : RedRose
 * Author(s)    : rajesh gopu, Abhishek Srivastava
 * Email ID.    : rajeshgopu.iv@lge.com, abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <glib-object.h>
#include <stdint.h>

#define DEFAULT_VALUE 0
#define INVALID_PARAM -1.0

typedef enum {
    HANDLER_WIFI = 0,
    HANDLER_CELLID,
    HANDLER_GOOGLE_WIFI_CELL,
    HANDLER_NW,
    HANDLER_GPS,
    HANDLER_LBS,
    HANDLER_HYBRID,
    HANDLER_MAX,
} HandlerTypes;

typedef enum {
    HANDLER_DATA_NMEA = 0,
    HANDLER_DATA_SATELLITE,
    HANDLER_DATA_POSITION
} HandlerDataTypes;

/*
 * Common Error codes of Location Framework
 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_NOT_IMPLEMENTED,
    ERROR_TIMEOUT,
    ERROR_NETWORK_ERROR,
    ERROR_NOT_APPLICABLE_TO_THIS_HANDLER,
    ERROR_NOT_AVAILABLE,
    ERROR_WRONG_PARAMETER,
    ERROR_DUPLICATE_REQUEST,
    ERROR_REQUEST_INPROGRESS,
    ERROR_NOT_STARTED
} ErrorCodes;

/*
 * Common status codes of GPS
 */
typedef enum {
    GPS_STATUS_ERROR = 0,
    GPS_STATUS_UNAVAILABLE,
    GPS_STATUS_ACQUIRING,
    GPS_STATUS_AVAILABLE
} GPSStatus;

enum {
    ERROR_NETWORK_NOT_FOUND = 0,
    ERROR_NO_MEMORY,
    ERROR_NO_RESPONSE,
    ERROR_NO_DATA
};

typedef GObject HandlerObject;
typedef struct _Position Position;
typedef struct _Status Status;
typedef struct _Velocity Velocity;
typedef struct _Nmea Nmea;
typedef struct _Satellite Satellite;
typedef struct _Accuracy Accuracy;
typedef struct _Address Address;

#define LOCATION_DB_PREF_PATH   "/usr/share/location/location_position.xml"
#define LOCATION_DB_PREF_PATH_WIFI "/usr/share/location/location_wifi.xml"
#define LOCATION_DB_PREF_PATH_CELL "/usr/share/location/location_cell.xml"

#endif /* _LOCATION_H_ */
