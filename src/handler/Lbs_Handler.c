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
#include <loc_log.h>
#include <Location.h>

typedef struct _LbsHandlerPrivate {
    LbsPlugin *lbs_plugin;
    gboolean is_started;
    gint api_progress_flag;
#ifndef NOMINATIUM_LBS
    GeoCodeCallback geocode_cb;
    GeoCodeCallback rev_geocode_cb;
#endif
    GMutex mutex;
} LbsHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_LBS, LbsHandlerPrivate))

static void lbs_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(LbsHandler, lbs_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, lbs_handler_interface_init));

#define LBS_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_LBS, LbsHandlerPrivate))

/**
 * <Funciton >   lbs_handler_start
 * <Description>  start the Lbs  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_start(Handler *handler_data, const char* license_key)
{
    int ret = ERROR_NONE;
    LS_LOG_INFO("[DEBUG] lbs_handler_start");
    LbsHandlerPrivate *priv = GET_PRIVATE(handler_data);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    g_mutex_lock(&priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(&priv->mutex);
        return ERROR_NONE;
    }

    ret = priv->lbs_plugin->ops.start(priv->lbs_plugin->plugin_handler, handler_data, license_key);

    if (ret == ERROR_NONE)
        priv->is_started = TRUE;

    g_mutex_unlock(&priv->mutex);

    return ret;
}

/**o
 * <Funciton >   lbs_handler_get_last_position
 * <Description>  stop the Lbs handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_stop(Handler *self, gboolean forcestop)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("[DEBUG]lbs_handler_stop() api progress %d\n", priv->api_progress_flag);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE)
        priv->api_progress_flag = 0;

    if (priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->is_started == TRUE) {

        g_return_val_if_fail(priv->lbs_plugin->ops.stop, ERROR_NOT_AVAILABLE);

        ret = priv->lbs_plugin->ops.stop(priv->lbs_plugin->plugin_handler);

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

gboolean lbs_handler_get_handler_status(Handler *self)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, 0);
    return priv->is_started;
}

#ifndef NOMINATIUM_LBS
void lbs_handler_rev_geocode_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type)
{
    LS_LOG_INFO("gps_handler_geocode_cb.");
    LbsHandlerPrivate *priv = GET_PRIVATE(userdata);
    g_return_if_fail(priv);

    priv->api_progress_flag &= ~LBS_REVGEOCODE_ON;
    priv->rev_geocode_cb (enable_cb, response, error, userdata, type);
}

void lbs_handler_geocode_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type)
{
    LS_LOG_INFO("gps_handler_geocode_cb.");
    LbsHandlerPrivate *priv = GET_PRIVATE(userdata);
    g_return_if_fail(priv);

    priv->api_progress_flag &= ~LBS_GEOCODE_ON;
    priv->geocode_cb (enable_cb, response, error, userdata, type);
}
#endif

#ifdef NOMINATIUM_LBS
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

    if (priv->api_progress_flag & LBS_GEOCODE_ON)
        return ERROR_DUPLICATE_REQUEST;

    if (address->freeform == FALSE) {

        g_return_val_if_fail(priv->lbs_plugin->ops.get_geocode, ERROR_NOT_AVAILABLE);

        result = priv->lbs_plugin->ops.get_geocode(priv->lbs_plugin->plugin_handler, address, pos, ac); //geocode_handler_cb, self);
    } else {

        g_return_val_if_fail(priv->lbs_plugin->ops.get_geocode_freetext, ERROR_NOT_AVAILABLE);

        result = priv->lbs_plugin->ops.get_geocode_freetext(priv->lbs_plugin->plugin_handler, address->freeformaddr, pos, ac); //geocode_handler_cb, self);
    }

    LS_LOG_INFO("[DEBUG]result : lbs_handler_geo_code %d  \n", result);

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

    result = priv->lbs_plugin->ops.get_reverse_geocode(priv->lbs_plugin->plugin_handler, pos, address); //reverse_geocode_handler_cb, self);

    LS_LOG_INFO("[DEBUG]result : lbs_handler_geo_code %d  \n", result);

    return result;
}
#else
/**
 * <Funciton >   lbs_handler_reverse_google_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_google_geo_code(Handler *self, const char *data, GeoCodeCallback geocode_cb)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int result = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->ops.get_google_geocode, ERROR_NOT_AVAILABLE);
    priv->geocode_cb = geocode_cb;
    result = priv->lbs_plugin->ops.get_google_geocode(priv->lbs_plugin->plugin_handler, data, lbs_handler_geocode_cb);

    if (result == ERROR_NONE)
        priv->api_progress_flag |= LBS_GEOCODE_ON;

    LS_LOG_INFO("[DEBUG]result : lbs_handler_google_geo_code %d  \n", result);

    return result;
}

/**
 * <Funciton >   lbs_handler_reverse_google_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_reverse_google_geo_code(Handler *self, const char *data, GeoCodeCallback rev_geocode_cb)
{
    LbsHandlerPrivate *priv = GET_PRIVATE(self);
    int result = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->ops.get_reverse_google_geocode, ERROR_NOT_AVAILABLE);

    priv->rev_geocode_cb = rev_geocode_cb;
    result = priv->lbs_plugin->ops.get_reverse_google_geocode(priv->lbs_plugin->plugin_handler, data, lbs_handler_rev_geocode_cb);

    if (result == ERROR_NONE)
        priv->api_progress_flag |= LBS_REVGEOCODE_ON;

    LS_LOG_INFO("[DEBUG]result : lbs_handler_reverse_google_geo_code %d  \n", result);

    return result;
}
#endif
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
#ifdef NOMINATIUM_LBS
        plugin_free(priv->lbs_plugin, "lbs");
#else
        plugin_free(priv->lbs_plugin, "googlelbs");
#endif
        priv->lbs_plugin = NULL;
    }

    g_mutex_clear(&priv->mutex);

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
    interface->start = lbs_handler_start;
    interface->stop = lbs_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) lbs_handler_function_not_implemented;
    interface->start_tracking = (TYPE_START_TRACK) lbs_handler_function_not_implemented;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) lbs_handler_function_not_implemented;
    interface->get_ttfx = (TYPE_GET_TTFF) lbs_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) lbs_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) lbs_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) lbs_handler_function_not_implemented;
#ifdef NOMINATIUM_LBS
    interface->get_geo_code = lbs_handler_geo_code;
    interface->get_rev_geocode = lbs_handler_reverse_geo_code;
#else
    interface->get_google_geo_code = lbs_handler_google_geo_code;
    interface->get_rev_google_geocode = lbs_handler_reverse_google_geo_code;
#endif
    interface->add_geofence_area = (TYPE_ADD_GEOFENCE_AREA) lbs_handler_function_not_implemented;
    interface->remove_geofence = (TYPE_REMOVE_GEOFENCE) lbs_handler_function_not_implemented;
    interface->resume_geofence = (TYPE_RESUME_GEOFENCE) lbs_handler_function_not_implemented;
    interface->pause_geofence = (TYPE_PAUSE_GEOFENCE) lbs_handler_function_not_implemented;
    interface->get_handler_status = lbs_handler_get_handler_status;
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
#ifdef NOMINATIUM_LBS
    priv->lbs_plugin = (LbsPlugin *) plugin_new("lbs");
#else
    priv->lbs_plugin = (LbsPlugin *) plugin_new("googlelbs");
#endif

    if (priv->lbs_plugin == NULL) {
        LS_LOG_WARNING("[DEBUG] Lbs plugin loading failed\n");
    }

    g_mutex_init(&priv->mutex);
}

/**
 * <Funciton >   lbs_handler_class_init
 * <Description>  Gobject class init
 * @param     <LbsHandlerClass> <In> <LbsClass instance>
 * @return    int
 */
static void lbs_handler_class_init(LbsHandlerClass *klass)
{
    LS_LOG_INFO("[DEBUG] lbs_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = lbs_handler_dispose;
    gobject_class->finalize = lbs_handler_finalize;

    g_type_class_add_private(klass, sizeof(LbsHandlerPrivate));
}
