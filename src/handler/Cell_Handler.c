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
 * Filename  : Celld_Handler.c
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
#include <string.h>
#include <Handler_Interface.h>
#include <Cell_Handler.h>
#include <Plugin_Loader.h>
#include <cjson/json.h>
#include <LocationService_Log.h>
typedef struct _CellHandlerPrivate {
    CellPlugin*  cell_plugin;
    gboolean is_started;
    Position* pos;

    PositionCallback pos_cb;

    StartTrackingCallBack track_cb;

    gpointer nwhandler;
    guint log_handler;
    gint api_progress_flag;
} CellHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_CELL, CellHandlerPrivate))

static void cell_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(CellHandler, cell_handler, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (HANDLER_TYPE_INTERFACE, cell_handler_interface_init));

#define CELL_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_CELL, CellHandlerPrivate))



/**
 * <Funciton >   cell_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void cell_handler_tracking_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy, int error,
        gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG] GPS Handler : tracking_cb  latitude =%f , longitude =%f , accuracy =%f\n", position->latitude,position->longitude,accuracy->horizAccuracy);
    CellHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    (*(priv->track_cb)) (enable_cb ,position,accuracy, error, priv->nwhandler, type);

}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void cell_handler_position_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy,
        int error, gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]cell  haposition_cb callback called  %d \n" , enable_cb);
    CellHandlerPrivate* priv = GET_PRIVATE(privateIns);
    g_return_if_fail(priv);
    g_return_if_fail(priv->pos_cb);
    (*priv->pos_cb) (enable_cb ,position, accuracy, error, priv->nwhandler, type);//call SA position callback



}
/**
 * <Funciton >   cell_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_start(Handler *handler_data ) {
    LS_LOG_DEBUG("[DEBUG]cell_handler_start Called");
    CellHandlerPrivate* priv = GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(priv->is_started == TRUE)
        return ERROR_NONE;
    g_return_val_if_fail(priv->cell_plugin, ERROR_NOT_AVAILABLE);

    g_return_val_if_fail(priv->cell_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->cell_plugin->plugin_handler, ERROR_NOT_AVAILABLE);
    ret = priv->cell_plugin->ops.start(priv->cell_plugin->plugin_handler, handler_data);
    if (ret == ERROR_NONE)
        priv->is_started = TRUE;


    return ret;
}

/**o
 * <Funciton >   cell_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_stop(Handler *self)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_stop() \n");
    CellHandlerPrivate* priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;
    if ( priv->is_started == TRUE) {
        g_return_val_if_fail(priv->cell_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->cell_plugin->ops.stop(priv->cell_plugin->plugin_handler);
        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }
    return ret;

}

static void cell_data_cb(LSHandle *sh, LSMessage *reply, void *ctx)
{
    LS_LOG_DEBUG("cell_data_cb \n");

    struct json_object *new_obj;
    struct json_object *act_obj = 0;
    struct json_object *plmn_obj = 0;
    struct json_object *full_name_obj = 0;
    struct json_object *lac_obj = 0;
    struct json_object *cid_obj = 0;
    struct json_object *signalStrengthObj = 0;
    struct json_object *return_value = 0;
    unsigned char act = 1;
    char *carriertype;
    char  *plmn , mcc[3], mnc[3] ;
    char *full_name;
    char* bssidstring;
    int lac, cid,signal;
    gboolean ret;
    CellHandlerPrivate* priv = GET_PRIVATE( (Handler *) ctx);
    g_return_if_fail(priv);
    if(priv->is_started == FALSE)
        return;
    /*Creating a json array*/

    struct json_object *celldata = json_object_new_object();
    struct json_object *cellTowersListItem = json_object_new_object();
    struct json_object *celltowerlist = json_object_new_array();


    const char* str = LSMessageGetPayload(reply);
    LS_LOG_DEBUG("message=%s \n \n",str);

    // str ="{\"act\":1,\"plmn\": 40486}";
    LS_LOG_DEBUG("read root\n");
    new_obj = json_tokener_parse(str);
    LS_LOG_DEBUG("new_obj value %d\n", new_obj);
    return_value = json_object_object_get(new_obj, "returnValue");
    ret =  json_object_get_boolean(return_value);
    if(ret == FALSE){// For time being // failure case when telephony return error

        if(priv->api_progress_flag & CELL_START_TRACKING_ON){
            g_return_if_fail(priv->track_cb);
            (*priv->track_cb) (TRUE ,NULL, NULL, ERROR_NETWORK_ERROR, priv->nwhandler, HANDLER_CELLID);
        }
        if(priv->api_progress_flag & CELL_GET_POSITION_ON){
            g_return_if_fail(priv->pos_cb);
            (*priv->pos_cb) (TRUE ,NULL, NULL, ERROR_NETWORK_ERROR, priv->nwhandler, HANDLER_CELLID);//call SA position callback
        }
        return;
    }
    act_obj = json_object_object_get(new_obj, "act");

    act =  json_object_get_int(act_obj);
    LS_LOG_DEBUG("act value %d\n", act);
    plmn_obj = json_object_object_get(new_obj, "plmn");
    plmn = json_object_get_string(plmn_obj);
    LS_LOG_DEBUG("plmn value%s\n", plmn);
    full_name_obj = json_object_object_get(new_obj, "full_name");
    full_name = json_object_get_string(full_name_obj);
    lac_obj = json_object_object_get(new_obj, "lac");
    lac = json_object_get_int(lac_obj);
    cid_obj = json_object_object_get(new_obj, "cid");
    cid = json_object_get_int(cid_obj);
    signalStrengthObj =json_object_object_get(new_obj, "rssi");
    signal = json_object_get_int(signalStrengthObj);

    //parsing plmn
    if(plmn != NULL){
        strncpy(mcc, plmn , 3);
        strncpy(mnc, plmn+3 , strlen(plmn)-3 );
    }
    // converting to required json
    struct json_object *cell_data_obj = json_object_new_object();


    json_object_object_add(cell_data_obj,"homeMobileCountryCode",json_object_new_int(atoi(mcc)));
    json_object_object_add(cell_data_obj,"homeMobileNetworkCode",json_object_new_int(atoi(mnc)));
    json_object_object_add(cell_data_obj,"radioType",json_object_new_string("gsm"));
    json_object_object_add(cell_data_obj,"carrier",json_object_new_string(full_name));

    json_object_object_add(cellTowersListItem,"cellId",json_object_new_int(cid));
    json_object_object_add(cellTowersListItem,"locationAreaCode",json_object_new_int(lac));
    json_object_object_add(cellTowersListItem,"mobileCountryCode",json_object_new_int(atoi(mcc)));
    json_object_object_add(cellTowersListItem,"mobileNetworkCode",json_object_new_int(atoi(mnc)));
    json_object_object_add(cellTowersListItem,"age",json_object_new_int(0));
    json_object_object_add(cellTowersListItem,"signalStrength",json_object_new_int(signal));

    json_object_array_add(celltowerlist,cellTowersListItem);

    json_object_object_add(cell_data_obj,"cellTowers", celltowerlist);
    if(priv->api_progress_flag & CELL_START_TRACKING_ON){
        priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler, TRUE ,cell_handler_tracking_cb, (gpointer)json_object_to_json_string(cell_data_obj));
    }
    if(priv->api_progress_flag & CELL_GET_POSITION_ON)
        priv->cell_plugin->ops.get_position(priv->cell_plugin->plugin_handler, cell_handler_position_cb, (gpointer)json_object_to_json_string(cell_data_obj));
    json_object_put(celldata);
    json_object_put(celltowerlist);
    json_object_put(cellTowersListItem);
    json_object_put(cell_data_obj);
}
gboolean request_cell_data(LSHandle* sh, gpointer self, int subscribe){

    if(subscribe){
        // cuurnetly only on method later of we can request other method for subscription
        return LSCall(sh,"palm://com.palm.telephony/network/getLocationUpdates", "{}", cell_data_cb, self,NULL,NULL);
    }
    return LSCall(sh,"palm://com.palm.telephony/network/getLocationUpdates", "{}", cell_data_cb, self,NULL,NULL);
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from cell receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int cell_handler_get_position(Handler *self,gboolean enable, PositionCallback pos_cb, gpointer handler,int handlertype,LSHandle* sh  )
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_get_position\n");
    int result = ERROR_NONE;
    CellHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    if(enable){
        if(priv->api_progress_flag & CELL_GET_POSITION_ON)
            return ERROR_DUPLICATE_REQUEST;


        priv->pos_cb= pos_cb;
        priv->nwhandler = handler;
        LS_LOG_DEBUG("[DEBUG]cell_handler_get_position value of newof hanlder %u\n",priv->nwhandler );
        result =request_cell_data(sh,self,FALSE);
        if (result == TRUE) {
            priv->api_progress_flag |= CELL_GET_POSITION_ON;
            result = ERROR_NONE;
        }
        else
            result = ERROR_NOT_AVAILABLE;
    }
    else {
        priv->api_progress_flag &= ~CELL_GET_POSITION_ON;
        LS_LOG_DEBUG("[DEBUG]result : cell_handler_get_position Clearing Position");
    }
    LS_LOG_DEBUG("[DEBUG]result : cell_handler_get_position %d  \n", result);
    return result;
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void cell_handler_start_tracking(Handler *self, gboolean enable ,StartTrackingCallBack track_cb, gpointer handlerobj,int handlertype, LSHandle* sh)
{
    LS_LOG_DEBUG("[DEBUG]Cell handler start Tracking called ");
    int result = ERROR_NONE;
    CellHandlerPrivate* priv = GET_PRIVATE(self);
    if(priv == NULL){
        track_cb(TRUE ,NULL,NULL, ERROR_NOT_AVAILABLE, NULL , HANDLER_CELLID);
        return;
    }
    priv->track_cb =NULL;
    if(enable){
        if (priv->api_progress_flag & CELL_START_TRACKING_ON) {
            return;
        }
        priv->track_cb = track_cb;
        priv->nwhandler= handlerobj;
        // LS_LOG_DEBUG("[DEBUG] cell_handler_start_tracking : priv->track_cb %d  priv->useradata %d \n " ,priv->track_cb ,priv->useradata);
        result =request_cell_data(sh,self,TRUE);
        if(result == TRUE){
            priv->api_progress_flag |= CELL_START_TRACKING_ON;
        }
        else
            track_cb(TRUE ,NULL,NULL, ERROR_NOT_AVAILABLE, NULL , HANDLER_CELLID);
    }else{


        result = priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler, enable ,cell_handler_tracking_cb, NULL);
        if (result == ERROR_NONE)
            priv->api_progress_flag &= ~CELL_START_TRACKING_ON;

    }
    LS_LOG_DEBUG("[DEBUG] return from cell_handler_start_tracking , %d  \n", result);

}

/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int cell_handler_get_last_position(Handler *self, Position **position, Accuracy **accuracy )
{
    int ret = 0;
    CellHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail (priv->cell_plugin, ERROR_NOT_AVAILABLE);


    CellPluginOps pluginOps = priv->cell_plugin->ops;

    // ret = pluginOps.get_last_position(priv->cell_plugin->plugin_handler, position, accuracy);
    return ret;
}

/**
 * <Funciton >   cell_handler_get_velocity
 * <Description>  get the velocity value from  the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <VelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int cell_handler_get_velocity(Handler *self, VelocityCallback vel_cb )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   cell_handler_get_last_velocity
 * <Description>  get the last velocity from the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int cell_handler_get_last_velocity(Handler *self, Velocity **velocity, Accuracy **accuracy  )
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   cell_handler_get_accuracy
 * <Description>  get the accuracy details of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <AccuracyCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int cell_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_get_power_requirement
 * <Description>  get the power requirement of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <power> <In> <power requireemnt of the GPS handler>
 * @return    int
 */
static int cell_handler_get_power_requirement(Handler *self, int* power  )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_get_time_to_first_fix
 * <Description>  get the cell TTFF value of GPS
 * @param     <self> <In> <Handler Gobject>
 * @param     <ttff> <In> <resuting TTFF value>
 * @return    int
 */
static int cell_handler_get_time_to_first_fix(Handler *self,double* ttff  )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}
/**
 * <Funciton >   gps_handler_get_gps_satellite_data
 * <Description>  get the satellite data by  gps by given callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <SatelliteCallback> <In> <callabck function to get the result>
 * @return    int
 */
static int cell_handler_get_gps_satellite_data(Handler *self , gboolean enable_satellite, SatelliteCallback sat_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}
/**
 * <Funciton >   gps_handler_get_nmea_data
 * <Description>  get the nmea data from GPS by the given callabck
 * @param     <self> <In> <Handler Gobject>
 * @param     <NmeaCallback> <In> <callback function to get result>
 * @return    int
 */
static int cell_handler_get_nmea_data(Handler *self , gboolean enable_nmea ,NmeaCallback nmea_cb , gpointer agent_userdata)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   gps_handler_send_extra_command
 * <Description>  send the extra command
 * @param     <self> <In> <Handler Gobject>
 * @param     <command> <In> <Handler Gobject>
 *  * @param     <ExtraCmdCallback> <In> <callabck function to get result>
 * @return    int
 */
static int cell_handler_send_extra_command(Handler *self ,int* command,ExtraCmdCallback extra_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_set_property
 * <Description>  set the proeprty value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> < Gobject>
 * @param     <property_id> <In> <property key >
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int cell_handler_set_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
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
static int cell_handler_get_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get  the current handler used in Hrbrid
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of current handler used>
 * @return    int
 */
static int cell_handler_get_current_handler(Handler *self ,int handlerType )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_set_current_handler
 * <Description>  set the current handler to Hybrid handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of handler to set>
 * @return    int
 */
static int cell_handler_set_current_handler(Handler *self ,int handlerType )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_compare_handler
 * <Description>  compare the handler1 & handler 2 and find the best accurate result
 * @param     <self> <In> <Handler Gobject>
 * @param     <handler1> <In> <id of handler1>
 * @param     <handler2> <In> <id of handler2>
 * @return    int
 */
static int cell_handler_compare_handler(Handler *self ,int handler1 ,int handler2 )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_geo_code
 * <Description>  Convert the given address to Latitide & longitude
 * @param     <Address> <In> <address to geocodet>
 * @param     <GeoCodeCallback> <In> <callback function to get result>
 * @return    int
 */
static int cell_handler_geo_code(Handler *self ,Address* address ,GeoCodeCallback geo_cb  )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}


/**
 * <Funciton >   cell_handler_reverse_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_reverse_geo_code(Handler *self ,Position* pos,RevGeocodeCallback rev_geo_cb )
{

    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}


/**
 * <Funciton >   cell_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void cell_handler_dispose(GObject *gobject)
{


    G_OBJECT_CLASS (cell_handler_parent_class)->dispose (gobject);

}


/**
 * <Funciton >   cell_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void cell_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_finalize");
    CellHandlerPrivate* priv = GET_PRIVATE(gobject);
    g_return_if_fail(priv);
    plugin_free(priv->cell_plugin, "cell");
    priv->cell_plugin = NULL;

    if (priv->pos) {
        position_free(priv->pos);
        priv->pos = NULL;
    }

    g_log_remove_handler(NULL, priv->log_handler);

    G_OBJECT_CLASS (cell_handler_parent_class)->finalize (gobject);
}


/**
 * <Funciton >   cell_handler_interface_init
 * <Description>  cell interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void cell_handler_interface_init(HandlerInterface *interface) {
    interface->start = (TYPE_START_FUNC) cell_handler_start;
    interface->stop = (TYPE_STOP_FUNC) cell_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) cell_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) cell_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) cell_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) cell_handler_get_velocity;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) cell_handler_get_last_velocity;
    interface->get_accuracy = (TYPE_GET_ACCURACY) cell_handler_get_accuracy;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) cell_handler_get_power_requirement;
    interface->get_ttfx = (TYPE_GET_TTFF) cell_handler_get_time_to_first_fix;
    interface->get_sat_data = (TYPE_GET_SAT) cell_handler_get_gps_satellite_data;
    interface->get_nmea_data = (TYPE_GET_NMEA) cell_handler_get_nmea_data;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) cell_handler_send_extra_command;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) cell_handler_get_current_handler;
    interface->set_cur_handler= (TYPE_SET_CUR_HANDLER) cell_handler_set_current_handler;
    interface->compare_handler= (TYPE_COMP_HANDLER) cell_handler_compare_handler;
    interface->get_geo_code= (TYPE_GEO_CODE) cell_handler_geo_code;
    interface->get_rev_geocode= (TYPE_REV_GEO_CODE) cell_handler_reverse_geo_code;
}


/**
 * <Funciton >   cell_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void cell_handler_init(CellHandler *self)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_init");
    CellHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(priv));
    priv->cell_plugin= (CellPlugin*)plugin_new("cell");
    if(priv->cell_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG]cell plugin loading failed\n");
    }
    priv->log_handler = g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL
            | G_LOG_FLAG_RECURSION, lsloghandler, NULL);
    priv->pos = NULL;
}

/**
 * <Funciton >   cell_handler_class_init
 * <Description>  Gobject class init
 * @param     <CellHandlerClass> <In> <CellClass instance>
 * @return    int
 */
static void cell_handler_class_init(CellHandlerClass *klass)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = cell_handler_dispose;
    gobject_class->finalize = cell_handler_finalize;

    g_type_class_add_private(klass, sizeof(CellHandlerPrivate));

}
