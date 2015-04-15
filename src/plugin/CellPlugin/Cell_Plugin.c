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
#include <geoclue/geoclue-position.h>
#include <geoclue/geoclue-velocity.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <loc_log.h>

/*All g-signals indexes*/
enum {
    SIGNAL_TRACKING,
    SIGNAL_END
};

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
    char *license_key;
    gulong signal_handler_ids[SIGNAL_END];
} GeoclueCell;

#define LGE_CELL_SERVICE_NAME "org.freedesktop.Geoclue.Providers.LgeCellService"
#define LGE_CELL_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/LgeCellService"

static void cell_position_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer userdata);
static void tracking_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer plugin_data);

static void signal_connection(GeoclueCell *geoclueCell, int signal_type, gboolean connect)
{
    gpointer signal_instance = NULL;
    gchar *signal_name = NULL;
    GCallback signal_cb = NULL;

    if (geoclueCell == NULL)
        return;

    if (signal_type < 0 || signal_type >= SIGNAL_END)
        return;

    switch (signal_type) {
        case SIGNAL_TRACKING:
            signal_instance = G_OBJECT(geoclueCell->geoclue_pos);
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
                                           geoclueCell->signal_handler_ids[signal_type])) {
            geoclueCell->signal_handler_ids[signal_type] = g_signal_connect(signal_instance,
                                                                            signal_name,
                                                                            signal_cb,
                                                                            geoclueCell);
            LS_LOG_INFO("Connecting singal [%d:%s] %d\n",
                        signal_type,
                        signal_name,
                        geoclueCell->signal_handler_ids[signal_type]);
        } else {
            LS_LOG_INFO("Already connected signal [%d:%s] %d\n",
                        signal_type,
                        signal_name,
                        geoclueCell->signal_handler_ids[signal_type]);
        }
    } else {
        g_signal_handler_disconnect(signal_instance,
                                    geoclueCell->signal_handler_ids[signal_type]);
        LS_LOG_INFO("Disconnecting singal [%d:%s] %d\n",
                    signal_type,
                    signal_name,
                    geoclueCell->signal_handler_ids[signal_type]);

        geoclueCell->signal_handler_ids[signal_type] = 0;
    }
}

void set_options_cb(GeoclueProvider *provider, GError *error, gpointer userdata) {
    if (error != NULL) {
        LS_LOG_ERROR("set_options_cb called error %s",error->message);
        g_error_free(error);
    } else {
        LS_LOG_DEBUG("set_options_cb called success");
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
    GValue *gvalue = NULL;

    options = g_hash_table_new(g_str_hash, g_str_equal);
    gvalue = g_new0(GValue, 1);

    g_value_init(gvalue, G_TYPE_STRING);
    g_value_set_string(gvalue, value);
    g_hash_table_insert(options, key, gvalue);

    geoclue_provider_set_options_async(GEOCLUE_PROVIDER(instance), options, set_options_cb, instance);
    g_value_unset(gvalue);
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
    GeoclueCell *plugin_data = (GeoclueCell *) userdata;

    g_return_if_fail(plugin_data);

    if (error) {
        LS_LOG_ERROR("position_cb_async error %s",error->message);
        g_error_free(error);
        (*plugin_data->pos_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, plugin_data->userdata, HANDLER_CELLID);
    } else {
        cell_position_cb(position, fields, timestamp, latitude, longitude, altitude, accuracy, userdata);
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
static void cell_position_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer userdata)
{
    LS_LOG_INFO("[DEBUG] Cell Plugin CC: position_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueCell *plugin_data = (GeoclueCell *) userdata;

    g_return_if_fail(plugin_data);
    g_return_if_fail(plugin_data->pos_cb);

    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = INVALID_PARAM;
    GeoclueAccuracyLevel level;

    fields = fields | POSITION_FIELDS_ALTITUDE | VELOCITY_FIELDS_SPEED | VELOCITY_FIELDS_DIRECTION | VELOCITY_FIELDS_CLIMB;
    Position *ret_pos = position_create(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM,
                                        INVALID_PARAM, fields);
    g_return_if_fail(ret_pos);

    if (accuracy != NULL) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, INVALID_PARAM);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, INVALID_PARAM, INVALID_PARAM);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    set_store_position(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, hor_acc, INVALID_PARAM, LOCATION_DB_PREF_PATH_CELL);

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
static void tracking_cb(GeocluePosition *position,
                        GeocluePositionFields fields,
                        int64_t timestamp,
                        double latitude,
                        double longitude,
                        double altitude,
                        GeoclueAccuracy *accuracy,
                        gpointer plugin_data)
{
    LS_LOG_DEBUG("[DEBUG] Cell Plugin CC: tracking_cb  latitude =%f , longitude =%f \n", latitude, longitude);
    GeoclueCell *cellPlugin = (GeoclueCell *) plugin_data;

    g_return_if_fail(cellPlugin);
    g_return_if_fail(cellPlugin->track_cb);

    Accuracy *ac;
    double hor_acc = DEFAULT_VALUE;
    double vert_acc = INVALID_PARAM;
    GeoclueAccuracyLevel level;

    fields = fields | POSITION_FIELDS_ALTITUDE | VELOCITY_FIELDS_SPEED | VELOCITY_FIELDS_DIRECTION | VELOCITY_FIELDS_CLIMB;
    Position *ret_pos = position_create(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM,
                                        INVALID_PARAM, fields);

    g_return_if_fail(ret_pos);

    if (accuracy != NULL) {
        geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, INVALID_PARAM);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, INVALID_PARAM);
    }

    if (ac == NULL) {
        position_free(ret_pos);
        return;
    }

    set_store_position(timestamp, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM, hor_acc, INVALID_PARAM, LOCATION_DB_PREF_PATH_CELL);

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
    LS_LOG_INFO("[DEBUG] Cell Plugin CC: enter in plugin callback");

    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);

    geoclueCell->pos_cb = positionCB;

    g_return_val_if_fail(geoclueCell->geoclue_pos, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueCell->geoclue_pos, "GEOLOCATION", geoclueCell->license_key) == TRUE) {
        LS_LOG_WARNING("License key Sent Success\n");
    } else
        return ERROR_NOT_AVAILABLE;

    if (send_geoclue_command(geoclueCell->geoclue_pos, "CELLDATA", agent_userdata) == TRUE) {
        LS_LOG_WARNING("[DEBUG] cell data sent  done\n");
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
        g_signal_handlers_disconnect_by_func(G_OBJECT(geoclueCell->geoclue_pos),
                                             G_CALLBACK(tracking_cb),
                                             geoclueCell);
        g_object_unref(geoclueCell->geoclue_pos);
        geoclueCell->geoclue_pos = NULL;
    }

    memset(geoclueCell->signal_handler_ids, 0, sizeof(gulong)*SIGNAL_END);
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
        LS_LOG_ERROR("[DEBUG] Error while creating LG Cell geoclue object !!");
        unreference_geoclue(geoclueCell);
        return FALSE;
    }

    LS_LOG_INFO("[DEBUG] intialize_cell_geoclue_service  done\n");

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
static int start(gpointer plugin_data, gpointer handler_data, const char *license_key)
{
    LS_LOG_INFO("[DEBUG] cell plugin start  plugin_data : %d  ,handler_data :%d \n", plugin_data, handler_data);
    GeoclueCell *geoclueCell = (GeoclueCell *) plugin_data;
    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);

    geoclueCell->userdata = handler_data;

    if (intialize_cell_geoclue_service(geoclueCell) == FALSE)
        return ERROR_NOT_AVAILABLE;

    geoclueCell->license_key = g_strdup(license_key);

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

    LS_LOG_INFO("[DEBUG] Cell Plugin start_tracking ");
    geoclueCell->track_cb = NULL;

    if (enableTracking) {
        geoclueCell->track_cb = handler_tracking_cb;

        if (geoclueCell->track_started == FALSE) {
            signal_connection(geoclueCell, SIGNAL_TRACKING, TRUE);
            geoclueCell->track_started = TRUE;
            LS_LOG_DEBUG("[DEBUG] registered position-changed signal \n");
        }

        if (send_geoclue_command(geoclueCell->geoclue_pos, "GEOLOCATION", geoclueCell->license_key) == TRUE) {
            LS_LOG_WARNING("License key Sent Success\n");
        } else
            return ERROR_NOT_AVAILABLE;

        if (send_geoclue_command(geoclueCell->geoclue_pos, "CELLDATA", celldata) == TRUE) {

            if (send_geoclue_command(geoclueCell->geoclue_pos, "REQUESTED_STATE", "PERIODICUPDATESON") == FALSE) {
                LS_LOG_WARNING("[DEBUG] cell plugin not send\n");
                handler_tracking_cb(TRUE, NULL, NULL, ERROR_NETWORK_ERROR, geoclueCell->userdata, HANDLER_CELLID);
                return ERROR_NOT_AVAILABLE;
            }
        } else {
            signal_connection(geoclueCell, SIGNAL_TRACKING, FALSE);
            geoclueCell->track_started = FALSE;
            return ERROR_NOT_AVAILABLE;
        }

        return ERROR_NONE;
    } else {
        signal_connection(geoclueCell, SIGNAL_TRACKING, FALSE);
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
    LS_LOG_INFO("[DEBUG]cell plugin stop\n");
    GeoclueCell *geoclueCell = (GeoclueCell *) handle;

    g_return_val_if_fail(geoclueCell, ERROR_NOT_AVAILABLE);

    unreference_geoclue(geoclueCell);

    if (geoclueCell->license_key != NULL)
        g_free(geoclueCell->license_key);

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
    LS_LOG_INFO("[DEBUG]cell plugin init\n");
    ops->start = start;
    ops->stop = stop;
    ops->get_position = get_position;
    ops->start_tracking = start_tracking;
    /*
     * Handler for plugin
     */
    GeoclueCell *celld = g_new0(GeoclueCell, 1);

    if (celld == NULL)
        return NULL;

    memset(celld, 0, sizeof(GeoclueCell));

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
    LS_LOG_INFO("[DEBUG]cell plugin shutdown\n");

    g_return_if_fail(handle);

    GeoclueCell *geoclueCell = (GeoclueCell *) handle;
    unreference_geoclue(geoclueCell);

    g_free(geoclueCell);
}

