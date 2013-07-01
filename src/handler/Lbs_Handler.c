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
#include <LocationService_Log.h>
#include <Location.h>

typedef struct _LbsHandlerPrivate {
    LbsPlugin*  lbs_plugin;
    gboolean is_started;
    Position* pos;

    GeoCodeCallback geo_cb;
    RevGeocodeCallback rev_geo_cb;
    guint log_handler;
    gint api_progress_flag;
} LbsHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_LBS, LbsHandlerPrivate))

static void lbs_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(LbsHandler, lbs_handler, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (HANDLER_TYPE_INTERFACE, lbs_handler_interface_init));

#define LBS_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_LBS, LbsHandlerPrivate))




void geocode_handler_cb (gboolean enable_cb,Position* position,  Accuracy *accuracy , gpointer userdata)
{
    LbsHandlerPrivate* priv = GET_PRIVATE(userdata);

    g_return_if_fail(priv);
    g_return_if_fail(priv->rev_geo_cb);
    //(*(priv->rev_geo_cb)) (enable_cb ,position,accuracy, userdata);
    priv->api_progress_flag &= ~LBS_GEOCODE_ON;
}





/**
 * <Funciton >   lbs_handler_start
 * <Description>  start the Lbs  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int lbs_handler_start(Handler *handler_data ) {
    int ret = ERROR_NONE;

    LS_LOG_DEBUG("[DEBUG] lbs_handler_start");
    LbsHandlerPrivate* priv = GET_PRIVATE(handler_data);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if(priv->is_started == TRUE)
        return ERROR_NONE;

    g_return_val_if_fail(priv->lbs_plugin, ERROR_NOT_AVAILABLE);

    g_return_val_if_fail(priv->lbs_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->lbs_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    ret = priv->lbs_plugin->ops.start(priv->lbs_plugin->plugin_handler, handler_data);
    if (ret == ERROR_NONE)
        priv->is_started = TRUE;
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

    LbsHandlerPrivate* priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("[DEBUG]lbs_handler_stop() api progress %d\n", priv->api_progress_flag);

    if(priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;


    if ( priv->is_started == TRUE) {
        g_return_val_if_fail(priv->lbs_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->lbs_plugin->ops.stop(priv->lbs_plugin->plugin_handler);
        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from Lbs receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int lbs_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}


/**
 * <Funciton >   handler_stop
 * <Description>  get the position from Lbs receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int lbs_handler_start_tracking(Handler *self, gboolean enable ,StartTrackingCallBack track_cb)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the Lbs handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int lbs_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_velocity
 * <Description>  get the velocity value from  the Lbs receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <VelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int lbs_handler_get_velocity(Handler *self, VelocityCallback vel_cb )
{



    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_last_velocity
 * <Description>  get the last velocity from the Lbs receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int lbs_handler_get_last_velocity(Handler *self, Velocity *velocity, Accuracy *accuracy  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;



    return ret;
}

/**
 * <Funciton >   lbs_handler_get_accuracy
 * <Description>  get the accuracy details of the Lbs handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <AccuracyCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int lbs_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_power_requirement
 * <Description>  get the power requirement of the Lbs handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <power> <In> <power requireemnt of the Lbs handler>
 * @return    int
 */
static int lbs_handler_get_power_requirement(Handler *self, int* power  )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_time_to_first_fix
 * <Description>  get the Lbs TTFF value of Lbs
 * @param     <self> <In> <Handler Gobject>
 * @param     <ttff> <In> <resuting TTFF value>
 * @return    int
 */
static int lbs_handler_get_time_to_first_fix(Handler *self,double* ttff  )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_lbs_satellite_data
 * <Description>  get the satellite data by  Lbs by given callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <SatelliteCallback> <In> <callabck function to get the result>
 * @return    int
 */
static int lbs_handler_get_lbs_satellite_data(Handler *self , gboolean enable_satellite, SatelliteCallback sat_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_get_nmea_data
 * <Description>  get the nmea data from Lbs by the given callabck
 * @param     <self> <In> <Handler Gobject>
 * @param     <NmeaCallback> <In> <callback function to get result>
 * @return    int
 */
static int lbs_handler_get_nmea_data(Handler *self , gboolean enable_nmea ,NmeaCallback nmea_cb , gpointer agent_userdata)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_send_extra_command
 * <Description>  send the extra command
 * @param     <self> <In> <Handler Gobject>
 * @param     <command> <In> <Handler Gobject>
 *  * @param     <ExtraCmdCallback> <In> <callabck function to get result>
 * @return    int
 */
static int lbs_handler_send_extra_command(Handler *self ,int* command,ExtraCmdCallback extra_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_set_property
 * <Description>  set the proeprty value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> < Gobject>
 * @param     <property_id> <In> <property key >
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int lbs_handler_set_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the property value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> <Handler Gobject>
 * @param     <property_id> <In> <property id to change>
 * @param     <value> <In> <value to set>
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int lbs_handler_get_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get  the current handler used in Hrbrid
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of current handler used>
 * @return    int
 */
static int lbs_handler_get_current_handler(Handler *self ,int handlerType )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   lbs_handler_set_current_handler
 * <Description>  set the current handler to Hybrid handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of handler to set>
 * @return    int
 */
static int lbs_handler_set_current_handler(Handler *self ,int handlerType )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_compare_handler
 * <Description>  compare the handler1 & handler 2 and find the best accurate result
 * @param     <self> <In> <Handler Gobject>
 * @param     <handler1> <In> <id of handler1>
 * @param     <handler2> <In> <id of handler2>
 * @return    int
 */
static int lbs_handler_compare_handler(Handler *self ,int handler1 ,int handler2 )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   lbs_handler_geo_code
 * <Description>  Convert the given address to Latitide & longitude
 * @param     <Address> <In> <address to geocodet>
 * @param     <GeoCodeCallback> <In> <callback function to get result>
 * @return    int
 */
static int lbs_handler_geo_code(Handler *self ,Address* address ,GeoCodeCallback geo_cb )
{
    LbsHandlerPrivate* priv = GET_PRIVATE(self);
    int result = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if(priv->api_progress_flag & LBS_GEOCODE_ON)
        return ERROR_DUPLICATE_REQUEST;
    g_return_val_if_fail(geo_cb, ERROR_NOT_AVAILABLE);
    priv->geo_cb= geo_cb;
    g_return_val_if_fail(priv->lbs_plugin->ops.get_geocode, ERROR_NOT_AVAILABLE);

    result = priv->lbs_plugin->ops.get_geocode(priv->lbs_plugin->plugin_handler, address, geocode_handler_cb, self);
    if (result == ERROR_NONE)
        priv->api_progress_flag |= LBS_GEOCODE_ON;

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
static int lbs_handler_reverse_geo_code(Handler *self ,Position* pos,RevGeocodeCallback rev_geo_callback )
{

    return ERROR_NONE;
}


/**
 * <Funciton >   lbs_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void lbs_handler_dispose(GObject *gobject)
{

    G_OBJECT_CLASS (lbs_handler_parent_class)->dispose (gobject);
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
    LbsHandlerPrivate* priv = GET_PRIVATE(gobject);
    g_return_if_fail(priv);
    plugin_free(priv->lbs_plugin, "Lbs");
    priv->lbs_plugin = NULL;


    g_log_remove_handler(NULL, priv->log_handler);


    G_OBJECT_CLASS (lbs_handler_parent_class)->finalize (gobject);
}


/**
 * <Funciton >   lbs_handler_interface_init
 * <Description>  Lbs interface init,map all th local function to Lbs interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void lbs_handler_interface_init(HandlerInterface *interface) {
    interface->start = (TYPE_START_FUNC) lbs_handler_start;
    interface->stop = (TYPE_STOP_FUNC) lbs_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) lbs_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) lbs_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) lbs_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) lbs_handler_get_velocity;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) lbs_handler_get_last_velocity;
    interface->get_accuracy = (TYPE_GET_ACCURACY) lbs_handler_get_accuracy;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) lbs_handler_get_power_requirement;
    interface->get_ttfx = (TYPE_GET_TTFF) lbs_handler_get_time_to_first_fix;
    interface->get_sat_data = (TYPE_GET_SAT) lbs_handler_get_lbs_satellite_data;
    interface->get_nmea_data = (TYPE_GET_NMEA) lbs_handler_get_nmea_data;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) lbs_handler_send_extra_command;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) lbs_handler_get_current_handler;
    interface->set_cur_handler= (TYPE_SET_CUR_HANDLER) lbs_handler_set_current_handler;
    interface->compare_handler= (TYPE_COMP_HANDLER) lbs_handler_compare_handler;
    interface->get_geo_code= (TYPE_GEO_CODE) lbs_handler_geo_code;
    interface->get_rev_geocode= (TYPE_REV_GEO_CODE) lbs_handler_reverse_geo_code;
}


/**
 * <Funciton >   lbs_handler_init
 * <Description>  Lbs handler init Gobject function
 * @param     <self> <In> <Lbs Handler Gobject>
 * @return    int
 */
static void lbs_handler_init(LbsHandler *self)
{
    LbsHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(priv));
    priv->lbs_plugin= (LbsPlugin*)plugin_new("Lbs");
    if(priv->lbs_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG] Lbs plugin loading failed\n");
    }
    priv->log_handler = g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL
            | G_LOG_FLAG_RECURSION, lsloghandler, NULL);

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
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = lbs_handler_dispose;
    gobject_class->finalize = lbs_handler_finalize;

    g_type_class_add_private(klass, sizeof(LbsHandlerPrivate));
}
