/********************************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group   : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Wifi_Handler.c
 * Purpose  : Provides GPS related location services to Location Service Agent
 * Platform  : RedRose
 * Author(s)  : Abhishek Srivastava
 * Email ID.  :abhishek.srivastava@lge.com
 * Creation Date : 12-04-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <Handler_Interface.h>
#include <Wifi_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>

typedef struct _WifiHandlerPrivate {
    WifiPlugin *wifi_plugin;
    gboolean is_started;
    PositionCallback pos_cb;

    StartTrackingCallBack track_cb;
    gpointer nwhandler;
    gint api_progress_flag;
    guint log_handler;
    GMutex *mutex;
} WifiHandlerPrivate;

#define WIFI_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_WIFI, WifiHandlerPrivate))

static void wifi_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(WifiHandler, wifi_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE,
                        wifi_handler_interface_init));

/**
 * <Funciton >   wifi_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void wifi_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error,
                              gpointer privateIns, int type)
{
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(privateIns);
    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    //Return pos is NULL 25/10/2013 [START]
    g_return_if_fail(position);
    g_return_if_fail(accuracy);
    //Return pos is NULL 25/10/2013 [END]
    (*(priv->track_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);
}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void wifi_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error,
                              gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]wifi  haposition_cb callback called  %d \n", enable_cb);
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(privateIns);
    g_return_if_fail(priv->pos_cb);
    (*priv->pos_cb)(enable_cb, position, accuracy, error, priv->nwhandler, type); //call SA position callback
}
/**
 * <Funciton >   wifi_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int wifi_handler_start(Handler *handler_data)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_start Called\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->plugin_handler, ERROR_NOT_AVAILABLE);
    g_mutex_lock(priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(priv->mutex);
        return ERROR_NONE;
    }

    ret = priv->wifi_plugin->ops.start(priv->wifi_plugin->plugin_handler, handler_data);

    if (ret == ERROR_NONE) priv->is_started = TRUE;

    g_mutex_unlock(priv->mutex);
    return ret;
}

/**o
 * <Funciton >   wifi_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int wifi_handler_stop(Handler *self, int handler_type, gboolean forcestop)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_stop() \n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE) priv->api_progress_flag = 0;

    if (priv->api_progress_flag != 0) return ERROR_REQUEST_INPROGRESS;

    if (priv->is_started == TRUE) {
        g_return_val_if_fail(priv->wifi_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->wifi_plugin->ops.stop(priv->wifi_plugin->plugin_handler);

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from wifi receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int wifi_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handler)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_get_position\n");
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable) {
        if (priv->api_progress_flag & WIFI_GET_POSITION_ON) return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(pos_cb, ERROR_NOT_AVAILABLE);
        priv->pos_cb = pos_cb;
        priv->nwhandler = handler;
        g_return_val_if_fail(priv->wifi_plugin->ops.get_position, ERROR_NOT_AVAILABLE);
        LS_LOG_DEBUG("[DEBUG]wifi_handler_get_position value of newof hanlder %u\n", priv->nwhandler);
        result = priv->wifi_plugin->ops.get_position(priv->wifi_plugin->plugin_handler, wifi_handler_position_cb);

        if (result == ERROR_NONE) priv->api_progress_flag |= WIFI_GET_POSITION_ON;

        LS_LOG_DEBUG("[DEBUG]result : wifi_handler_get_position %d  \n", result);
    } else {
        // Clear the Flag
        priv->api_progress_flag &= ~WIFI_GET_POSITION_ON;
        LS_LOG_DEBUG("[DEBUG]result : gps_handler_get_position Clearing Position\n");
    }

    return result;
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void wifi_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb,
                                        gpointer handlerobj)
{
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    if (priv == NULL ||
            (priv->wifi_plugin->ops.start_tracking) == NULL) track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_WIFI);

    priv->track_cb = NULL;

    if (enable) {
        if (priv->api_progress_flag & WIFI_START_TRACKING_ON) return;

        priv->track_cb = track_cb;
        priv->nwhandler = handlerobj;
        result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler, enable, wifi_handler_tracking_cb);

        if (result == ERROR_NONE) priv->api_progress_flag |= WIFI_START_TRACKING_ON;
        else track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_WIFI);
    } else {
        priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler, enable, wifi_handler_tracking_cb);
        priv->api_progress_flag &= ~WIFI_START_TRACKING_ON;
    }

    LS_LOG_DEBUG("[DEBUG] return from wifi_handler_start_tracking , %d  \n", result);
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int wifi_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy)
{
    if (get_stored_position(position, accuracy, LOCATION_DB_PREF_PATH_WIFI) == ERROR_NOT_AVAILABLE) {
        LS_LOG_DEBUG("get last wifi_handler_get_last_position Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

/**
 * <Funciton >   wifi_handler_reverse_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int wifi_handler_function_not_implemented(Handler *self, Position *pos, Address *address)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   wifi_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void wifi_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(wifi_handler_parent_class)->dispose(gobject);
}

/**
 * <Funciton >   wifi_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void wifi_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_finalize\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(gobject);

    if (priv->wifi_plugin != NULL) {
        plugin_free(priv->wifi_plugin, "wifi");
        priv->wifi_plugin = NULL;
    }

    if (priv->mutex != NULL) {
        g_mutex_free(priv->mutex);
    }

    g_log_remove_handler(NULL, priv->log_handler);
    memset(priv, 0x00, sizeof(WifiHandlerPrivate));
    G_OBJECT_CLASS(wifi_handler_parent_class)->finalize(gobject);
}

/**
 * <Funciton >   wifi_handler_interface_init
 * <Description>  wifi interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void wifi_handler_interface_init(HandlerInterface *interface)
{
    interface->start = (TYPE_START_FUNC) wifi_handler_start;
    interface->stop = (TYPE_STOP_FUNC) wifi_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) wifi_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) wifi_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) wifi_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) wifi_handler_function_not_implemented;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) wifi_handler_function_not_implemented;
    interface->get_accuracy = (TYPE_GET_ACCURACY) wifi_handler_function_not_implemented;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) wifi_handler_function_not_implemented;
    interface->get_ttfx = (TYPE_GET_TTFF) wifi_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) wifi_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) wifi_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) wifi_handler_function_not_implemented;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) wifi_handler_function_not_implemented;
    interface->set_cur_handler = (TYPE_SET_CUR_HANDLER) wifi_handler_function_not_implemented;
    interface->compare_handler = (TYPE_COMP_HANDLER) wifi_handler_function_not_implemented;
    interface->get_geo_code = (TYPE_GEO_CODE) wifi_handler_function_not_implemented;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) wifi_handler_function_not_implemented;
}

/**
 * <Funciton >   wifi_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void wifi_handler_init(WifiHandler *self)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_init\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);
    memset(priv, 0x00, sizeof(WifiHandlerPrivate));
    priv->wifi_plugin = (WifiPlugin *) plugin_new("wifi");

    if (priv->wifi_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG]wifi plugin loading failed\n");
    }

    priv->mutex = g_mutex_new();

    if (priv->mutex == NULL) LS_LOG_ERROR("Failed in allocating mutex\n");

    priv->log_handler = g_log_set_handler(NULL, G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                                          lsloghandler, NULL);
}

/**
 * <Funciton >   wifi_handler_class_init
 * <Description>  Gobject class init
 * @param     <WifiHandlerClass> <In> <WifiClass instance>
 * @return    int
 */
static void wifi_handler_class_init(WifiHandlerClass *klass)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = wifi_handler_dispose;
    gobject_class->finalize = wifi_handler_finalize;
    g_type_class_add_private(klass, sizeof(WifiHandlerPrivate));
}
