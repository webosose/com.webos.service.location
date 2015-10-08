/********************************************************************
 * Copyright (c) 2015 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : CSP-2
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Network_Plugin.c
 * Purpose      : Plugin to the NetworkHandler ,implementation based on D-Bus Geoclue network service.
 * Platform     : W2
 * Author(s)    : deepa.sangolli
 * Email ID.    : deepa.sangolli@lge.com
 * Creation Date: 12-05-2015
 *
 * Modifications:
 * Sl No    Modified by     Date    Version Description
 *
 *
 ********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <Location_Plugin.h>
#include <Location.h>
#include <Position.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <Gps_stored_data.h>
#include <loc_log.h>


/**
 * Local Network plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeocluePosition *geoclue_pos;

    /*
     * CallBacks from Network Handler, Location_Plugin.h
     */
    StartTrackingCallBack tracking_cb;
    gpointer userdata;
    char *license_key;
} GeoclueNetwork;

#define LGE_NETWORK_SERVICE_NAME       "org.freedesktop.Geoclue.Providers.LgeNetworkService"
#define LGE_NETWORK_SERVICE_PATH       "/org/freedesktop/Geoclue/Providers/LgeNetworkService"

/**
 **
 * <Funciton >   send_geoclue_command
 * <Description>  send geoclue command
 * @return    int
 */
static gboolean send_geoclue_command(GeocluePosition *instance, gchar *key, gchar *value)
{
    GHashTable *options;
    GValue *gvalue;
    GError *error = NULL;
    options = g_hash_table_new(g_str_hash, g_str_equal);
    gvalue = g_new0(GValue, 1);

    g_value_init(gvalue, G_TYPE_STRING);
    g_value_set_string(gvalue, value);
    g_hash_table_insert(options, key, gvalue);

    if (!geoclue_provider_set_options(GEOCLUE_PROVIDER(instance), options, &error)) {
        LS_LOG_ERROR("[DEBUG] NW Error geoclue_provider_set_options(%s) : %s", key, error->message);
        g_value_unset(gvalue);
        g_error_free(error);
        g_free(gvalue);
        g_hash_table_destroy(options);
        return FALSE;
    }

    LS_LOG_INFO("NW Success to geoclue_provider_set_options(%s)", key);
    g_value_unset(gvalue);
    g_free(gvalue);
    g_hash_table_destroy(options);

    return TRUE;
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
static void tracking_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer userdata)
{
    GeoclueNetwork *geoclueNetwork = (GeoclueNetwork *) userdata;
    Position *ret_pos = NULL;
    Accuracy *ret_acc = NULL;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;

    g_return_if_fail(geoclueNetwork);
    g_return_if_fail(geoclueNetwork->tracking_cb);

    LS_LOG_DEBUG("[DEBUG] NW tracking_cb: latitude = %f, longitude = %f\n", latitude, longitude);

    if (accuracy)
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
    else
        hor_acc = INVALID_PARAM;

    fields = fields | POSITION_FIELDS_ALTITUDE | VELOCITY_FIELDS_SPEED | VELOCITY_FIELDS_DIRECTION | VELOCITY_FIELDS_CLIMB;
    ret_pos = position_create(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, fields);
    if (!ret_pos) {
       return;
       }

    ret_acc = accuracy_create(level, hor_acc, INVALID_PARAM);
    if (!ret_acc) {
       position_free(ret_pos);
       return;
       }

    set_store_position(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, hor_acc, INVALID_PARAM, LOCATION_DB_PREF_PATH_NETWORK);

    (*(geoclueNetwork->tracking_cb))(TRUE, ret_pos, ret_acc, ERROR_NONE, geoclueNetwork->userdata, HANDLER_NETWORK);

    position_free(ret_pos);
    accuracy_free(ret_acc);
}

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <Geocluenetwork> <In> <network plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueNetwork *geoclueNetwork)
{
    if (geoclueNetwork->geoclue_pos) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueNetwork->geoclue_pos)), G_CALLBACK(tracking_cb), geoclueNetwork);
        g_object_unref(geoclueNetwork->geoclue_pos);
        geoclueNetwork->geoclue_pos = NULL;
    }
}

/**
 * <Funciton >   intialize_network_geoclue_service
 * <Description>  create the network geoclue service instance
 * @param     <GeoclueNw> <In> <network plugin structure>
 * @return    gboolean
 */
static gboolean intialize_network_geoclue_service(GeoclueNetwork *geoclueNetwork)
{
    g_return_val_if_fail(geoclueNetwork, FALSE);

    //Check its alreasy started
    if (geoclueNetwork->geoclue_pos)
        return TRUE;

    /*
     *  Create Geoclue position & velocity instance
     */
    geoclueNetwork->geoclue_pos = geoclue_position_new(LGE_NETWORK_SERVICE_NAME, LGE_NETWORK_SERVICE_PATH);

    if (!geoclueNetwork->geoclue_pos) {
        LS_LOG_ERROR("[DEBUG] Error while creating LG network geoclue object !!");
        unreference_geoclue(geoclueNetwork);
        return FALSE;
    }
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
static int start(gpointer handle, gpointer userdata, const char *license_key)
{
    GeoclueNetwork *geoclueNetwork = (GeoclueNetwork *) handle;

    g_return_val_if_fail(geoclueNetwork, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO(" network plugin start called\n");
    geoclueNetwork->userdata = userdata;

    if (!intialize_network_geoclue_service(geoclueNetwork))
        return ERROR_NOT_AVAILABLE;

    geoclueNetwork->license_key = g_strdup(license_key);


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
    GeoclueNetwork *geoclueNetwork = (GeoclueNetwork *) handle;
    g_return_val_if_fail(geoclueNetwork, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO(" Network plugin stop called\n");
    unreference_geoclue(geoclueNetwork);

    if (geoclueNetwork->license_key)
        g_free(geoclueNetwork->license_key);
        geoclueNetwork->license_key = NULL;

    return ERROR_NONE;
}


/**
 * <Funciton >   start_tracking
 * <Description>  Get the position from Network
 * @return    int
 */
static int start_tracking(gpointer handle, gboolean enable, StartTrackingCallBack track_cb, gpointer celldata)
{
    GeoclueNetwork *geoclueNetwork = (GeoclueNetwork *) handle;

    g_return_val_if_fail(geoclueNetwork, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(geoclueNetwork->geoclue_pos, ERROR_NOT_AVAILABLE);

    geoclueNetwork->tracking_cb = NULL;

    if (enable) {
        if (send_geoclue_command(geoclueNetwork->geoclue_pos, "GEOLOCATION", geoclueNetwork->license_key)) {
            LS_LOG_DEBUG("License key Sent Success\n");
        } else
            return ERROR_NOT_AVAILABLE;
        if (send_geoclue_command(geoclueNetwork->geoclue_pos, "CELLDATA", celldata)) {
            LS_LOG_DEBUG("CELLDATA Sent Success\n");
        } else
            return ERROR_NOT_AVAILABLE;
        if (!send_geoclue_command(geoclueNetwork->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESON")) {
            LS_LOG_ERROR("[DEBUG] NW start_tracking: tracking on failed\n");
            return ERROR_NOT_AVAILABLE;
        }

        geoclueNetwork->tracking_cb = track_cb;
        g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueNetwork->geoclue_pos)), "position-changed", G_CALLBACK(tracking_cb), geoclueNetwork);
        LS_LOG_DEBUG("[DEBUG] NW start_tracking: tracking on succeeded\n");
    } else {
        if (!send_geoclue_command(geoclueNetwork->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESOFF")) {
            LS_LOG_DEBUG("[DEBUG] NW start_tracking: tracking off failed\n");
            return ERROR_NOT_AVAILABLE;
        }

        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueNetwork->geoclue_pos)), G_CALLBACK(tracking_cb), geoclueNetwork);
    }

    return ERROR_NONE;
}

/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <networkPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(NetworkPluginOps *ops)
{
    GeoclueNetwork *geoclueNetwork;
    g_return_val_if_fail(ops, NULL);
    LS_LOG_INFO("network init called\n");
    geoclueNetwork = g_new0(GeoclueNetwork, 1);
    g_return_val_if_fail(geoclueNetwork, NULL);

    ops->start = start;
    ops->stop = stop;
    ops->start_tracking = start_tracking;

    return (gpointer) geoclueNetwork;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    GeoclueNetwork *geoclueNetwork = (GeoclueNetwork *) handle;
    g_return_if_fail(geoclueNetwork);

    LS_LOG_INFO("network shutdown called\n");
    unreference_geoclue(geoclueNetwork);

    g_free(geoclueNetwork);
}

