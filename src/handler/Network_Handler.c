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
 * Filename  : Network_Handler.c
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
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Handler_Interface.h>
#include <Network_Handler.h>
#include <Wifi_Handler.h>
#include <Cell_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>
#include <Location.h>

typedef struct _NwHandlerPrivate {
    HandlerObject *handler_obj[MAX_HANDLER_TYPE];
    PositionCallback pos_cb_arr[MAX_HANDLER_TYPE];
    PositionCallback nw_cb_arr[MAX_HANDLER_TYPE];
    StartTrackingCallBack track_cb;

} NwHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_NW, NwHandlerPrivate))

static void nw_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(NwHandler, nw_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE,
                        nw_handler_interface_init));

#define WIFI_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_NW, NwHandlerPrivate))

/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_wifi_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error,
                                 gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]NW  nw_handler_position_wifi_cb callback called  %d , type %d\n", enable_cb, type);
    NwHandlerPrivate *priv = GET_PRIVATE(privateIns);
    g_return_if_fail(priv->pos_cb_arr[type]);
    (priv->pos_cb_arr[type])(enable_cb, position, accuracy, error, priv, type); //call SA position callback
}
/**
 * <Funciton >   nw_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns,
                            int type)
{
    LS_LOG_DEBUG(
        "[DEBUG] NW Handler : tracking_cb  latitude =%f , longitude =%f , accuracy =%f\n", position->latitude,
        position->longitude, accuracy->horizAccuracy);
    NwHandlerPrivate *priv = GET_PRIVATE(privateIns);
    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    (*(priv->track_cb))(enable_cb, position, accuracy, error, priv, type);
}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_cell_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error,
                                 gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]NW  nw_handler_position_cellid_cb callback called  %d \n", enable_cb);
    NwHandlerPrivate *priv = GET_PRIVATE(privateIns);
    g_return_if_fail(priv->pos_cb_arr[type]);
    (*priv->pos_cb_arr[type])(enable_cb, position, accuracy, error, priv, type); //call SA position callback
}
/**
 * <Funciton >   wifi_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_start(Handler *handler_data, int handler_type)
{
    LS_LOG_DEBUG("[DEBUG]nw_handler_start Called handler type%d ", handler_type);
    NwHandlerPrivate *priv = GET_PRIVATE(handler_data);
    int ret = ERROR_NOT_AVAILABLE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    // This should call cell id start or Wifi start
    switch (handler_type) {
        case HANDLER_WIFI:
            if (plugin_is_supported("wifi")) {
                priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_WIFI, NULL);
                g_return_val_if_fail(priv->handler_obj[handler_type], ERROR_NOT_AVAILABLE);
                priv->nw_cb_arr[handler_type] = nw_handler_position_wifi_cb;
                ret = handler_start(HANDLER_INTERFACE(priv->handler_obj [handler_type]), handler_type);

                if (ret == ERROR_NONE)
                    LS_LOG_DEBUG("wifi_handler_start started \n");
            }

            break;

        case HANDLER_CELLID:
            if (plugin_is_supported("cell")) {
                priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_CELL, NULL);
                g_return_val_if_fail(priv->handler_obj[handler_type], ERROR_NOT_AVAILABLE);
                priv->nw_cb_arr[handler_type] = nw_handler_position_cell_cb;
                ret = handler_start(HANDLER_INTERFACE(priv->handler_obj[handler_type]), handler_type);
            }

            break;

        default:
            break;
    }

    return ret;
}

/**
 * <Funciton >   wifi_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_stop(Handler *self, int handlertype)
{
    LS_LOG_DEBUG("[DEBUG]nw_handler_stop() \n");
    NwHandlerPrivate *priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    // This should call cell id stop or Wifi stop
    ret = handler_stop(HANDLER_INTERFACE(priv->handler_obj[handlertype]), handlertype);
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int nw_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handle,
                                   int handlertype, LSHandle *sh)
{
    LS_LOG_DEBUG("[DEBUG]nw_handler_get_position\n");
    int result = ERROR_NONE;
    NwHandlerPrivate *priv = GET_PRIVATE(self);
    LS_LOG_DEBUG("[DEBUG]nw_handler_get_position hanlder type [%d]\n", handlertype);
    priv->pos_cb_arr[handlertype] = pos_cb;
    result = handler_get_position(priv->handler_obj[handlertype], enable, priv->nw_cb_arr[handlertype], self, handlertype,
                                  sh);
    LS_LOG_DEBUG("[DEBUG]result : gps_handler_get_position %d  \n", result);
    return result;
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void nw_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb,
                                      gpointer handleobj, int handlertype,
                                      LSHandle *sh)
{
    int result = ERROR_NONE;
    NwHandlerPrivate *priv = GET_PRIVATE(self);

    if (priv == NULL) track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, handlertype);

    priv->track_cb = NULL;

    if (enable) {
        priv->track_cb = track_cb;
        LS_LOG_DEBUG("[DEBUG] nw_handler_start_tracking : priv->track_cb %d \n ", priv->track_cb);
        handler_start_tracking(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb, self,
                               handlertype, sh);
    } else {
        handler_start_tracking(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb, self,
                               handlertype, sh);
    }

    LS_LOG_DEBUG("[DEBUG] return from nw_handler_start_tracking , %d  \n", result);
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int nw_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy, int handlertype)
{
    int ret = ERROR_NONE;
    NwHandlerPrivate *priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    ret = handler_get_last_position(HANDLER_INTERFACE(priv->handler_obj[handlertype]), position, accuracy, handlertype);
    return ret;
}

static int nw_handler_function_not_implemented(Handler *self, Position *pos, Address *address)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   nw_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void nw_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(nw_handler_parent_class)->dispose(gobject);
}

/**
 * <Funciton >   nw_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void nw_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG(" [DEBUG]nw_handler_finalize");
    NwHandlerPrivate *priv = GET_PRIVATE(gobject);
    int handler;

    for (handler = 0; handler < MAX_HANDLER_TYPE ; handler++) {
        if (priv->handler_obj[handler]) {
            g_object_unref(priv->handler_obj[handler]);
            priv->handler_obj[handler] = NULL;
            priv->nw_cb_arr[handler] = NULL;
            priv->pos_cb_arr[handler] = NULL;
        }
    }

    memset(priv, 0x00, sizeof(NwHandlerPrivate));
    G_OBJECT_CLASS(nw_handler_parent_class)->finalize(gobject);
}

/**
 * <Funciton >   nw_handler_interface_init
 * <Description>  nw interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void nw_handler_interface_init(HandlerInterface *interface)
{
    interface->start = (TYPE_START_FUNC) nw_handler_start;
    interface->stop = (TYPE_STOP_FUNC) nw_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) nw_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) nw_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) nw_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) nw_handler_function_not_implemented;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) nw_handler_function_not_implemented;
    interface->get_accuracy = (TYPE_GET_ACCURACY) nw_handler_function_not_implemented;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) nw_handler_function_not_implemented;
    interface->get_ttfx = (TYPE_GET_TTFF) nw_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) nw_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) nw_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) nw_handler_function_not_implemented;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) nw_handler_function_not_implemented;
    interface->set_cur_handler = (TYPE_SET_CUR_HANDLER) nw_handler_function_not_implemented;
    interface->compare_handler = (TYPE_COMP_HANDLER) nw_handler_function_not_implemented;
    interface->get_geo_code = (TYPE_GEO_CODE) nw_handler_function_not_implemented;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) nw_handler_function_not_implemented;
}

/**
 * <Funciton >   nw_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void nw_handler_init(NwHandler *self)
{
    NwHandlerPrivate *priv = GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(NwHandlerPrivate));
}

/**
 * <Funciton >   nw_handler_class_init
 * <Description>  Gobject class init
 * @param     <NwHandlerClass> <In> <WifiClass instance>
 * @return    int
 */
static void nw_handler_class_init(NwHandlerClass *klass)
{
    g_print("nw_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = nw_handler_dispose;
    gobject_class->finalize = nw_handler_finalize;
    g_type_class_add_private(klass, sizeof(NwHandlerPrivate));
}
