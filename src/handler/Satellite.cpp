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


#include <glib.h>
#include <stdlib.h>
#include <Position.h>


Satellite *satellite_create(guint visible_satellites_count) {
    Satellite *satellite = g_slice_new0(Satellite);
    g_return_val_if_fail(satellite, NULL);

    satellite->visible_satellites_count = visible_satellites_count;
    satellite->sat_used = g_new0(SatelliteInfo, satellite->visible_satellites_count);

    return satellite;
}

static void update_satellite_used_in_fix_count(Satellite *satellite) {
    g_return_if_fail(satellite);

    satellite->num_satellite_used = 0;
    guint index = 0;

    if (satellite->visible_satellites_count > 0) {
        for (index = 0; index < satellite->visible_satellites_count; index++)
            if (satellite->sat_used[index].used)(satellite->num_satellite_used)++;
    }
}

int set_satellite_details(Satellite *satellite, gint index, gdouble snr, gint prn, gdouble elevation, gdouble azimuth,
                          gboolean used, gboolean hasalmanac, gboolean hasephemeris) {
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

void satellite_free(Satellite *satellite) {
    g_return_if_fail(satellite);

    g_free(satellite->sat_used);
    g_slice_free(Satellite, satellite);
}

