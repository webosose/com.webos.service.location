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
    WifiPlugin*  wifi_plugin;
    gboolean is_started;
    Position* pos;
    PositionCallback pos_cb;

    StartTrackingCallBack track_cb;
    gpointer nwhandler;
    gint api_progress_flag;
} WifiHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_WIFI, WifiHandlerPrivate))

static void wifi_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(WifiHandler, wifi_handler, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (HANDLER_TYPE_INTERFACE, wifi_handler_interface_init));

#define WIFI_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_WIFI, WifiHandlerPrivate))



/**
 * <Funciton >   wifi_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void wifi_handler_tracking_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy, int error,
        gpointer privateIns, int type)
{
    LS_LOG_DEBUG("GPS Handler : tracking_cb  latitude =%f , longitude =%f , accuracy =%f\n", position->latitude,position->longitude,accuracy->horizAccuracy);
    WifiHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    (*(priv->track_cb)) (enable_cb ,position,  accuracy, error, priv->nwhandler, type);

}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void wifi_handler_position_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy,
        int error, gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]wifi  haposition_cb callback called  %d \n" , enable_cb);
    WifiHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb);
    (*priv->pos_cb) (enable_cb ,position, accuracy, error, priv->nwhandler, type );//call SA position callback



}
/**
 * <Funciton >   wifi_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int wifi_handler_start(Handler *handler_data ) {
    LS_LOG_DEBUG("[DEBUG]wifi_handler_start Called\n");
    WifiHandlerPrivate* priv = GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;


    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(priv->is_started == TRUE)
        return ERROR_NONE;
    g_return_val_if_fail(priv->wifi_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->plugin_handler, ERROR_NOT_AVAILABLE);
    ret = priv->wifi_plugin->ops.start(priv->wifi_plugin->plugin_handler, handler_data);


    if (ret == ERROR_NONE)
        priv->is_started = TRUE;
    return ret;
}

/**o
 * <Funciton >   wifi_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int wifi_handler_stop(Handler *self)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_stop() \n");
    WifiHandlerPrivate* priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;
    if ( priv->is_started == TRUE) {
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
static int wifi_handler_get_position(Handler *self,  gboolean enable, PositionCallback pos_cb, gpointer handler  )
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_get_position\n");
    int result = ERROR_NONE;
    WifiHandlerPrivate* priv = GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(enable){
        if(priv->api_progress_flag & WIFI_GET_POSITION_ON)
            return ERROR_DUPLICATE_REQUEST;
        g_return_val_if_fail(pos_cb, ERROR_NOT_AVAILABLE);
        priv->pos_cb= pos_cb;
        priv->nwhandler = handler;
        g_return_val_if_fail(priv->wifi_plugin->ops.get_position, ERROR_NOT_AVAILABLE);
        LS_LOG_DEBUG("[DEBUG]wifi_handler_get_position value of newof hanlder %u\n",priv->nwhandler );
        result = priv->wifi_plugin->ops.get_position(priv->wifi_plugin->plugin_handler, wifi_handler_position_cb);
        if (result == ERROR_NONE)
            priv->api_progress_flag |= WIFI_GET_POSITION_ON;

        LS_LOG_DEBUG("[DEBUG]result : wifi_handler_get_position %d  \n", result);
    }
    else{
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
static void wifi_handler_start_tracking(Handler *self, gboolean enable ,StartTrackingCallBack track_cb , gpointer handlerobj)
{

    int result = ERROR_NONE;
    WifiHandlerPrivate* priv = GET_PRIVATE(self);
    if(priv == NULL || (priv->wifi_plugin->ops.start_tracking) == NULL)
        track_cb(TRUE ,NULL,NULL, ERROR_NOT_AVAILABLE, NULL , HANDLER_WIFI);

    priv->track_cb =NULL;
    if(enable){
        if (priv->api_progress_flag & WIFI_START_TRACKING_ON)
            return;

        priv->track_cb = track_cb;
        priv->nwhandler= handlerobj;
        result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler, enable ,wifi_handler_tracking_cb);
        if (result == ERROR_NONE)
            priv->api_progress_flag |= WIFI_START_TRACKING_ON;
        else
            track_cb(TRUE ,NULL,NULL, ERROR_NOT_AVAILABLE, NULL , HANDLER_WIFI);
    }else{
        result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler, enable ,wifi_handler_tracking_cb);
        if (result == ERROR_NONE)
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
static int wifi_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy )
{
    int ret = ERROR_NONE;
    WifiHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(wifi_plugin_get_stored_position(position, accuracy) == ERROR_NOT_AVAILABLE)
        return ERROR_NOT_AVAILABLE;
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_velocity
 * <Description>  get the velocity value from  the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <VelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int wifi_handler_get_velocity(Handler *self, VelocityCallback vel_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_last_velocity
 * <Description>  get the last velocity from the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int wifi_handler_get_last_velocity(Handler *self, Velocity **velocity, Accuracy **accuracy  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_accuracy
 * <Description>  get the accuracy details of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <AccuracyCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int wifi_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_power_requirement
 * <Description>  get the power requirement of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <power> <In> <power requireemnt of the GPS handler>
 * @return    int
 */
static int wifi_handler_get_power_requirement(Handler *self, int* power  )
{
    int ret = 0;
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_time_to_first_fix
 * <Description>  get the wifi TTFF value of GPS
 * @param     <self> <In> <Handler Gobject>
 * @param     <ttff> <In> <resuting TTFF value>
 * @return    int
 */
static int wifi_handler_get_time_to_first_fix(Handler *self,double* ttff  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}
/**
 * <Funciton >   gps_handler_get_gps_satellite_data
 * <Description>  get the satellite data by  gps by given callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <SatelliteCallback> <In> <callabck function to get the result>
 * @return    int
 */
static int wifi_handler_get_gps_satellite_data(Handler *self , gboolean enable_satellite, SatelliteCallback sat_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}
/**
 * <Funciton >   gps_handler_get_nmea_data
 * <Description>  get the nmea data from GPS by the given callabck
 * @param     <self> <In> <Handler Gobject>
 * @param     <NmeaCallback> <In> <callback function to get result>
 * @return    int
 */
static int wifi_handler_get_nmea_data(Handler *self , gboolean enable_nmea ,NmeaCallback nmea_cb , gpointer agent_userdata)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   gps_handler_send_extra_command
 * <Description>  send the extra command
 * @param     <self> <In> <Handler Gobject>
 * @param     <command> <In> <Handler Gobject>
 *  * @param     <ExtraCmdCallback> <In> <callabck function to get result>
 * @return    int
 */
static int wifi_handler_send_extra_command(Handler *self ,int* command,ExtraCmdCallback extra_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_set_property
 * <Description>  set the proeprty value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> < Gobject>
 * @param     <property_id> <In> <property key >
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int wifi_handler_set_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
    int ret = 0;
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
static int wifi_handler_get_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
    int ret = 0;
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get  the current handler used in Hrbrid
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of current handler used>
 * @return    int
 */
static int wifi_handler_get_current_handler(Handler *self ,int handlerType )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_set_current_handler
 * <Description>  set the current handler to Hybrid handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of handler to set>
 * @return    int
 */
static int wifi_handler_set_current_handler(Handler *self ,int handlerType )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_compare_handler
 * <Description>  compare the handler1 & handler 2 and find the best accurate result
 * @param     <self> <In> <Handler Gobject>
 * @param     <handler1> <In> <id of handler1>
 * @param     <handler2> <In> <id of handler2>
 * @return    int
 */
static int wifi_handler_compare_handler(Handler *self ,int handler1 ,int handler2 )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   wifi_handler_geo_code
 * <Description>  Convert the given address to Latitide & longitude
 * @param     <Address> <In> <address to geocodet>
 * @param     <GeoCodeCallback> <In> <callback function to get result>
 * @return    int
 */
static int wifi_handler_geo_code(Handler *self ,Address* address ,GeoCodeCallback geo_cb  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
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
static int wifi_handler_reverse_geo_code(Handler *self ,Position* pos,RevGeocodeCallback rev_geo_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}


/**
 * <Funciton >   wifi_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void wifi_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS (wifi_handler_parent_class)->dispose (gobject);

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
    WifiHandlerPrivate* priv = GET_PRIVATE(gobject);

    plugin_free(priv->wifi_plugin, "wifi");
    priv->wifi_plugin = NULL;

    if (priv->pos) {
        position_free(priv->pos);
        priv->pos = NULL;
    }

    priv->pos_cb = NULL;

    priv->track_cb = NULL;
    priv->nwhandler = NULL;

    G_OBJECT_CLASS (wifi_handler_parent_class)->finalize (gobject);
}


/**
 * <Funciton >   wifi_handler_interface_init
 * <Description>  wifi interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void wifi_handler_interface_init(HandlerInterface *interface) {
    interface->start = (TYPE_START_FUNC) wifi_handler_start;
    interface->stop = (TYPE_STOP_FUNC) wifi_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) wifi_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) wifi_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) wifi_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) wifi_handler_get_velocity;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) wifi_handler_get_last_velocity;
    interface->get_accuracy = (TYPE_GET_ACCURACY) wifi_handler_get_accuracy;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) wifi_handler_get_power_requirement;
    interface->get_ttfx = (TYPE_GET_TTFF) wifi_handler_get_time_to_first_fix;
    interface->get_sat_data = (TYPE_GET_SAT) wifi_handler_get_gps_satellite_data;
    interface->get_nmea_data = (TYPE_GET_NMEA) wifi_handler_get_nmea_data;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) wifi_handler_send_extra_command;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) wifi_handler_get_current_handler;
    interface->set_cur_handler= (TYPE_SET_CUR_HANDLER) wifi_handler_set_current_handler;
    interface->compare_handler= (TYPE_COMP_HANDLER) wifi_handler_compare_handler;
    interface->get_geo_code= (TYPE_GEO_CODE) wifi_handler_geo_code;
    interface->get_rev_geocode= (TYPE_REV_GEO_CODE) wifi_handler_reverse_geo_code;
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
    WifiHandlerPrivate* priv = GET_PRIVATE(self);
    priv->wifi_plugin= (WifiPlugin*)plugin_new("wifi");
    if(priv->wifi_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG]wifi plugin loading failed\n");
    }
    priv->api_progress_flag = WIFI_PROGRESS_NONE;
    priv->is_started = FALSE;
    priv->pos = NULL;

    priv->pos_cb = NULL;

    priv->track_cb = NULL;
    priv->nwhandler = NULL;
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
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = wifi_handler_dispose;
    gobject_class->finalize = wifi_handler_finalize;

    g_type_class_add_private(klass, sizeof(WifiHandlerPrivate));

}
