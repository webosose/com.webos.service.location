/*
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Satellite.c
 *
 * Created on: Feb 22, 2013
 * Author: rajeshgopu.iv
 */

#include <stdlib.h>
#include <string.h>
#include <Position.h>

Satellite *satellite_create(guint visible_satellites_count)
{
    Satellite *satellite = g_slice_new0(Satellite);
    g_return_val_if_fail(satellite, NULL);
    satellite->visible_satellites_count = visible_satellites_count;
    satellite->sat_used = g_new0(SatelliteInfo, satellite->visible_satellites_count);

    return satellite;
}

static void update_satellite_used_in_fix_count(Satellite *satellite)
{
    g_return_if_fail(satellite);
    satellite->num_satellite_used = 0;
    int index = 0;

    if (satellite->visible_satellites_count > 0) {
        for (index = 0; index < satellite->visible_satellites_count; index++)
            if (satellite->sat_used[index].used) (satellite->num_satellite_used)++;
    }
}

int set_satellite_details(Satellite *satellite, gint index, gdouble snr, guint prn, gdouble elevation, gdouble azimuth, gboolean used,
                          gboolean hasalmanac, gboolean hasephemeris)
{
    g_return_val_if_fail(satellite, FALSE);
    g_return_val_if_fail(satellite->sat_used, FALSE);

    satellite->sat_used[index].prn = prn;
    satellite->sat_used[index].snr = snr;
    satellite->sat_used[index].elevation = elevation;
    satellite->sat_used[index].azimuth = azimuth;
    satellite->sat_used[index].used = used;
    satellite->sat_used[index].hasalmanac = hasalmanac;
    satellite->sat_used[index].hasephemeris = hasephemeris;
    update_satellite_used_in_fix_count(satellite);

    return TRUE;
}

void satellite_free(Satellite *satellite)
{
    g_return_if_fail(satellite);
    g_free(satellite->sat_used);
    g_slice_free(Satellite, satellite);
}

