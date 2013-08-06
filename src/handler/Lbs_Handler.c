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
 * Filename  : Lbs_Handler.c
 * Purpose  : Provides Lbs related location services to Location Service Agent
 * Platform  : RedRose
 * Author(s)  :  Abhishek Srivastava
 * Email ID.  : abhishek.srivastava@lge.com
 * Creation Date : 12-06-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Handler_Interface.h>
#include <Lbs_Handler.h>
#include <Plugin_Loader.h>
#include <Location_Plugin.h>
#include <Address.h>
#include <LocationService_Log.h>
#include <Location.h>

typedef struct _LbsHandlerPrivate {
    LbsPlugin *lbs_plugin;
    gboolean is_started;
    guint log_handler;
    gint api_progress_flag;
    GMutex *mutex;
} LbsHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_LBS, LbsHandlerPrivate))

static void lbs_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(LbsHandler, lbs_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE,
                        lbs_handler_interface_init));

#define LBS_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_LBS, LbsHandlerPrivate))

/**
 * <Funciton >   lbs_handler_start
 * <Description>  start the Lbs  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_start(Handler *handler_data)
{
    int ret = ERROR_NONE;
    LS_LOG_DEBUG("[DEBUG] lbs_handler_start");
    LbsHandlerPrivate *priv = GET_PRIVATE(handler_data);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_mutex_lock(priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(priv->mutex);
        return ERROR_NONE;
    }

    g_return_val_if_fail(priv->lbs_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->plugin_handler, ERROR_NOT_AVAILABLE);
    ret = priv->lbs_plugin->ops.start(priv->lbs_plugin->plugin_handler, handler_data);

    if (ret == ERROR_NONE) priv->is_started = TRUE;

    g_mutex_unlock(priv->mutex);
    return ret;
}

/**o
 * <Funciton >   lbs_handler_get_last_position
 * <Description>  stop the Lbs handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_stop(Handler *self)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("[DEBUG]lbs_handler_stop() api progress %d\n", priv->api_progress_flag);

    if (priv->api_progress_flag != 0) return ERROR_REQUEST_INPROGRESS;

    if (priv->is_started == TRUE) {
        g_return_val_if_fail(priv->lbs_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->lbs_plugin->ops.stop(priv->lbs_plugin->plugin_handler);

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

/**
 * <Funciton >   lbs_handler_geo_code
 * <Description>  Convert the given address to Latitide & longitude
 * @param     <Address> <In> <address to geocodet>
 * @param     <GeoCodeCallback> <In> <callback function to get result>
 * @return    int
 */
static int lbs_handler_geo_code(Handler *self, Address *address, Position *pos, Accuracy *ac)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int result = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (priv->api_progress_flag & LBS_GEOCODE_ON) return ERROR_DUPLICATE_REQUEST;

    if (address->freeform == FALSE) {
        g_return_val_if_fail(priv->lbs_plugin->ops.get_geocode, ERROR_NOT_AVAILABLE);
        result = priv->lbs_plugin->ops.get_geocode(priv->lbs_plugin->plugin_handler, address, pos,
                 ac); //geocode_handler_cb, self);
    } else {
        g_return_val_if_fail(priv->lbs_plugin->ops.get_geocode_freetext, ERROR_NOT_AVAILABLE);
        result = priv->lbs_plugin->ops.get_geocode_freetext(priv->lbs_plugin->plugin_handler, address->freeformaddr, pos,
                 ac); //geocode_handler_cb, self);
    }

    LS_LOG_DEBUG("[DEBUG]result : lbs_handler_geo_code %d  \n", result);
    return result;
}

/**
 * <Funciton >   lbs_handler_reverse_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_reverse_geo_code(Handler *self, Position *pos, Address *address)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int result = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->ops.get_reverse_geocode, ERROR_NOT_AVAILABLE);
    result = priv->lbs_plugin->ops.get_reverse_geocode(priv->lbs_plugin->plugin_handler, pos,
             address); //reverse_geocode_handler_cb, self);
    LS_LOG_DEBUG("[DEBUG]result : lbs_handler_geo_code %d  \n", result);
    return result;
}

/**
 * <Funciton >   lbs_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void lbs_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(lbs_handler_parent_class)->dispose(gobject);
}

/**
 * <Funciton >   lbs_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void lbs_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG] lbs_handler_finalize");
    LbsHandlerPrivate *priv = GET_PRIVATE(gobject);
    g_return_if_fail(priv);

    if (priv->lbs_plugin != NULL) {
        plugin_free(priv->lbs_plugin, "Lbs");
        priv->lbs_plugin = NULL;
    }

    if (priv->mutex != NULL) {
        g_mutex_free(priv->mutex);
    }

    g_log_remove_handler(NULL, priv->log_handler);
    G_OBJECT_CLASS(lbs_handler_parent_class)->finalize(gobject);
}

static int lbs_handler_function_not_implemented(Handler *self, Position *pos, Address *address)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_interface_init
 * <Description>  Lbs interface init,map all th local function to Lbs interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void lbs_handler_interface_init(HandlerInterface *interface)
{
    interface->start = (TYPE_START_FUNC) lbs_handler_start;
    interface->stop = (TYPE_STOP_FUNC) lbs_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) lbs_handler_function_not_implemented;
    interface->start_tracking = (TYPE_START_TRACK) lbs_handler_function_not_implemented;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) lbs_handler_function_not_implemented;
    interface->get_velocity = (TYPE_GET_VELOCITY) lbs_handler_function_not_implemented;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) lbs_handler_function_not_implemented;
    interface->get_accuracy = (TYPE_GET_ACCURACY) lbs_handler_function_not_implemented;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) lbs_handler_function_not_implemented;
    interface->get_ttfx = (TYPE_GET_TTFF) lbs_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) lbs_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) lbs_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) lbs_handler_function_not_implemented;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) lbs_handler_function_not_implemented;
    interface->set_cur_handler = (TYPE_SET_CUR_HANDLER) lbs_handler_function_not_implemented;
    interface->compare_handler = (TYPE_COMP_HANDLER) lbs_handler_function_not_implemented;
    interface->get_geo_code = (TYPE_GEO_CODE) lbs_handler_geo_code;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) lbs_handler_reverse_geo_code;
}

/**
 * <Funciton >   lbs_handler_init
 * <Description>  Lbs handler init Gobject function
 * @param     <self> <In> <Lbs Handler Gobject>
 * @return    int
 */
static void lbs_handler_init(LbsHandler *self)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(LbsHandlerPrivate));
    priv->lbs_plugin = (LbsPlugin *) plugin_new("lbs");

    if (priv->lbs_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG] Lbs plugin loading failed\n");
    }

    priv->mutex = g_mutex_new();

    if (priv->mutex == NULL) LS_LOG_ERROR("Failed in allocating mutex\n");

    priv->log_handler = g_log_set_handler(NULL, G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                                          lsloghandler, NULL);
}

/**
 * <Funciton >   lbs_handler_class_init
 * <Description>  Gobject class init
 * @param     <LbsHandlerClass> <In> <LbsClass instance>
 * @return    int
 */
static void lbs_handler_class_init(LbsHandlerClass *klass)
{
    LS_LOG_DEBUG("[DEBUG] lbs_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = lbs_handler_dispose;
    gobject_class->finalize = lbs_handler_finalize;
    g_type_class_add_private(klass, sizeof(LbsHandlerPrivate));
}
