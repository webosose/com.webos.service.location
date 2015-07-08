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
#include <Gps_stored_data.h>
#include <loc_log.h>
#include <Location.h>
typedef struct _NwHandlerPrivate NwHandlerPrivate;
struct _NwHandlerPrivate {
    HandlerObject *handler_obj[MAX_HANDLER_TYPE];
    PositionCallback pos_cb_arr[MAX_HANDLER_TYPE];
    PositionCallback nw_cb_arr[MAX_HANDLER_TYPE];
    StartTrackingCallBack track_cb;
    StartTrackingCallBack track_get_loc_update_cb;
};
static void nw_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(NwHandler, nw_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, nw_handler_interface_init));

#define NW_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_NW, NwHandlerPrivate))
static void intialize_nw_handler(Handler *handler_data, int handler_type);
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_wifi_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_INFO("[DEBUG]NW  nw_handler_position_wifi_cb callback called  %d , type %d\n", enable_cb, type);
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(privateIns);

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
void nw_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    if (priv->track_cb)
        (*(priv->track_cb))(enable_cb, position, accuracy, error, priv, type);

    if (priv->track_get_loc_update_cb != NULL)
        (*(priv->track_get_loc_update_cb))(enable_cb, position, accuracy, error, priv, type);
}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_cell_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_INFO("[DEBUG]NW  nw_handler_position_cellid_cb callback called  %d \n", enable_cb);
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb_arr[type]);

    (*priv->pos_cb_arr[type])(enable_cb, position, accuracy, error, priv, type); //call SA position callback
}
/**
 * <Funciton >   wifi_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_start(Handler *handler_data, int handler_type, const char* license_key)
{
    LS_LOG_INFO("[DEBUG]nw_handler_start Called handler type%d ", handler_type);
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(handler_data);
    int ret = ERROR_NOT_AVAILABLE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    // This should call cell id start or Wifi start
    switch (handler_type) {
        case HANDLER_WIFI:
            intialize_nw_handler(handler_data, handler_type);
            g_return_val_if_fail(priv->handler_obj[handler_type], ERROR_NOT_AVAILABLE);
            priv->nw_cb_arr[handler_type] = nw_handler_position_wifi_cb;
            ret = handler_start(HANDLER_INTERFACE(priv->handler_obj [handler_type]), handler_type, license_key);

            if (ret == ERROR_NONE)
                LS_LOG_INFO("wifi_handler_start started \n");

            break;

        case HANDLER_CELLID:
            intialize_nw_handler(handler_data, handler_type);
            g_return_val_if_fail(priv->handler_obj[handler_type], ERROR_NOT_AVAILABLE);
            priv->nw_cb_arr[handler_type] = nw_handler_position_cell_cb;
            ret = handler_start(HANDLER_INTERFACE(priv->handler_obj[handler_type]), handler_type, license_key);
            break;

        default:
            break;
    }

    return ret;
}

static void intialize_nw_handler(Handler *handler_data, int handler_type)
{
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(handler_data);

    switch (handler_type) {
        case HANDLER_WIFI:
            if (plugin_is_supported("wifi")) {
                if (!priv->handler_obj[handler_type])
                    priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_WIFI, NULL);
            }

            break;

        case HANDLER_CELLID:
            if (plugin_is_supported("cell")) {
                if (!priv->handler_obj[handler_type])
                    priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_CELL, NULL);
            }

            break;
    }
}
/**
 * <Funciton >   wifi_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_stop(Handler *self, int handlertype, gboolean forcestop)
{
    LS_LOG_INFO("[DEBUG]nw_handler_stop() \n");
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    // This should call cell id stop or Wifi stop
    if (priv->handler_obj[handlertype] != NULL)
        ret = handler_stop(HANDLER_INTERFACE(priv->handler_obj[handlertype]), handlertype, forcestop);

    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int nw_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handle, int handlertype, LSHandle *sh)
{
    int result = ERROR_NONE;
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("[DEBUG]nw_handler_get_position hanlder type [%d]\n", handlertype);
    priv->pos_cb_arr[handlertype] = pos_cb;

    g_return_val_if_fail(priv->handler_obj[handlertype], ERROR_NOT_AVAILABLE);

    result = handler_get_position(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, priv->nw_cb_arr[handlertype], self, handlertype, sh);

    return result;
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void nw_handler_start_tracking(Handler *self,
                                      gboolean enable,
                                      StartTrackingCallBack track_cb,
                                      gpointer handleobj,
                                      int handlertype,
                                      LSHandle *sh)
{
    int result = ERROR_NONE;
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);

    if (priv == NULL) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, handlertype);
        return;
    }

    priv->track_cb = NULL;

    if (enable) {
        priv->track_cb = track_cb;
    }
    handler_start_tracking(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb, self, handlertype, sh);
    LS_LOG_INFO("[DEBUG] return from nw_handler_start_tracking , %d  \n", result);
}

static gboolean nw_handler_get_handler_status(Handler *self, int handlertype)
{
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);
    int ret = 0;

    g_return_val_if_fail(priv, 0);

    // This should call cell id stop or Wifi stop
    if (priv->handler_obj[handlertype] != NULL)
        ret = handler_get_handler_status(HANDLER_INTERFACE(priv->handler_obj[handlertype]), handlertype);

    return ret;
}
static void nw_handler_get_location_updates(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer handleobj, int handlertype,
        LSHandle *sh)
{
    int result = ERROR_NONE;
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);

    if (priv == NULL) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, handlertype);
        return;
    }

    priv->track_get_loc_update_cb = NULL;

    if (enable) {
        priv->track_get_loc_update_cb = track_cb;
    }
    handler_get_location_updates(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb, self, handlertype, sh);
    if(enable)
        LS_LOG_INFO("nw_handler_get_location_updates enable, result %d", result);
    else
        LS_LOG_INFO("nw_handler_get_location_updates disable, result %d", result);
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
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    intialize_nw_handler(self, handlertype);
    ret = handler_get_last_position(HANDLER_INTERFACE(priv->handler_obj[handlertype]), position, accuracy, handlertype);

    return ret;
}

static int nw_handler_function_not_implemented(Handler *self, Position *pos, GeoCodeCallback geocode_cb)
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
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(gobject);
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
    interface->get_ttfx = (TYPE_GET_TTFF) nw_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) nw_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) nw_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) nw_handler_function_not_implemented;
#ifdef NOMINATIUM_LBS
    interface->get_geo_code = (TYPE_GEO_CODE) nw_handler_function_not_implemented;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) nw_handler_function_not_implemented;
#else
    interface->get_google_geo_code = (TYPE_GOOGLE_GEO_CODE) nw_handler_function_not_implemented;
    interface->get_rev_google_geocode = (TYPE_REV_GOOGLE_GEO_CODE) nw_handler_function_not_implemented;
#endif
    interface->add_geofence_area = (TYPE_ADD_GEOFENCE_AREA) nw_handler_function_not_implemented;
    interface->remove_geofence = (TYPE_REMOVE_GEOFENCE) nw_handler_function_not_implemented;
    interface->resume_geofence = (TYPE_RESUME_GEOFENCE) nw_handler_function_not_implemented;
    interface->pause_geofence = (TYPE_PAUSE_GEOFENCE) nw_handler_function_not_implemented;
    interface->get_location_updates = (TYPE_GET_LOCATION_UPDATES) nw_handler_get_location_updates;
    interface->get_handler_status = (TYPE_GET_HANDLER_STATUS) nw_handler_get_handler_status;
}

/**
 * <Funciton >   nw_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void nw_handler_init(NwHandler *self)
{
    NwHandlerPrivate *priv = NW_HANDLER_GET_PRIVATE(self);
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
    LS_LOG_INFO("nw_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = nw_handler_dispose;
    gobject_class->finalize = nw_handler_finalize;

    g_type_class_add_private(klass, sizeof(NwHandlerPrivate));
}
