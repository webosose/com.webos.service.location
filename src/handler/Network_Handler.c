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
#include <Google_Handler.h>
#include <Cell_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>
#include <Location.h>

typedef struct _NwHandlerPrivate {
    HandlerObject *handler_obj [MAX_HANDLER_TYPE];
    gboolean is_started;
    Position* pos;
    PositionCallback pos_cb_arr[MAX_HANDLER_TYPE];
    PositionCallback nw_cb_arr[MAX_HANDLER_TYPE];
    StartTrackingCallBack track_cb;

} NwHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_NW, NwHandlerPrivate))

static void nw_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(NwHandler, nw_handler, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (HANDLER_TYPE_INTERFACE, nw_handler_interface_init));

#define WIFI_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_NW, NwHandlerPrivate))









/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_wifi_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy, int error,
        gpointer privateIns , int type)
{
    LS_LOG_DEBUG("[DEBUG]NW  nw_handler_position_wifi_cb callback called  %d , type %d\n" , enable_cb, type);
    NwHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb_arr[type]);
    (priv->pos_cb_arr[type]) (enable_cb ,position, accuracy, error, priv, type);//call SA position callback



}

void nw_handler_position_google_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy, int error,
        gpointer privateIns,int type)
{
    LS_LOG_DEBUG("[DEBUG]NW  nw_handler_position_google_cb callback called  %d \n" , enable_cb);
    NwHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb_arr[type]);
    (*priv->pos_cb_arr[type]) (enable_cb ,position, accuracy, error, priv, type);//call SA position callback



}
/**
 * <Funciton >   nw_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_tracking_cb(gboolean enable_cb ,Position *position,Accuracy *accuracy, int error,
        gpointer privateIns , int type)
{
    LS_LOG_DEBUG("[DEBUG] NW Handler : tracking_cb  latitude =%f , longitude =%f , accuracy =%f\n", position->latitude,position->longitude,accuracy->horizAccuracy);
    NwHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    (*(priv->track_cb)) (enable_cb ,position,accuracy, error, priv, type);

}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void nw_handler_position_cell_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy, int error,
        gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]NW  nw_handler_position_cellid_cb callback called  %d \n" , enable_cb);
    NwHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb_arr[type]);
    (*priv->pos_cb_arr[type]) (enable_cb ,position, accuracy, error, priv, type);//call SA position callback



}
/**
 * <Funciton >   wifi_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_start(Handler *handler_data, int handler_type ) {
    LS_LOG_DEBUG("[DEBUG]nw_handler_start Called handler type%d ", handler_type );
    NwHandlerPrivate* priv = GET_PRIVATE(handler_data);
    HandlerObject *wifihandler;
    int ret = ERROR_NOT_AVAILABLE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    // This should call cell id start or Wifi start
    switch (handler_type){
        case HANDLER_WIFI:
            if(plugin_is_supported("wifi")){
                priv->handler_obj [handler_type]  = g_object_new(HANDLER_TYPE_WIFI, NULL);
                g_return_val_if_fail(priv->handler_obj [handler_type], ERROR_NOT_AVAILABLE);
                priv->nw_cb_arr[handler_type] = nw_handler_position_wifi_cb;
                ret = handler_start(HANDLER_INTERFACE(priv->handler_obj [handler_type]), handler_type);
                if(ret == ERROR_NONE)
                    LS_LOG_DEBUG("wifi_handler_start started \n");
            }

            break;
        case HANDLER_CELLID:
            if(plugin_is_supported("cell")){
                priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_CELL, NULL);
                g_return_val_if_fail(priv->handler_obj [handler_type], ERROR_NOT_AVAILABLE);
                priv->nw_cb_arr[handler_type] = nw_handler_position_cell_cb;
                ret = handler_start(HANDLER_INTERFACE( priv->handler_obj[handler_type]),handler_type);
            }

            break;
        case HANDLER_GOOGLE_WIFI_CELL:
            priv->handler_obj[handler_type] = g_object_new(HANDLER_TYPE_GOOGLE, NULL);
            g_return_val_if_fail(priv->handler_obj[handler_type], ERROR_NOT_AVAILABLE);
            priv->nw_cb_arr[handler_type] = nw_handler_position_google_cb;
            ret = handler_start(HANDLER_INTERFACE(priv->handler_obj[handler_type]),handler_type);
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
    NwHandlerPrivate* priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    // This should call cell id stop or Wifi stop
    ret = handler_stop(HANDLER_INTERFACE(priv->handler_obj[handlertype]), handlertype);
    priv->is_started = FALSE;
    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int nw_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handle, int handlertype,LSHandle* sh )
{
    LS_LOG_DEBUG("[DEBUG]nw_handler_get_position\n");
    int result = ERROR_NONE;
    NwHandlerPrivate* priv = GET_PRIVATE(self);

    LS_LOG_DEBUG("[DEBUG]nw_handler_get_position hanlder type [%d]\n", handlertype);
    priv->pos_cb_arr[handlertype] = pos_cb;
    result = handler_get_position (priv->handler_obj[handlertype] , enable, priv->nw_cb_arr[handlertype] , self, handlertype,sh);
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
static void nw_handler_start_tracking(Handler *self, gboolean enable ,StartTrackingCallBack track_cb ,gpointer handleobj, int handlertype, LSHandle* sh)
{
    int result = ERROR_NONE;
    NwHandlerPrivate* priv = GET_PRIVATE(self);
    if(priv == NULL)
        track_cb(TRUE ,NULL,NULL, ERROR_NOT_AVAILABLE, NULL , handlertype);
    priv->track_cb =NULL;
    if(enable){

        priv->track_cb = track_cb;
        LS_LOG_DEBUG("[DEBUG] nw_handler_start_tracking : priv->track_cb %d \n " ,priv->track_cb);
        handler_start_tracking(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb,  self, handlertype,sh);


    }else{

        handler_start_tracking(HANDLER_INTERFACE(priv->handler_obj[handlertype]), enable, nw_handler_tracking_cb,  self, handlertype,sh);
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
static int nw_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy, int handlertype )
{
    int ret = 0;
    NwHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    ret = handler_get_last_position (HANDLER_INTERFACE(priv->handler_obj[handlertype]) , position, accuracy, handlertype);
    return ret;
}

/**
 * <Funciton >   wifi_handler_get_velocity
 * <Description>  get the velocity value from  the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <VelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int nw_handler_get_velocity(Handler *self, VelocityCallback vel_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   nw_handler_get_last_velocity
 * <Description>  get the last velocity from the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int nw_handler_get_last_velocity(Handler *self, Velocity **velocity, Accuracy **accuracy  )
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
static int nw_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb )
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
static int nw_handler_get_power_requirement(Handler *self, int* power  )
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
static int nw_handler_get_time_to_first_fix(Handler *self,double* ttff  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}
/**
 * <Funciton >   nw_handler_get_nw_satellite_data
 * <Description>  get the satellite data by  nw by given callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <SatelliteCallback> <In> <callabck function to get the result>
 * @return    int
 */
static int nw_handler_get_gps_satellite_data(Handler *self , gboolean enable_satellite, SatelliteCallback sat_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}
/**
 * <Funciton >   nw_handler_get_nmea_data
 * <Description>  get the nmea data from GPS by the given callabck
 * @param     <self> <In> <Handler Gobject>
 * @param     <NmeaCallback> <In> <callback function to get result>
 * @return    int
 */
static int nw_handler_get_nmea_data(Handler *self , gboolean enable_nmea ,NmeaCallback nmea_cb , gpointer agent_userdata)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   nw_handler_send_extra_command
 * <Description>  send the extra command
 * @param     <self> <In> <Handler Gobject>
 * @param     <command> <In> <Handler Gobject>
 *  * @param     <ExtraCmdCallback> <In> <callabck function to get result>
 * @return    int
 */
static int nw_handler_send_extra_command(Handler *self ,int* command,ExtraCmdCallback extra_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   nw_handler_set_property
 * <Description>  set the proeprty value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> < Gobject>
 * @param     <property_id> <In> <property key >
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int nw_handler_set_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
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
static int nw_handler_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
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
static int nw_handler_get_current_handler(Handler *self ,int handlerType )
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
static int nw_handler_set_current_handler(Handler *self ,int handlerType )
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
static int nw_handler_compare_handler(Handler *self ,int handler1 ,int handler2 )
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
static int nw_handler_geo_code(Handler *self ,Address* address ,GeoCodeCallback geo_cb  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}


/**
 * <Funciton >   nw_handler_reverse_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int nw_handler_reverse_geo_code(Handler *self ,Position* pos,RevGeocodeCallback rev_geo_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}


/**
 * <Funciton >   nw_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void nw_handler_dispose(GObject *gobject)
{

    G_OBJECT_CLASS (nw_handler_parent_class)->dispose (gobject);
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
    NwHandlerPrivate* priv = GET_PRIVATE(gobject);
    int handler;

    for( handler = 0; handler<3; handler++) {
        if(priv->handler_obj[handler]){
            g_object_unref (priv->handler_obj[handler]);
            priv->handler_obj[handler] = NULL;
            priv->nw_cb_arr[handler] = NULL ;
            priv->pos_cb_arr[handler] = NULL;
        }
    }
    if (priv->pos) {
        position_free(priv->pos);
        priv->pos = NULL;
    }

    G_OBJECT_CLASS (nw_handler_parent_class)->finalize (gobject);
}


/**
 * <Funciton >   nw_handler_interface_init
 * <Description>  nw interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void nw_handler_interface_init(HandlerInterface *interface) {
    interface->start = (TYPE_START_FUNC) nw_handler_start;
    interface->stop = (TYPE_STOP_FUNC) nw_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) nw_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) nw_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) nw_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) nw_handler_get_velocity;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) nw_handler_get_last_velocity;
    interface->get_accuracy = (TYPE_GET_ACCURACY) nw_handler_get_accuracy;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) nw_handler_get_power_requirement;
    interface->get_ttfx = (TYPE_GET_TTFF) nw_handler_get_time_to_first_fix;
    interface->get_sat_data = (TYPE_GET_SAT) nw_handler_get_gps_satellite_data;
    interface->get_nmea_data = (TYPE_GET_NMEA) nw_handler_get_nmea_data;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) nw_handler_send_extra_command;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) nw_handler_get_current_handler;
    interface->set_cur_handler= (TYPE_SET_CUR_HANDLER) nw_handler_set_current_handler;
    interface->compare_handler= (TYPE_COMP_HANDLER) nw_handler_compare_handler;
    interface->get_geo_code= (TYPE_GEO_CODE) nw_handler_geo_code;
    interface->get_rev_geocode= (TYPE_REV_GEO_CODE) nw_handler_reverse_geo_code;
}


/**
 * <Funciton >   nw_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void nw_handler_init(NwHandler *self)
{
    NwHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(priv));


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
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = nw_handler_set_property;
    gobject_class->get_property = nw_handler_get_property;
    gobject_class->dispose = nw_handler_dispose;
    gobject_class->finalize = nw_handler_finalize;

    g_type_class_add_private(klass, sizeof(NwHandlerPrivate));

}
