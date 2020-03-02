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


#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <glib-object.h>
#include <stdint.h>
#include <location_errors.h>

#define DEFAULT_VALUE 0
#define INVALID_PARAM -1.0
#define LOCATION_DB_PREF_PATH_GPS      "/var/location/location_gps.xml"
#define LOCATION_DB_PREF_PATH_NETWORK  "/var/location/location_network.xml"

typedef enum {
    HANDLER_NETWORK = 0,
    HANDLER_GPS,
    HANDLER_LBS,
    HANDLER_HYBRID,
    HANDLER_MAX,
} HandlerTypes;

typedef enum {
    HANDLER_DATA_NMEA = 0,
    HANDLER_DATA_SATELLITE,
    HANDLER_DATA_POSITION,
    HANLDER_GEOFENCE_REMOVE,
    HANLDER_GEOFENCE_PAUSE,
    HANLDER_GEOFENCE_RESUME
} HandlerDataTypes;


/*
 * Common status codes of GPS
 */
typedef enum {
    GPS_STATUS_ERROR = 0,
    GPS_STATUS_UNAVAILABLE,
    GPS_STATUS_ACQUIRING,
    GPS_STATUS_AVAILABLE
} GPSStatus;

typedef struct _Position Position;
typedef struct _Status Status;
typedef struct _Velocity Velocity;
typedef struct _Nmea Nmea;
typedef struct _Satellite Satellite;
typedef struct _Accuracy Accuracy;
typedef struct _Address Address;

struct _Location {
    double speed;
    double longitude;
    double latitude;
    double altitude;
    double horizontalAccuracy;
    double verticalAccuracy;
    double heading;
    int64_t timestamp;
    unsigned flag;
};

/* optional parameter bits */
#define LOCATION_SPEED_BIT               0x0001
#define LOCATION_ALTITUDE_BIT            0x0002
#define LOCATION_HORIZONTAL_ACCURACY_BIT 0x0004
#define LOCATION_VERTICAL_ACCURACY_BIT   0x0008
#define LOCATION_HEADING_BIT             0x0010
#define LOCATION_TIMESTAMP_BIT           0x0020

#endif /* _LOCATION_H_ */

/* vim:set et: */
