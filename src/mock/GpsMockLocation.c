/* vim:set et: */
/* Copyright (C) 2015, LG Electronics

2015.11 Hee S. Jeong, initial implementation */

#define MOCK_LOCATION_INTERNAL_SOURCE
#include <string.h>
#include <loc_log.h>
#include "MockLocation.h"



extern struct _mock_location_provider gps_mock_location_provider;
static void (* gps_location_cb)(nyx_gps_location_t *positiondata, void *user_data);
static void (* gps_sv_status_cb)(nyx_gps_sv_status_t *sat_data, void *user_data);
static void (*gps_nmea_cb)(int64_t timestamp, const char* nmea, int length, void *user_data);


static void
gps_mock_location( struct _Location* loc, void* ctx )
{
    if ( loc ) {
        nyx_gps_location_t gpsloc;
        memset( &gpsloc, 0, sizeof(nyx_gps_location_t) );
        gpsloc.size = sizeof(nyx_gps_location_t);
        gpsloc.flags = NYX_GPS_LOCATION_HAS_LAT_LONG;
        gpsloc.latitude = loc->latitude;
        gpsloc.longitude = loc->longitude;
        if ( loc->flag & LOCATION_ALTITUDE_BIT ) {
            gpsloc.altitude = loc->altitude;
            gpsloc.flags |= NYX_GPS_LOCATION_HAS_ALTITUDE;
        }
        if ( loc->flag & LOCATION_SPEED_BIT ) {
            gpsloc.speed = loc->speed;
            gpsloc.flags |= NYX_GPS_LOCATION_HAS_SPEED;
        }
        if ( loc->flag & LOCATION_HEADING_BIT ) {
            gpsloc.bearing = loc->heading;
            gpsloc.flags |= NYX_GPS_LOCATION_HAS_BEARING;
        }
        if ( loc->flag & LOCATION_HORIZONTAL_ACCURACY_BIT ) {
            gpsloc.accuracy = loc->horizontalAccuracy;
            gpsloc.flags |= NYX_GPS_LOCATION_HAS_ACCURACY;
        }
        if ( loc->flag & LOCATION_TIMESTAMP_BIT ) {
            gpsloc.timestamp = loc->timestamp;
        }
        gps_location_cb( &gpsloc, ctx );
        LS_LOG_INFO( MOCK_TAG "GPS : %f %f\n", gpsloc.latitude, gpsloc.altitude );
    }
}

nyx_error_t
gps_mock_init(nyx_device_handle_t handle, nyx_gps_callbacks_t *gps_cbs, nyx_gps_xtra_callbacks_t *xtra_cbs, nyx_agps_callbacks_t *agps_cbs, nyx_gps_ni_callbacks_t *gps_ni_cbs, nyx_agps_ril_callbacks_t *agps_ril_cbs, nyx_gps_geofence_callbacks_t *geofence_cbs)
{
    gps_mock_location_provider.ctx = gps_cbs->user_data;
    gps_mock_location_provider.location = gps_mock_location;
    gps_location_cb = gps_cbs->location_cb;
    gps_sv_status_cb = gps_cbs->sv_status_cb;
    gps_nmea_cb = gps_cbs->nmea_cb;

    return nyx_gps_init(handle, gps_cbs, xtra_cbs, agps_cbs,
        gps_ni_cbs, agps_ril_cbs, geofence_cbs );
}



nyx_error_t
gps_mock_start(nyx_device_handle_t handle)
{
    if ( start_mock_location( &gps_mock_location_provider ) == LOCATION_SUCCESS )
        return NYX_ERROR_NONE;
    return nyx_gps_start( handle );
}

nyx_error_t
gps_mock_stop(nyx_device_handle_t handle)
{
    if ( stop_mock_location( &gps_mock_location_provider ) == LOCATION_SUCCESS )
        return NYX_ERROR_NONE;
    return nyx_gps_stop( handle );
}

/* EOF */
