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


#ifndef _POSITION_H_
#define _POSITION_H_

#include <Location.h>

G_BEGIN_DECLS

//location fields ID
typedef enum {
    TYPE_POSITION = 0,
    TYPE_VELOCITY = 1 << 0,
    TYPE_ACCURACY = 1 << 1,
} FiledType;

/*
 * Identify fields set by bitmask
 */
typedef enum {
    //position
    POSITION_FIELDS_NONE = 0,
    POSITION_FIELDS_LATITUDE = 1 << 0,
    POSITION_FIELDS_LONGITUDE = 1 << 1,
    POSITION_FIELDS_ALTITUDE = 1 << 2,
    //velocity
    VELOCITY_FIELDS_SPEED = 1 << 3,
    VELOCITY_FIELDS_DIRECTION = 1 << 4,
    VELOCITY_FIELDS_CLIMB = 1 << 5,
    //accuracy
} LocationFields;

typedef enum _GpsAccuracyLevel {
    ACCURACY_LEVEL_NONE = 0,
    ACCURACY_LEVEL_COUNTRY,
    ACCURACY_LEVEL_REGION,
    ACCURACY_LEVEL_LOCALITY,
    ACCURACY_LEVEL_POSTALCODE,
    ACCURACY_LEVEL_STREET,
    ACCURACY_LEVEL_DETAILED,
} GpsAccuracyLevel;

struct _Accuracy {
    GpsAccuracyLevel level;
    gdouble horizAccuracy;
    gdouble vertAccuracy;
};

typedef struct {
    gint prn;
    gdouble snr;
    gdouble elevation;
    gdouble azimuth;
    gboolean used;
    gboolean hasalmanac;
    gboolean hasephemeris;
} SatelliteInfo;

struct _Position {
    gint64 timestamp;
    gdouble latitude;
    gdouble longitude;
    gdouble altitude;
    gdouble speed;
    gdouble direction;
    gdouble climb;
};


struct _Satellite {
    guint num_satellite_used;
    guint visible_satellites_count;
    SatelliteInfo *sat_used;
};

struct _Address {
    gchar *freeformaddr;
    gchar *locality;
    gchar *region;
    gchar *country;
    gchar *countrycode;
    gchar *area;
    gchar *street;
    gchar *postcode;
    gboolean freeform;
};


Satellite *satellite_create(guint visible_satellites_count);
int set_satellite_details(Satellite *satellite,
                          gint index,
                          gdouble snr,
                          gint prn,
                          gdouble elevation,
                          gdouble azimuth,
                          gboolean used,
                          gboolean hasalmanac,
                          gboolean hasephemeris);
void satellite_free(Satellite *satellite);

G_END_DECLS

#endif  /* _POSITION_H_ */
