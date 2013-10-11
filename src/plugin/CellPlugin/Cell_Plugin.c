/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Cell_Plugin.c
 * Purpose  : Plugin to the WIFIHandler ,implementation based on  D-Bus Geoclue cell service.
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
#include <string.h>
#include <Location_Plugin.h>
#include <Location.h>
#include <Position.h>
#include <Accuracy.h>
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-velocity.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <LocationService_Log.h>

// PmLogging
#define LS_LOG_CONTEXT_NAME     "avocado.location.cell_plugin"
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
    gboolean track_started;
    gpointer userdata;
    guint log_handler;
} GeoclueCell;

#define LGE_CELL_SERVICE_NAME "org.freedesktop.Geoclue.Providers.LgeCellService"
#define LGE_CELL_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/LgeCellService"

static void position_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy, gpointer userdata);

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
    GValue *gvalue = NULL;
    options = g_hash_table_new(g_str_hash, g_str_equal);
    LS_LOG_DEBUG(" value of sendgeolcuecommand %s ", value);
    gvalue = g_new0(GValue, 1);
    g_value_init(gvalue, G_TYPE_STRING);
    g_value_set_string(gvalue, value);
    g_hash_table_insert(options, key, gvalue);

    if (geoclue_provider_set_options(GEOCLUE_PROVIDER(instance), options, &error) == FALSE) {
        LS_LOG_DEBUG("[DEBUG] Error geoclue_provider_set_options(%s) : %s", gvalue, error->message);
        g_error_free(error);
        error = NULL;
        g_hash_table_destroy(options);
        g_free(gvalue);
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
 * <Description>  position callback ,called by cell plugin when
 * it got response for position request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <error if any error sets the error code>
 * @param     <self> <In> <userdata userdata cellplugin instance>
 * @return    void
 */
static void position_cb_async(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                              double longitude,
                              double altitude, GeoclueAccuracy *accuracy, GError *error, gpointer userdata)
{
    GeoclueCell *plugin_data = (GeoclueCell *) userdata;
    g_return_if_fail(plugin_data);

    if (error) {
        g_error_free(error);
        (*plugin_data->pos_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, plugin_data->userdata, HANDLER_CELLID);
    } else {
        position_cb(position, fields, timestamp, latitude, longitude, altitude, accuracy, userdata);
    }
}
/**
 * <Funciton >   position_cb
 * <Description>  position callback ,called by cell plugin when
 * it got response for position async request
 * @param     <self> <In> <GeocluePosition position structure of geoclue>
 * @param     <self> <In> <GeocluePositionFields tells which fields are set>
 * @param     <self> <In> <timestamp timestamp of location>
 * @param     <self> <In> <latitude latitude value>
 * @param     <self> <In> <longitude longitude value>
 * @param     <self> <In> <altitude altitude value>
 * @param     <self> <In> <accuracy accuracy value>
 * @param     <self> <In> <userdata userdata cellplugin instance>
 * @return    void
 */
static void position_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy, gpointer userdata)
{
    LS_LOG_DEBUG("[DEBUG] position_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueCell *plugin_data = (GeoclueCell *) userdata;
    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->pos_cb);
    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level;
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, DEFAULT_VALUE, DEFAULT_VALUE,
                                        DEFAULT_VALUE, fields);
    g_return_if_fail(ret_pos);

    if (accuracy != NULL) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
        /* if(accuracy){
         // Free accuracy object if created
         geoclue_accuracy_free(accuracy);
         }*/
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    (*plugin_data->pos_cb)(TRUE, ret_pos, ac, ERROR_NONE, plugin_data->userdata, HANDLER_CELLID);
    position_free(ret_pos);
    accuracy_free(ac);
}

/**
 * <Funciton >      tracking_cb
 * <Description>    tracking callback ,called by gps plugin when
 *                  it got response for tracking request request
 * @param           <self> <In> <GeocluePosition position structure of geoclue>
 * @param           <self> <In> <GeocluePositionFields tells which fields are set>
 * @param           <self> <In> <timestamp timestamp of location>
 * @param           <self> <In> <latitude latitude value>
 * @param           <self> <In> <longitude longitude value>
 * @param           <self> <In> <altitude altitude value>
 * @param           <self> <In> <accuracy accuracy value>
 * @param           <self> <In> <error if any error sets the error code>
 * @param           <self> <In> <userdata userdata gpsplugin instance>
 * @return          void
 */
static void tracking_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude,
                        double longitude, double altitude,
                        GeoclueAccuracy *accuracy,

                        gpointer plugin_data)
{
    LS_LOG_DEBUG("[DEBUG] Cell Plugin CC: tracking_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueCell *cellPlugin = (GeoclueCell *) plugin_data;
    g_return_if_fail(cellPlugin);
    g_return_if_fail(cellPlugin->track_cb);
    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = DEFAULT_VALUE;
    GeoclueAccuracyLevel level;
    Position *ret_pos = position_create(timestamp, latitude, longitude, altitude, DEFAULT_VALUE, DEFAULT_VALUE,
                                        DEFAULT_VALUE, fields);
    g_return_if_fail(ret_pos);

    if (accuracy != NULL) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    (*cellPlugin->track_cb)(TRUE, ret_pos, ac, ERROR_NONE, cellPlugin->userdata, HANDLER_CELLID);
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
static int get_position(gpointer handle, PositionCallback positionCB, gpointer agent_userdata)
{
    GeoclueCell *geoclueCell = (GeoclueCell *) handle;
    LS_LOG_DEBUG("[DEBUG] enter in plugin callback");
    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);
    geoclueCell->pos_cb = positionCB;
    g_return_val_if_fail(geoclueCell->geoclue_pos, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueCell->geoclue_pos, "CELLDATA", agent_userdata) == TRUE) {
        LS_LOG_DEBUG("[DEBUG] cell data sent  done\n");
    } else
        return ERROR_NOT_AVAILABLE;

    geoclue_position_get_position_async(geoclueCell->geoclue_pos, (GeocluePositionCallback) position_cb_async, geoclueCell);
    return ERROR_NONE;
}

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueCell> <In> <cell plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueCell *geoclueCell)
{
    if (geoclueCell->geoclue_pos) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueCell->geoclue_pos)), G_CALLBACK(position_cb),
                                             geoclueCell);
        g_object_unref(geoclueCell->geoclue_pos);
        geoclueCell->geoclue_pos = NULL;
    }
}

/**
 * <Funciton >   intialize_cell_geoclue_service
 * <Description>  create the GPS geoclue service instance
 * @param     <GeoclueCell> <In> <cell plugin structure>
 * @return    gboolean
 */
static gboolean intialize_cell_geoclue_service(GeoclueCell *geoclueCell)
{
    g_return_val_if_fail(geoclueCell, FALSE);

    //Check its alreasy started
    if (geoclueCell->geoclue_pos) {
        return TRUE;
    }

    /*
     *  Create Geoclue position & velocity instance
     */
    geoclueCell->geoclue_pos = geoclue_position_new(LGE_CELL_SERVICE_NAME, LGE_CELL_SERVICE_PATH);

    if (geoclueCell->geoclue_pos == NULL) {
        LS_LOG_DEBUG("[DEBUG] Error while creating LG Cell geoclue object !!");
        unreference_geoclue(geoclueCell);
        return FALSE;
    }

    LS_LOG_DEBUG("[DEBUG] intialize_cell_geoclue_service  done\n");
    return TRUE;
}

/**
 * <Funciton >   start
 * <Description>  start the plugin by resgistering the callbacks
 * @param     <PositionCallback> <In> <extra commad identifier>
 * @param     <VelocityCallback> <In> <Gobject private instance>
 * @param     <SatelliteCallback> <In> <Gobject private instance>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
static int start(gpointer plugin_data, gpointer handler_data)
{
    LS_LOG_DEBUG("[DEBUG] cell plugin start  plugin_data : %d  ,handler_data :%d \n", plugin_data, handler_data);
    GeoclueCell *geoclueCell = (GeoclueCell *) plugin_data;
    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);
    geoclueCell->userdata = handler_data;

    if (intialize_cell_geoclue_service(geoclueCell) == FALSE) return ERROR_NOT_AVAILABLE;

    return ERROR_NONE;
}

/**
 * <Funciton >   start_tracking
 * <Description>  Get the position from GPS
 * @param     <handle> <In> <local Geoclue GPS handle>
 * @param     <Satellite> <In> <Satellite info result>
 * @return    int
 */
static int start_tracking(gpointer plugin_data, gboolean enableTracking, StartTrackingCallBack handler_tracking_cb,
                          gpointer celldata)
{
    GeoclueCell *geoclueCell = (GeoclueCell *) plugin_data;
    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("[DEBUG] Cell Plugin start_tracking ");
    geoclueCell->track_cb = NULL;

    if (enableTracking) {
        if (send_geoclue_command(geoclueCell->geoclue_pos, "CELLDATA", celldata) == TRUE) {
            if (send_geoclue_command(geoclueCell->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESON") == FALSE) {
                LS_LOG_DEBUG("[DEBUG] cell plugin not send\n");
                handler_tracking_cb(TRUE, NULL, NULL, ERROR_NETWORK_ERROR, geoclueCell->userdata, HANDLER_CELLID);
                return ERROR_NOT_AVAILABLE;
            }
        } else {
            return ERROR_NOT_AVAILABLE;
        }

        geoclueCell->track_cb = handler_tracking_cb;

        if (geoclueCell->track_started == FALSE) {
            g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueCell->geoclue_pos)), "position-changed", G_CALLBACK(tracking_cb),
                             plugin_data);
            geoclueCell->track_started = TRUE;
            LS_LOG_DEBUG("[DEBUG] registered position-changed signal \n");
        }

        return ERROR_NONE;
    } else {
        g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueCell->geoclue_pos)), G_CALLBACK(tracking_cb),
                                             plugin_data);
        geoclueCell->track_started = FALSE;
        return ERROR_NONE;
    }

    return ERROR_NOT_AVAILABLE;
}

/**
 * <Funciton >   stop
 * <Description>  stop the plugin
 * @param     <handle> <In> <enable/disable the callback>
 * @return    int
 */
static int stop(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]cell plugin stop\n");
    GeoclueCell *geoclueCell = (GeoclueCell *) handle;
    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);
    unreference_geoclue(geoclueCell);
    return ERROR_NONE;
}

/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <CellPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(CellPluginOps *ops)
{
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);
    LS_LOG_DEBUG("[DEBUG]cell plugin init\n");
    ops->start = start;
    ops->stop = stop;
    ops->get_position = get_position;
    ops->start_tracking = start_tracking;
    /*
     * Handler for plugin
     */
    GeoclueCell *celld = g_new0(GeoclueCell, 1);

    if (celld == NULL) return NULL;

    memset(celld, 0, sizeof(GeoclueCell));
    celld->log_handler = g_log_set_handler(NULL,
                                           G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, lsloghandler,
                                           NULL);
    return (gpointer) celld;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]cell plugin shutdown\n");
    g_return_if_fail(handle);
    GeoclueCell *geoclueCell = (GeoclueCell *) handle;
    g_log_remove_handler(NULL, geoclueCell->log_handler);
    unreference_geoclue(geoclueCell);
    g_free(geoclueCell);
}

