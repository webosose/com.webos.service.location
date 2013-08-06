/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Wifi_Plugin.c
 * Purpose  : Plugin to the WIFIHandler ,implementation based on  D-Bus Geoclue wifi service.
 * Platform  : RedRose
 * Author(s) : Abhishek Srivastava
 * Email ID. : abhishek.srivastava@lge.com
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
#include <geoclue/geoclue-position.h>

#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <LocationService_Log.h>

// PmLogging
#define LS_LOG_CONTEXT_NAME     "avocado.location.wifi_plugin"
PmLogContext gLsLogContext;

/**
 * Local GPS plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeocluePosition *geoclue_pos;

    /*
     * CallBacks from GPS Handler ,Location_Plugin.h
     */

    PositionCallback pos_cb;
    StartTrackingCallBack track_cb;
    gpointer userdata;
    guint log_handler;
} GeoclueWifi;

#define LGE_WIFI_SERVICE_NAME "org.freedesktop.Geoclue.Providers.LgeWifiService"
#define LGE_WIFI_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/LgeWifiService"

static void position_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy, gpointer userdata);
static void status_cb(GeoclueProvider *provider, gint status, gpointer userdata);
/**
 **
 * <Funciton >   send_geoclue_command
 * <Description>  send geoclue command
 * @return    int
 */
static gboolean send_geoclue_command(GeocluePosition *instance, gchar *key, gchar *value)
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
        g_free(gvalue);
        g_hash_table_destroy(options);
        return FALSE;
    } else {
        LS_LOG_DEBUG("[DEBUG] Success to geoclue_provider_set_options(%s)", gvalue);
    }

    g_free(gvalue);
    g_hash_table_destroy(options);
    return TRUE;
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
static void position_cb_async(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                              double longitude,
                              double altitude, GeoclueAccuracy *accuracy, GError *error, gpointer userdata)
{
    GeoclueWifi *plugin_data = (GeoclueWifi *) userdata;
    g_return_if_fail(plugin_data);

    if (error) {
        switch (error->code) {
            case GEOCLUE_ERROR_NOT_AVAILABLE:
                LS_LOG_DEBUG("[DEBUG] Aquiring position for wifi");
                break;
                /*case ERROR_NO_MEMORY:
                 case ERROR_NO_RESPONSE:
                 (*plugin_data->pos_cb)(TRUE ,NULL, NULL,ERROR_NOT_AVAILABLE,plugin_data->userdata, HANDLER_WIFI);
                 break;*/
deafault:
                break;
        }
    } else {
        g_return_if_fail(plugin_data->geoclue_pos);
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(plugin_data->geoclue_pos)), G_CALLBACK(position_cb),
                                             plugin_data);
        position_cb(position, fields, timestamp, latitude, longitude, altitude, accuracy, userdata);
    }
}
/**
 * <Funciton >   position_cb
 * <Description>  position callback ,called by wifi plugin when
 * it got response for position async request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <userdata userdata wifiplugin instance>
 * @return    void
 */
static void position_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy, gpointer userdata)
{
    LS_LOG_DEBUG("[DEBUG] position_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueWifi *plugin_data = (GeoclueWifi *) userdata;
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
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, DEFAULT_VALUE, DEFAULT_VALUE,
                                        DEFAULT_VALUE, fields);
    g_return_if_fail(ret_pos);

    if (accuracy) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    g_return_if_fail(ac);
    //Call the GPS Handler callback
    (*plugin_data->pos_cb)(TRUE, ret_pos, ac, ERROR_NONE, plugin_data->userdata, HANDLER_WIFI);
    position_free(ret_pos);
    accuracy_free(ac);
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
static void tracking_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy,

                        gpointer plugin_data)
{
    LS_LOG_DEBUG("[DEBUG] Wifi Plugin CC: tracking_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueWifi *wifiPlugin = (GeoclueWifi *) plugin_data;
    g_return_if_fail(wifiPlugin);
    g_return_if_fail(wifiPlugin->track_cb);
    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level;
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, DEFAULT_VALUE, DEFAULT_VALUE,
                                        DEFAULT_VALUE, fields);
    g_return_if_fail(ret_pos);

    if (accuracy) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    g_return_if_fail(ac);
    (*wifiPlugin->track_cb)(TRUE, ret_pos, ac, ERROR_NONE, wifiPlugin->userdata, HANDLER_WIFI);
    position_free(ret_pos);
    accuracy_free(ac);
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
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;
    LS_LOG_DEBUG("[DEBUG] enter in plugin callback");
    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    geoclueWifi->pos_cb = positionCB;
    g_return_val_if_fail(geoclueWifi->geoclue_pos, ERROR_NOT_AVAILABLE);
    geoclue_position_get_position_async(geoclueWifi->geoclue_pos, (GeocluePositionCallback) position_cb_async, geoclueWifi);
    g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), "position-changed", G_CALLBACK(position_cb),
                     geoclueWifi);
    return ERROR_NONE;
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
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), G_CALLBACK(status_cb),
                                             geoclueWifi);
        g_object_unref(geoclueWifi->geoclue_pos);
        geoclueWifi->geoclue_pos = NULL;
    }
}
static void status_cb(GeoclueProvider *provider, gint status, gpointer userdata)
{
    GeoclueWifi *plugin_data = (GeoclueWifi *) userdata;
    g_return_if_fail(plugin_data);

    switch (status) {
        case GEOCLUE_STATUS_UNAVAILABLE: {
            LS_LOG_DEBUG("[DEBUG] gps-plugin status called status GEOCLUE_STATUS_AVAILABLE:\n");

            if (plugin_data->pos_cb != NULL) {
                (*plugin_data->pos_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, plugin_data->userdata, HANDLER_WIFI);
            }

            if (plugin_data->track_cb) {
                (*plugin_data->track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, plugin_data->userdata, HANDLER_WIFI);
            }

            g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(plugin_data->geoclue_pos)), G_CALLBACK(status_cb),
                                                 plugin_data);
        }
        break;

        default:
            break;
    }
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
    if (geoclueWifi->geoclue_pos) {
        return TRUE;
    }

    /*
     *  Create Geoclue position & velocity instance
     */
    geoclueWifi->geoclue_pos = geoclue_position_new(LGE_WIFI_SERVICE_NAME, LGE_WIFI_SERVICE_PATH);

    if (geoclueWifi->geoclue_pos == NULL) {
        LS_LOG_DEBUG("[DEBUG] Error while creating LG Wifi geoclue object !!");
        unreference_geoclue(geoclueWifi);
        return FALSE;
    }

    g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), "status-changed", G_CALLBACK(status_cb),
                     geoclueWifi);
    LS_LOG_DEBUG("[DEBUG] intialize_wifi_geoclue_service  done\n");
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
static int start(gpointer plugin_data, gpointer handler_data)
{
    LS_LOG_DEBUG("[DEBUG] wifi plugin start  plugin_data : %d  ,handler_data :%d \n", plugin_data, handler_data);
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) plugin_data;
    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    geoclueWifi->userdata = handler_data;

    if (intialize_wifi_geoclue_service(geoclueWifi) == FALSE) {
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

/**
 * <Funciton >   start_tracking
 * <Description>  Get the position from GPS
 * @param     <handle> <In> <local Geoclue GPS handle>
 * @param     <Satellite> <In> <Satellite info result>
 * @return    int
 */
static int start_tracking(gpointer plugin_data, gboolean enableTracking, StartTrackingCallBack handler_tracking_cb)
{
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) plugin_data;
    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("[DEBUG] WIFI Plugin start_tracking ");
    geoclueWifi->track_cb = NULL;

    if (geoclueWifi->geoclue_pos == NULL) return ERROR_NOT_AVAILABLE;

    if (enableTracking) {
        geoclueWifi->track_cb = handler_tracking_cb;

        if (send_geoclue_command(geoclueWifi->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESON") == FALSE) {
            geoclueWifi->track_cb = NULL;
            return ERROR_NOT_AVAILABLE;
        }

        g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), "position-changed", G_CALLBACK(tracking_cb),
                         plugin_data);
        LS_LOG_DEBUG("[DEBUG] registered position-changed signal \n");
        return ERROR_NONE;
    } else {
        send_geoclue_command(geoclueWifi->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESOFF");
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), G_CALLBACK(tracking_cb),
                                             plugin_data);
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueWifi->geoclue_pos)), G_CALLBACK(status_cb),
                                             geoclueWifi);
        return ERROR_NONE;
    }
}

/**
 * <Funciton >   stop
 * <Description>  stop the plugin
 * @param     <handle> <In> <enable/disable the callback>
 * @return    int
 */
static int stop(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]wifi plugin stop\n");
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;
    g_return_val_if_fail(geoclueWifi, ERROR_NOT_AVAILABLE);
    unreference_geoclue(geoclueWifi);
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
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);
    LS_LOG_DEBUG("[DEBUG]wifi plugin init\n");
    ops->start = start;
    ops->stop = stop;
    ops->get_position = get_position;
    ops->start_tracking = start_tracking;
    /*
     * Handler for plugin
     */
    GeoclueWifi *wifid = g_new0(GeoclueWifi, 1);

    if (wifid == NULL) return NULL;

    wifid->log_handler = g_log_set_handler(NULL,
                                           G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, lsloghandler,
                                           NULL);
    return (gpointer) wifid;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]wifi plugin shutdown\n");
    g_return_if_fail(handle);
    GeoclueWifi *geoclueWifi = (GeoclueWifi *) handle;
    g_log_remove_handler(NULL, geoclueWifi->log_handler);
    unreference_geoclue(geoclueWifi);
    g_free(geoclueWifi);
}

