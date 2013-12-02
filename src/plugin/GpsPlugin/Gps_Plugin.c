/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Gps_Plugin.c
 * Purpose  : Plugin to the GPSHandler ,implementation based on  D-Bus Geoclue gps service.
 * Platform  : RedRose
 * Author(s) : Rajesh Gopu I.V , Abhishek Srivastava
 * Email ID. : rajeshgopu.iv@lge.com , abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <Location_Plugin.h>
#include <Location.h>
#include <Position.h>
#include <Accuracy.h>
#include <geoclue/geoclue-positiongps.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-nmea.h>
#include <geoclue/geoclue-satellite.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <LocationService_Log.h>

/**
 * Local GPS plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeocluePositionGps *geoclue_pos;
    GeoclueAccuracy *geoclue_accuracy;
    GeoclueNmea *geoclue_nmea;
    GeoclueSatellite *geoclue_satellite;

    /*
     * CallBacks from GPS Handler, Location_Plugin.h
     */
    StatusCallback status_cb;
    PositionCallback pos_cb;
    SatelliteCallback sat_cb;
    StartTrackingCallBack track_cb;
    NmeaCallback nmea_cb;

    gpointer userdata;
} GeoclueGps;

#define LGE_GPS_SERVICE_NAME    "org.freedesktop.Geoclue.Providers.LgeGpsService"
#define LGE_GPS_SERVICE_PATH    "/org/freedesktop/Geoclue/Providers/LgeGpsService"

static void position_cb(GeocluePositionGps *position, GeocluePositionGpsFields fields, int timestamp, double latitude,
                        double longitude,
                        double altitude, double speed, double direction, double climb, GeoclueAccuracy *accuracy, gpointer userdata);

static void satellite_cb(GeoclueSatellite *satellite, guint64 used_in_fix, int visible_satellites,
                         guint64 ephemeris_info, guint64 almanac_info,
                         GPtrArray *sat_data, gpointer userdata);

/**
 * <Funciton >   send_geoclue_command
 * <Description>  send geoclue command
 * @return    int
 */
static gboolean send_geoclue_command(GeocluePositionGps *instance, gchar *key, gchar *value)
{
    GHashTable *options;
    GError *error = NULL;
    options = g_hash_table_new(g_str_hash, g_str_equal);
    GValue *gvalue = g_new0(GValue, 1);
    g_value_init(gvalue, G_TYPE_STRING);
    g_value_set_string(gvalue, value);
    g_hash_table_insert(options, key, gvalue);

    if (!geoclue_provider_set_options(GEOCLUE_PROVIDER(instance), options, &error)) {
        LS_LOG_DEBUG("[DEBUG] Error geoclue_provider_set_options(%s) : %s", gvalue, error->message);
        g_error_free(error);
        error = NULL;
        g_hash_table_destroy(options);
        g_free(gvalue);
        return FALSE;
    }

    LS_LOG_DEBUG("[DEBUG] Success to geoclue_provider_set_options(%s)", gvalue);
    g_free(gvalue);
    g_hash_table_destroy(options);
    return TRUE;
}

/**
 * <Funciton >   position_cb_async
 * <Description>  position callback ,called by gps plugin when
 * it got response for position request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <error if any error sets the error code>
 * @param     <self> <In> <userdata userdata gpsplugin instance>
 * @return    void
 */
static void position_cb_async(GeocluePositionGps *position, GeocluePositionGpsFields fields, int timestamp,
                              double latitude, double longitude,
                              double altitude, double speed, double direction, double climb, GeoclueAccuracy *accuracy, GError *error,
                              gpointer userdata)
{
    GeoclueGps *plugin_data = (GeoclueGps *) userdata;
    g_return_if_fail(plugin_data);

    if (error) {
        LS_LOG_DEBUG("[DEBUG] Aquiring position");
    } else {
        LS_LOG_DEBUG("[DEBUG] Aquired position");
        g_return_if_fail(plugin_data->geoclue_pos);
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(plugin_data->geoclue_pos)), G_CALLBACK(position_cb),
                                             plugin_data);
        position_cb(position, fields, timestamp, latitude, longitude, altitude, speed, direction, climb, accuracy, userdata);
    }
}

/**
 * <Funciton >   position_cb
 * <Description>  position callback ,called by gps plugin when
 * it got response for position async request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <userdata userdata gpsplugin instance>
 * @return    void
 */
static void position_cb(GeocluePositionGps *position, GeocluePositionGpsFields fields, int timestamp, double latitude,
                        double longitude,
                        double altitude, double speed, double direction, double climb, GeoclueAccuracy *accuracy, gpointer userdata)
{
    LS_LOG_DEBUG("[DEBUG]position_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueGps *plugin_data = (GeoclueGps *) userdata;
    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->pos_cb);

    if (plugin_data->geoclue_pos != NULL) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(plugin_data->geoclue_pos)), G_CALLBACK(position_cb),
                                             plugin_data);
    }

    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level;
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, speed, direction, climb, fields);
    g_return_if_fail(ret_pos);

    if (accuracy) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    set_store_position(latitude, longitude, altitude, speed, direction, hor_acc, vert_acc, LOCATION_DB_PREF_PATH);
    //Call the GPS Handler callback
    (*plugin_data->pos_cb)(TRUE, ret_pos, ac, ERROR_NONE, plugin_data->userdata, HANDLER_GPS);
    position_free(ret_pos);
    accuracy_free(ac);
}

/**
 * <Funciton >   status_cb
 * <Description>  status callback ,called by gps plugin when
 * it got response for status async request
 **/
static void status_cb(GeoclueProvider *provider, gint status, gpointer userdata)
{
    GeoclueGps *plugin_data = (GeoclueGps *) userdata;
    //Call the GPS Handler callback
    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->status_cb);

    switch (status) {
        case GEOCLUE_STATUS_AVAILABLE:
            LS_LOG_DEBUG("[DEBUG] gps-plugin status called status GEOCLUE_STATUS_ACQUIRING:\n");
            (*plugin_data->status_cb)(FALSE, GPS_STATUS_AVAILABLE, plugin_data->userdata);
            break;

        case GEOCLUE_STATUS_UNAVAILABLE:
            LS_LOG_DEBUG("[DEBUG] gps-plugin status called status GEOCLUE_STATUS_AVAILABLE:\n");
            (*plugin_data->status_cb)(TRUE, GPS_STATUS_UNAVAILABLE, plugin_data->userdata);
            break;

        default:
            break;
    }
}

/**
 * <Funciton >   nmea_callback
 * <Description>  nmea  callback ,called by gps plugin when
 * it got response for nmea async request
 * */
static void nmea_callback(GeoclueNmea *nmea, int timestamp, gchar *data, int length, gpointer userdata)
{
    GeoclueGps *plugin_data = (GeoclueGps *) userdata;
    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->nmea_cb);
    LS_LOG_DEBUG("[DEBUG]value of nmea data %s", data);
    (*plugin_data->nmea_cb)(TRUE, timestamp, data, length, plugin_data->userdata);
}

/**
 * <Funciton >   satellite_cb
 * <Description>  satellite  callback ,called by gps plugin when
 * it got response for satellite async request
 * */
static void satellite_cb(GeoclueSatellite *satellite, guint64 used_in_fix, int visible_satellites,
                         guint64 ephemeris_info, guint64 almanac_info,
                         GPtrArray *sat_data, gpointer userdata)
{
    GeoclueGps *plugin_data = (GeoclueGps *) userdata;
    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->sat_cb);
    int index = DEFAULT_VALUE;
    Satellite *sat = NULL;
    sat = satellite_create(visible_satellites);
    g_return_if_fail(sat);
    sat->visible_satellites_count = visible_satellites;
    LS_LOG_DEBUG(" number of satellite %d : \n", visible_satellites);

    for (index = DEFAULT_VALUE; index < visible_satellites; index++) {
        gboolean used = FALSE;
        gboolean hasephemeris = FALSE;
        gboolean hasalmanac = FALSE;
        GValueArray *vals = (GValueArray *) g_ptr_array_index(sat_data, index);
        guint prn = g_value_get_uint(g_value_array_get_nth(vals, 0));
        gdouble snr = g_value_get_double(g_value_array_get_nth(vals, 1));
        gdouble elev = g_value_get_double(g_value_array_get_nth(vals, 2));
        gdouble azim = g_value_get_double(g_value_array_get_nth(vals, 3));
        ((used_in_fix & (1 << prn - 1))) == DEFAULT_VALUE ? (used = FALSE) : (used = TRUE);
        ((ephemeris_info & (1 << prn - 1))) == DEFAULT_VALUE ? (hasephemeris = FALSE) : (hasephemeris = TRUE);
        ((almanac_info & (1 << prn - 1))) == DEFAULT_VALUE ? (hasalmanac = FALSE) : (hasalmanac = TRUE);
        // LS_LOG_DEBUG("[DEBUG]value of satellite prn %d \n snr %f \n elev %f \n  azi %f \n used %d \n hasalmanac %d \n hasephemeris %d\n" , prn, snr,elev,azim,used,hasalmanac,hasephemeris);
        set_satellite_details(sat, index, snr, prn, elev, azim, used, hasalmanac, hasephemeris);
    }

    //call satellite cb
    if (visible_satellites > DEFAULT_VALUE)(*plugin_data->sat_cb)(TRUE, sat, plugin_data->userdata);

    satellite_free(sat);
}

/**
 * <Funciton >   tracking_cb
 * <Description>  tracking callback ,called by gps plugin when
 * it got response for tracking request request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <error if any error sets the error code>
 * @param     <self> <In> <userdata userdata gpsplugin instance>
 * @return    void
 */
static void tracking_cb(GeocluePositionGps *position, GeocluePositionGpsFields fields, int timestamp, double latitude,
                        double longitude,
                        double altitude, double speed, double direction, double climb, GeoclueAccuracy *accuracy,

                        gpointer plugin_data)
{
    LS_LOG_DEBUG("[DEBUG] GPS Plugin CC: tracking_cb  latitude =%f , longitude =%f ", latitude, longitude);
    GeoclueGps *gpsPlugin = (GeoclueGps *) plugin_data;
    g_return_if_fail(gpsPlugin);
    g_return_if_fail(gpsPlugin->track_cb);
    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level;
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, speed, direction, climb, fields);
    g_return_if_fail(ret_pos);

    if (accuracy) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    set_store_position(latitude, longitude, altitude, speed, direction, hor_acc, vert_acc, LOCATION_DB_PREF_PATH);
    (*gpsPlugin->track_cb)(TRUE, ret_pos, ac, ERROR_NONE, gpsPlugin->userdata, HANDLER_GPS);
    position_free(ret_pos);
    accuracy_free(ac);
}
static int send_extra_command(gpointer handle , char *command)
{
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("send_extra_command command= %d", command);

    if (send_geoclue_command(geoclueGps->geoclue_pos , "REQUESTED_STATE", command) == FALSE)
        return ERROR_NOT_AVAILABLE;

    return ERROR_NONE;
}
int send_network_data(gpointer handle, gpointer data)
{
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("send_network_data %s", data);

    if (send_geoclue_command(geoclueGps->geoclue_pos , "Network_data", data) == FALSE) {
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}
/**
 * <Funciton >   get_position
 * <Description>  Get the position from GPS
 * @param     <handle> <In> <enable/disable the callback>
 * @param     <Position> <In> <Position structure info>
 * @param     <Accuracy> <In> <Accuracy info>
 * @return    int
 */
static int get_position(gpointer handle, PositionCallback positionCB)
{
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(geoclueGps->geoclue_pos, ERROR_NOT_AVAILABLE);
    geoclueGps->pos_cb = NULL;

    if (send_geoclue_command(geoclueGps->geoclue_pos, "REQUESTED_STATE",
                             "PERIODICUPDATES") == FALSE) return ERROR_NOT_AVAILABLE;

    if (send_geoclue_command(geoclueGps->geoclue_pos, "REQUESTED_STATE", "GPSENABLE") == FALSE) {
        LS_LOG_DEBUG("GPS ENGINE NOT STARTED");
        return ERROR_NOT_AVAILABLE;
    }

    geoclueGps->pos_cb = positionCB;
    geoclue_position_get_position_async(geoclueGps->geoclue_pos, (GeocluePositionGpsCallback) position_cb_async,
                                        geoclueGps);
    g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), "positiongps-changed", G_CALLBACK(position_cb),
                     geoclueGps);
    return ERROR_NONE;
}

static int get_gps_data(gpointer handle, gboolean enable_data, gpointer gps_cb, int datatype)
{
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);

    if (enable_data == TRUE) {
        if (send_geoclue_command(geoclueGps->geoclue_pos, "REQUESTED_STATE",
                                 "PERIODICUPDATES") == FALSE) return ERROR_NOT_AVAILABLE;

        if (send_geoclue_command(geoclueGps->geoclue_pos, "REQUESTED_STATE", "GPSENABLE") == FALSE) {
            LS_LOG_DEBUG("GPS ENGINE NOT STARTED");
            return ERROR_NOT_AVAILABLE;
        }
    }

    switch (datatype) {
        case HANDLER_DATA_NMEA: {
            g_return_val_if_fail(geoclueGps->geoclue_nmea, ERROR_NOT_AVAILABLE);
            geoclueGps->nmea_cb = NULL;

            if (enable_data) {
                geoclueGps->nmea_cb = (NmeaCallback) gps_cb;
                g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_nmea)), "nmea-changed", G_CALLBACK(nmea_callback),
                                 geoclueGps);
            } else g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_nmea)),
                        G_CALLBACK(nmea_callback), geoclueGps);
        }
        break;

        case HANDLER_DATA_SATELLITE: {
            // satellite
            g_return_val_if_fail(geoclueGps->geoclue_satellite, ERROR_NOT_AVAILABLE);
            geoclueGps->sat_cb = NULL;

            if (enable_data) {
                geoclueGps->sat_cb = (SatelliteCallback) gps_cb;
                g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_satellite)), "satellite-changed",
                                 G_CALLBACK(satellite_cb), geoclueGps);
            } else g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_satellite)),
                        G_CALLBACK(satellite_cb), geoclueGps);
        }
        break;

        case HANDLER_DATA_POSITION: {
            //starttracking
            if (geoclueGps->geoclue_pos == NULL) return ERROR_NOT_AVAILABLE;

            geoclueGps->track_cb = NULL;
            LS_LOG_DEBUG("[DEBUG] start_tracking userdata addr : %d  enableTracking =%d \n", geoclueGps->userdata, enable_data);

            if (enable_data) {
                geoclueGps->track_cb = (PositionCallback) gps_cb;
                g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), "positiongps-changed", G_CALLBACK(tracking_cb),
                                 geoclueGps);
                LS_LOG_DEBUG("[DEBUG] registered position-changed signal \n");
            } else {
                g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), G_CALLBACK(tracking_cb),
                                                     geoclueGps);
            }
        }
        break;
    } // switch end

    return ERROR_NONE;
}

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueGps> <In> <gps plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueGps *geoclueGps)
{
    if (geoclueGps->geoclue_pos) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), G_CALLBACK(status_cb),
                                             geoclueGps);
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), G_CALLBACK(position_cb),
                                             geoclueGps);
        g_object_unref(geoclueGps->geoclue_pos);
        geoclueGps->geoclue_pos = NULL;
    }

    if (geoclueGps->geoclue_nmea) {
        g_object_unref(geoclueGps->geoclue_nmea);
        geoclueGps->geoclue_nmea = NULL;
    }

    if (geoclueGps->geoclue_satellite) {
        g_object_unref(geoclueGps->geoclue_satellite);
        geoclueGps->geoclue_satellite = NULL;
    }
}

/**
 * <Funciton >   intialize_gps_geoclue_service
 * <Description>  create the GPS geoclue service instance
 * @param     <GeoclueGps> <In> <gps plugin structure>
 * @return    gboolean
 */
static gboolean intialize_gps_geoclue_service(GeoclueGps *geoclueGps)
{
    g_return_val_if_fail(geoclueGps, FALSE);

    //Check its alreasy started
    if (geoclueGps->geoclue_pos || geoclueGps->geoclue_nmea || geoclueGps->geoclue_satellite) {
        return TRUE;
    }

    /*
     *  Create Geoclue position instance
     */
    geoclueGps->geoclue_pos = geoclue_positiongps_new(LGE_GPS_SERVICE_NAME, LGE_GPS_SERVICE_PATH);
    geoclueGps->geoclue_nmea = geoclue_nmea_new(LGE_GPS_SERVICE_NAME, LGE_GPS_SERVICE_PATH);
    geoclueGps->geoclue_satellite = geoclue_satellite_new(LGE_GPS_SERVICE_NAME, LGE_GPS_SERVICE_PATH);

    if (geoclueGps->geoclue_pos == NULL || geoclueGps->geoclue_satellite == NULL || geoclueGps->geoclue_nmea == NULL) {
        LS_LOG_DEBUG("[DEBUG] Error while creating LG GPS geoclue object !!");
        unreference_geoclue(geoclueGps);
        return FALSE;
    }

    g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueGps->geoclue_pos)), "status-changed", G_CALLBACK(status_cb),
                     geoclueGps);
    LS_LOG_DEBUG("[DEBUG] intialize_gps_geoclue_service  done\n");
    return TRUE;
}

/**
 * <Funciton >   start
 * <Description>  start the plugin by resgistering the callbacks
 * @param     <StatusCallback> <In> <enable/disable the callback>
 * @param     <PositionCallback> <In> <extra commad identifier>
 * @param     <VelocityCallback> <In> <Gobject private instance>
 * @param     <SatelliteCallback> <In> <Gobject private instance>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
static int start(gpointer plugin_data, StatusCallback status_cb, gpointer handler_data)
{
    LS_LOG_DEBUG("[DEBUG] gps plugin start  plugin_data : %d  ,handler_data :%d \n", plugin_data, handler_data);
    GeoclueGps *geoclueGps = (GeoclueGps *) plugin_data;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);
    geoclueGps->userdata = handler_data;
    geoclueGps->status_cb = status_cb;

    if (intialize_gps_geoclue_service(geoclueGps) == FALSE) return ERROR_NOT_AVAILABLE;

    LS_LOG_DEBUG("[DEBUG] Sucess In start");
    return ERROR_NONE;
}

/**
 * <Funciton >   stop
 * <Description>  stop the plugin
 * @param     <handle> <In> <enable/disable the callback>
 * @return    int
 */
static int stop(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG] gps plugin stop\n");
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    g_return_val_if_fail(geoclueGps, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueGps->geoclue_pos, "REQUESTED_STATE", "GPSDISABLE") == FALSE) {
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <GpsPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(GpsPluginOps *ops)
{
    LS_LOG_DEBUG("[DEBUG]gps plugin init\n");
    ops->start = start;
    ops->stop = stop;
    ops->get_position = get_position;
    ops->get_gps_data = get_gps_data;
    ops->send_extra_command = send_extra_command;
    ops->send_network_data = send_network_data;
    /*
     * Handler for plugin
     */
    GeoclueGps *gpsd = g_new0(GeoclueGps, 1);

    if (gpsd == NULL) return NULL;

    return (gpointer) gpsd;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]gps plugin shutdown\n");
    g_return_if_fail(handle);
    GeoclueGps *geoclueGps = (GeoclueGps *) handle;
    unreference_geoclue(geoclueGps);
    g_free(geoclueGps);
}
