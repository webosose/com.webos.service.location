/*
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Position.c
 *
 * Created on: Feb 22, 2013
 * Author: rajeshgopu.iv
 */

#include <stdlib.h>
#include <string.h>
#include <Position.h>

Position *position_create(guint64 timestamp, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed,
                          gdouble direction, gdouble climb,
                          int flags)
{
    if (latitude < -90 || latitude > 90) return NULL;

    if (longitude < -180 || longitude > 180) return NULL;

    Position *position = g_slice_new0(Position);
    g_return_val_if_fail(position, NULL);
    position->timestamp = timestamp * 1000;

    if (flags & POSITION_FIELDS_NONE) return position;

    if (flags & POSITION_FIELDS_LATITUDE) position->latitude = latitude;

    if (flags & POSITION_FIELDS_LONGITUDE) position->longitude = longitude;

    if (flags & POSITION_FIELDS_ALTITUDE) position->altitude = altitude;

    if (flags & VELOCITY_FIELDS_SPEED) position->speed = speed;

    if (flags & VELOCITY_FIELDS_DIRECTION) position->direction = direction;

    if (flags & VELOCITY_FIELDS_CLIMB) position->climb = climb;

    return position;
}

void position_free(Position *position)
{
    g_return_if_fail(position);
    g_slice_free(Position, position);
}

