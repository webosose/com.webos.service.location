/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Wifi_Plugin.c
 * Purpose      : Plugin to the WIFIHandler ,implementation based on D-Bus Geoclue wifi service.
 * Platform     : RedRose
 * Author(s)    : Abhishek Srivastava
 * Email ID.    : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No    Modified by     Date    Version Description
 *
 *
 ********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Location_Plugin.h>
#include <Location.h>
#include <Position.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <loc_log.h>

/*All g-signals indexes*/
enum {
    SIGNAL_WIFI_STATUS,
    SIGNAL_TRACKING,
    SIGNAL_END
};

/**
 * Local Wifi plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeocluePosition *geoclue_pos;

    /*
     * CallBacks from Wifi Handler, Location_Plugin.h
     */
    PositionCallback position_cb;
    StartTrackingCallBack tracking_cb;
    gpointer userdata;
    char *license_key;
    gulong signal_handler_ids[SIGNAL_END];
} GeoclueWifi;

#define LGE_WIFI_SERVICE_NAME       "org.freedesktop.Geoclue.Providers.LgeWifiService"
#define LGE_WIFI_SERVICE_PATH       "/org/freedesktop/Geoclue/Providers/LgeWifiService"

static void status_cb(GeoclueProvider *provider, GeoclueStatus status, gpointer userdata);
static void tracking_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer userdata);

static void signal_connection(GeoclueWifi *geoclueWifi, int signal_type, gboolean connect)
{
    gpointer signal_instance = NULL;
    gchar *signal_name = NULL;
    GCallback signal_cb = NULL;

    if (geoclueWifi == NULL)
        return;

    if (signal_type < 0 || signal_type >= SIGNAL_END)
        return;

    switch (signal_type) {
        case SIGNAL_WIFI_STATUS:
            signal_instance = G_OBJECT(geoclueWifi->geoclue_pos);
            signal_name = "status-changed";
            signal_cb = G_CALLBACK(status_cb);
            break;
        case SIGNAL_TRACKING:
            signal_instance = G_OBJECT(geoclueWifi->geoclue_pos);
            signal_name = "position-changed";
            signal_cb = G_CALLBACK(tracking_cb);
            break;
        default:
            break;
    }

    if (signal_instance == NULL)
        return;

    if (connect) {
        if (!g_signal_handler_is_connected(signal_instance,
                                           geoclueWifi->signal_handler_ids[signal_type])) {
            geoclueWifi->signal_handler_ids[signal_type] = g_signal_connect(signal_instance,
                                                                            signal_name,
                                                                            signal_cb,
                                                                            geoclueWifi);
            LS_LOG_INFO("Connecting singal [%d:%s] %d\n",
                        signal_type,
                        signal_name,
                        geoclueWifi->signal_handler_ids[signal_type]);
        } else {
            LS_LOG_INFO("Already connected signal [%d:%s] %d\n",
                        signal_type,
                        signal_name,
                        geoclueWifi->signal_handler_ids[signal_type]);
        }
    } else {
        g_signal_handler_disconnect(signal_instance,
                                    geoclueWifi->signal_handler_ids[signal_type]);
        LS_LOG_INFO("Disconnecting singal [%d:%s] %d\n",
                    signal_type,
                    signal_name,
                    geoclueWifi->signal_handler_ids[signal_type]);

        geoclueWifi->signal_handler_ids[signal_type] = 0;
    }
}

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
        LS_LOG_ERROR("[DEBUG] WIFI Error geoclue_provider_set_options(%s) : %s", key, error->message);
        g_value_unset(gvalue);
        g_error_free(error);
        g_free(gvalue);
        g_hash_table_destroy(options);
        return FALSE;
    }

    LS_LOG_INFO("[DEBUG] WIFI Success to geoclue_provider_set_options(%s)", key);
    g_value_unset(gvalue);
    g_free(gvalue);
    g_hash_table_destroy(options);

    return TRUE;
}

static void status_cb(GeoclueProvider *provider, GeoclueStatus status, gpointer userdata)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) userdata;

    g_return_if_fail(geoclueWifi);

    switch (status) {
        case GEOCLUE_STATUS_UNAVAILABLE: {
            LS_LOG_DEBUG("[DEBUG] WIFI status_cb: status = GEOCLUE_STATUS_UNAVAILABLE\n");

            if (geoclueWifi->position_cb) {
                (*(geoclueWifi->position_cb))(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, geoclueWifi->userdata, HANDLER_WIFI);
            }

            if (geoclueWifi->tracking_cb) {
                (*(geoclueWifi->tracking_cb))(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, geoclueWifi->userdata, HANDLER_WIFI);
            }
        }
            break;

        default:
            break;
    }
}

/**
 * <Funciton >   position_cb_async
 * <Description>  position callback ,called by wifi plugin when
 * it got response for position request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <error if any error sets the error code>
 * @param     <self> <In> <userdata userdata wifiplugin instance>
 * @return    void
 */
static void position_cb_async(GeocluePosition *position,
                              GeocluePositionFields fields,
                              int64_t timestamp,
                              double latitude,
                              double longitude,
                              double altitude,
                              GeoclueAccuracy *accuracy,
                              GError *error,
                              gpointer userdata)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) userdata;
    Position *ret_pos = NULL;
    Accuracy *ret_acc = NULL;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;
    ErrorCodes error_code = ERROR_NOT_AVAILABLE;

    if (error) {
        LS_LOG_ERROR("Error getting position: %s", error->message);
        g_error_free(error);

        if (geoclueWifi && geoclueWifi->position_cb)
            (*(geoclueWifi->position_cb))(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, geoclueWifi->userdata, HANDLER_WIFI);

        return;
    }

    LS_LOG_INFO("[DEBUG] WIFI position_cb_async: latitude = %f, longitude = %f\n", latitude, longitude);

    if (accuracy)
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
    else
        hor_acc = INVALID_PARAM;

    fields = fields | POSITION_FIELDS_ALTITUDE | VELOCITY_FIELDS_SPEED | VELOCITY_FIELDS_DIRECTION | VELOCITY_FIELDS_CLIMB;
    ret_pos = position_create(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, fields);
    ret_acc = accuracy_create(level, hor_acc, INVALID_PARAM);

    if (!ret_pos || !ret_acc)
        goto DONE;

    error_code = ERROR_NONE;
DONE:

    if (geoclueWifi && geoclueWifi->position_cb) {

        set_store_position(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, hor_acc, INVALID_PARAM, LOCATION_DB_PREF_PATH_WIFI);

        (*(geoclueWifi->position_cb))(TRUE, ret_pos, ret_acc, error_code, geoclueWifi->userdata, HANDLER_WIFI);
    }

    geoclue_accuracy_free(accuracy);
    position_free(ret_pos);
    accuracy_free(ret_acc);
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
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) userdata;
    Position *ret_pos = NULL;
    Accuracy *ret_acc = NULL;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level = GEOCLUE_ACCURACY_LEVEL_NONE;

    g_return_if_fail(geoclueWifi);
    g_return_if_fail(geoclueWifi->tracking_cb);

    LS_LOG_DEBUG("[DEBUG] WIFI tracking_cb: latitude = %f, longitude = %f\n", latitude, longitude);

    if (accuracy)
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
    else
        hor_acc = INVALID_PARAM;

    fields = fields | POSITION_FIELDS_ALTITUDE | VELOCITY_FIELDS_SPEED | VELOCITY_FIELDS_DIRECTION | VELOCITY_FIELDS_CLIMB;
    ret_pos = position_create(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, fields);
    ret_acc = accuracy_create(level, hor_acc, INVALID_PARAM);

    if (!ret_pos || !ret_acc)
        return;

    set_store_position(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, hor_acc, INVALID_PARAM, LOCATION_DB_PREF_PATH_WIFI);

    (*(geoclueWifi->tracking_cb))(TRUE, ret_pos, ret_acc, ERROR_NONE, geoclueWifi->userdata, HANDLER_WIFI);

    position_free(ret_pos);
    accuracy_free(ret_acc);
}

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueWifi> <In> <wifi plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueWifi *geoclueWifi)
{
    if (geoclueWifi->geoclue_pos) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(geoclueWifi->geoclue_pos),
                                             G_CALLBACK(tracking_cb),
                                             geoclueWifi);
        g_signal_handlers_disconnect_by_func(G_OBJECT(geoclueWifi->geoclue_pos),
                                             G_CALLBACK(status_cb),
                                             geoclueWifi);
        g_object_unref(geoclueWifi->geoclue_pos);
        geoclueWifi->geoclue_pos = NULL;
    }

    memset(geoclueWifi->signal_handler_ids, 0, sizeof(gulong)*SIGNAL_END);
}

/**
 * <Funciton >   intialize_wifi_geoclue_service
 * <Description>  create the GPS geoclue service instance
 * @param     <GeoclueWifi> <In> <wifi plugin structure>
 * @return    gboolean
 */
static gboolean intialize_wifi_geoclue_service(GeoclueWifi *geoclueWifi)
{
    g_return_val_if_fail(geoclueWifi, FALSE);

    //Check its alreasy started
    if (geoclueWifi->geoclue_pos)
        return TRUE;

    /*
     *  Create Geoclue position & velocity instance
     */
    geoclueWifi->geoclue_pos = geoclue_position_new(LGE_WIFI_SERVICE_NAME, LGE_WIFI_SERVICE_PATH);

    if (geoclueWifi->geoclue_pos == NULL) {
        LS_LOG_ERROR("[DEBUG] Error while creating LG Wifi geoclue object !!");
        unreference_geoclue(geoclueWifi);
        return FALSE;
    }

    signal_connection(geoclueWifi, SIGNAL_WIFI_STATUS, TRUE);

    LS_LOG_INFO("[DEBUG] intialize_wifi_geoclue_service done\n");

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
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;

    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("[DEBUG] WIFI start called\n");
    geoclueWifi->userdata = userdata;

    if (intialize_wifi_geoclue_service(geoclueWifi) == FALSE)
        return ERROR_NOT_AVAILABLE;

    geoclueWifi->license_key = g_strdup(license_key);


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
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;
    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("[DEBUG] WIFI stop called\n");
    unreference_geoclue(geoclueWifi);

    if (geoclueWifi->license_key != NULL)
        g_free(geoclueWifi->license_key);

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
static int get_position(gpointer handle, PositionCallback pos_cb)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;

    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(geoclueWifi->geoclue_pos, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueWifi->geoclue_pos, "GEOLOCATION", geoclueWifi->license_key) == TRUE) {
        LS_LOG_WARNING("License key Sent Success\n");
    } else
        return ERROR_NOT_AVAILABLE;

    geoclueWifi->position_cb = pos_cb;
    geoclue_position_get_position_async(geoclueWifi->geoclue_pos, (GeocluePositionCallback) position_cb_async, geoclueWifi);

    return ERROR_NONE;
}

/**
 * <Funciton >   start_tracking
 * <Description>  Get the position from GPS
 * @param     <handle> <In> <local Geoclue GPS handle>
 * @param     <Satellite> <In> <Satellite info result>
 * @return    int
 */
static int start_tracking(gpointer handle, gboolean enable, StartTrackingCallBack track_cb)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;

    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(geoclueWifi->geoclue_pos, ERROR_NOT_AVAILABLE);

    geoclueWifi->tracking_cb = NULL;

    if (enable) {
        if (send_geoclue_command(geoclueWifi->geoclue_pos, "GEOLOCATION", geoclueWifi->license_key) == TRUE) {
            LS_LOG_WARNING("License key Sent Success\n");
        } else
            return ERROR_NOT_AVAILABLE;

        if (send_geoclue_command(geoclueWifi->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESON") == FALSE) {
            LS_LOG_ERROR("[DEBUG] WIFI start_tracking: tracking on failed\n");
            return ERROR_NOT_AVAILABLE;
        }

        geoclueWifi->tracking_cb = track_cb;
        signal_connection(geoclueWifi, SIGNAL_TRACKING, TRUE);

        LS_LOG_DEBUG("[DEBUG] WIFI start_tracking: tracking on succeeded\n");
    } else {
        if (send_geoclue_command(geoclueWifi->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESOFF") == FALSE) {
            LS_LOG_WARNING("[DEBUG] WIFI start_tracking: tracking off failed\n");
            return ERROR_NOT_AVAILABLE;
        }

        signal_connection(geoclueWifi, SIGNAL_TRACKING, FALSE);
    }

    return ERROR_NONE;
}

/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <WifiPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(WifiPluginOps *ops)
{
    GeoclueWifi *geoclueWifi;
    g_return_val_if_fail(ops, NULL);
    LS_LOG_INFO("[DEBUG] WIFI init called\n");
    geoclueWifi = g_new0(GeoclueWifi, 1);
    g_return_val_if_fail(geoclueWifi, NULL);

    ops->start = start;
    ops->stop = stop;
    ops->get_position = get_position;
    ops->start_tracking = start_tracking;

    return (gpointer) geoclueWifi;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;
    g_return_if_fail(geoclueWifi);

    LS_LOG_INFO("[DEBUG] WIFI shutdown called\n");
    unreference_geoclue(geoclueWifi);

    g_free(geoclueWifi);
}

