/*
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Velocity.c
 *
 * Created on: Feb 22, 2013
 * Author: rajeshgopu.iv
 */

#include <stdlib.h>
#include <string.h>
#include <Position.h>


Velocity *velocity_create(guint timestamp, gdouble speed, gdouble direction, gdouble climb, int flags)
{
    Velocity* velocity = g_slice_new0(Velocity);
    velocity->timestamp = timestamp;
    if (flags & POSITION_FIELDS_NONE)
        return velocity;

    if (flags & VELOCITY_FIELDS_SPEED)
        velocity->speed = speed;
    if (flags & VELOCITY_FIELDS_DIRECTION)
        velocity->direction = direction;
    if (flags & VELOCITY_FIELDS_CLIMB)
        velocity->climb = climb;

    return velocity;
}

void velocity_free(Velocity *velocity)
{
    g_return_if_fail(velocity);
    g_slice_free(Velocity, velocity);
}

