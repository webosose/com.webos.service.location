/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Position.h
 * Purpose      : Common location related variables & functions.
 * Platform     : RedRose
 * Author(s)    : Rajesh Gopu I.V
 * Email ID.    : rajeshgopu.iv@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

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

typedef struct {
    guint prn;
    gdouble snr;
    gdouble elevation;
    gdouble azimuth;
    gboolean used;
    gboolean hasalmanac;
    gboolean hasephemeris;
} SatelliteInfo;

struct _Position {
    guint64 timestamp;
    gdouble latitude;
    gdouble longitude;
    gdouble altitude;
    gdouble speed;
    gdouble direction;
    gdouble climb;
};

struct _Velocity {
    guint timestamp;
    gdouble speed;
    gdouble direction;
    gdouble climb;
};

struct _Nmea {
    guint timestamp;
    guint len;
    char *data;
};

struct _Satellite {
    guint num_satellite_used;
    guint visible_satellites_count;
    SatelliteInfo *sat_used;
};

Position *position_create(guint64 timestamp, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed,
                          gdouble direction, gdouble climb,
                          int flags);
void position_free(Position *position);

Velocity *velocity_create(guint timestamp, gdouble speed, gdouble direction, gdouble climp, int flags);
void velocity_free(Velocity *velocity);

Satellite *satellite_create(guint visible_satellites_count);
int set_satellite_details(Satellite *satellite, gint index, gdouble snr, guint prn, gdouble elevation, gdouble azimuth,
                          gboolean used,
                          gboolean hasalmanac, gboolean hasephemeris);
void satellite_free(Satellite *satellite);

G_END_DECLS

#endif  /* _POSITION_H_ */
