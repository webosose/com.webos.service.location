/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Gps_Handler.c
 * Purpose      : Provides GPS related location services to Location Service Agent
 * Platform     : RedRose
 * Author(s)    : rajesh gopu , Abhishek Srivastava
 * Email ID.    : rajeshgopu.iv@lge.com , abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Handler_Interface.h>
#include <Gps_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>
#include <Location.h>

#define GPS_UPDATE_INTERVAL_MAX     12*60

typedef struct _GpsHandlerPrivate {
    GpsPlugin *gps_plugin;
    gboolean is_started;
    Position *pos;
    Velocity *vel;

    PositionCallback pos_cb;
    StatusCallback status_cb;
    VelocityCallback vel_cb;
    SatelliteCallback sat_cb;

    NmeaCallback nmea_cb;
    StartTrackingCallBack track_cb;

    guint pos_timer;
    guint log_handler;
    gint api_progress_flag;
} GpsHandlerPrivate;

#define GPS_HANDLER_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_GPS, GpsHandlerPrivate))

static void handler_interface_init(HandlerInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GpsHandler, gps_handler, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (HANDLER_TYPE_INTERFACE, handler_interface_init));



static gboolean gps_timeout_cb(gpointer user_data)
{
    LS_LOG_DEBUG("timeout of gps position update\n");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_val_if_fail(priv, G_SOURCE_REMOVE);

    // Post to agent that Gps timeout happend
    LocationFields field = (POSITION_FIELDS_LATITUDE | POSITION_FIELDS_LONGITUDE | POSITION_FIELDS_ALTITUDE);
    Position *position = position_create(0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, field);
    Accuracy *accuracy = accuracy_create(ACCURACY_LEVEL_NONE, 0.0, 0.0);

    if (priv->pos_cb)
        (*priv->pos_cb)(TRUE, position, accuracy, ERROR_TIMEOUT, priv, HANDLER_GPS);
    if (priv->track_cb)
        (*priv->track_cb)(TRUE, position, accuracy, ERROR_TIMEOUT, priv, HANDLER_GPS);

    position_free(position);
    accuracy_free(accuracy);

    return G_SOURCE_REMOVE;
}

/**
 * <Funciton>       gps_handler_status_cb
 * <Description>    Callback function for Status, as per Location_Plugin.h
 * @param           <enable_cb> <In> <Handler GObject>
 * @param           <status> <In> <Status Info>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_status_cb(gboolean enable_cb, GPSStatus status, gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);

    if (status == GPS_STATUS_AVAILABLE) {
        // remove timeout timer.
        // This will be implemented with timeout and agent sync
        if (priv->pos_timer) {
            g_source_remove(priv->pos_timer);
            priv->pos_timer = 0;
        }
    }

    g_return_if_fail(priv->status_cb);

    LS_LOG_DEBUG("status_cb callback called %d\n", enable_cb);
    (*priv->status_cb)(enable_cb, status, user_data);
}

/**
 * <Funciton>       gps_handler_satellite_cb
 * <Description>    Callback function for Satellite ,as per Location_Plugin.h
 * @param           <self> <In> <Handler Gobject>
 * @param           <Satellite> <In> <Satellite data structure>
 * @param           <Accuracy> <In> <Accuracy data structure>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_satellite_cb(gboolean enable_cb, Satellite *satellite, gpointer user_data)
{ 
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->sat_cb);
    
    LS_LOG_DEBUG("satellite_cb callback called  %d\n", enable_cb);
    (*priv->sat_cb)(enable_cb, satellite, user_data);
}

/**
 * <Funciton>       gps_handler_nmea_cb
 * <Description>    Callback function for Nmea data
 * @param           <enable_cb> <In> <enable/disable of callback>
 * @param           <timestamp> <In> <timestamp>
 * @param           <data> <In> <data>
 * @param           <length> <In> <length>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_nmea_cb(gboolean enable_cb, int timestamp, char *data, int length, gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->nmea_cb);
    
    (*priv->nmea_cb)(enable_cb, timestamp, data, length, user_data);
}

/**
 * <Funciton>       gps_handler_tracking_cb
 * <Description>    Callback function for Location tracking
 * @param           <Position> <In> <Position data stricture>
 * @param           <Accuracy> <In> <Accuracy data structure>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer user_data, int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);

    LS_LOG_DEBUG("GPS Handler: tracking_cb latitude = %f, longitude = %f, accuracy = %f\n",
            position->latitude, position->longitude, accuracy->horizAccuracy);
    (*priv->track_cb)(enable_cb, position, accuracy, error, user_data, type);
}

/**
 * <Funciton>       gps_handler_position_cb
 * <Description>    Callback function for Position, as per Location_Plugin.h
 * @param           <Position> <In> <Position data stricture>
 * @param           <Accuracy> <In> <Accuracy data structure>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer user_data, int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->pos_cb);

    LS_LOG_DEBUG("GPS Handler: position_cb %d\n", enable_cb);
    (*priv->pos_cb)(enable_cb, position, accuracy, error, user_data, type); //call SA position callback
}

/**
 * <Funciton>       gps_handler_start
 * <Description>    start the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
static int gps_handler_start(Handler *self)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("gps_handler_start\n");
    if (priv->is_started)
        return ERROR_NONE;

    g_return_val_if_fail(priv->gps_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    ret = priv->gps_plugin->ops.start(priv->gps_plugin->plugin_handler, gps_handler_status_cb, self);
    if (ret == ERROR_NONE)
        priv->is_started = TRUE;

    return ret;
}

/**
 * <Funciton>       gps_handler_stop
 * <Description>    stop the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
static int gps_handler_stop(Handler *self)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    
    LS_LOG_DEBUG("gps_handler_stop() api progress %d\n", priv->api_progress_flag);
    if (priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->pos_timer) {
        g_source_remove(priv->pos_timer);
        priv->pos_timer = 0;
    }

    if (priv->is_started) {
        g_return_val_if_fail(priv->gps_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->gps_plugin->ops.stop(priv->gps_plugin->plugin_handler);
        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

/**
 * <Funciton>       gps_handler_get_position
 * <Description>    get the position from GPS receiver
 * @param           <self> <In> <Handler GObject>
 * @param           <self> <In> <Position callback function to get result>
 * @return          int
 */
static int gps_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb)
{
    int result = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("gps_handler_get_position\n");
    if (enable) {
        if (priv->api_progress_flag & GET_POSITION_ON)
            return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(pos_cb, ERROR_NOT_AVAILABLE);
        g_return_val_if_fail(priv->gps_plugin->ops.get_position, ERROR_NOT_AVAILABLE);
        priv->pos_cb = pos_cb;

        result = priv->gps_plugin->ops.get_position(priv->gps_plugin->plugin_handler, gps_handler_position_cb);

        if (result == ERROR_NONE) {
            if (!priv->pos_timer)
                priv->pos_timer = g_timeout_add_seconds(GPS_UPDATE_INTERVAL_MAX, gps_timeout_cb, self);

            priv->api_progress_flag |= GET_POSITION_ON;
        }

        LS_LOG_DEBUG("result: gps_handler_get_position %d\n", result);
    } else {
        // Clear the Flag
        priv->api_progress_flag &= ~GET_POSITION_ON;
        LS_LOG_DEBUG("result: gps_handler_get_position Clearing Position\n");
    }

    return result;
}

/**
 * <Funciton>       gps_handler_start_tracking
 * <Description>    get the position from GPS receiver
 * @param           <self> <In> <Handler Gobject>
 * @param           <self> <In> <Position callback function to get result>
 * @return          int
 */
static void gps_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb)
{
    LS_LOG_DEBUG("GPS handler start Tracking called\n");
    int result = ERROR_NONE;

    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    if ((priv == NULL) || (priv->gps_plugin->ops.get_gps_data == NULL)) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        return;
    }

    priv->track_cb = NULL;
    if (enable) {
        if (priv->api_progress_flag & START_TRACKING_ON)
            return;

        priv->track_cb = track_cb;
        LS_LOG_DEBUG("gps_handler_start_tracking: priv->track_cb %d\n", priv->track_cb);

        result = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb, HANDLER_DATA_POSITION);

        if (result == ERROR_NONE) {
            if (!priv->pos_timer)
                priv->pos_timer = g_timeout_add_seconds(GPS_UPDATE_INTERVAL_MAX, gps_timeout_cb, self);
            
            priv->api_progress_flag |= START_TRACKING_ON;
        } else {
            track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        }
    } else {  
        if (priv->pos_timer) {
            g_source_remove(priv->pos_timer);
            priv->pos_timer = 0;
        }

        result = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb, HANDLER_DATA_POSITION);
        if (result == ERROR_NONE)
            priv->api_progress_flag &= ~START_TRACKING_ON;

        LS_LOG_DEBUG("gps_handler_start_tracking Clearing bit\n");
    }
}

/**
 * <Funciton>       gps_handler_get_last_position
 * <Description>    get the last position from the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @param           <LastPositionCallback> <In> <Callabck to to get the result>
 * @return          int
 */
static int gps_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (gps_plugin_get_stored_position(position, accuracy) == ERROR_NOT_AVAILABLE) {
        LS_LOG_DEBUG("get last poistion Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

/**
 * <Funciton>       gps_handler_get_velocity
 * <Description>    get the velocity value from  the GPS receiver
 * @param           <self> <In> <Handler GObject>
 * @param           <VelocityCallback> <In> <Callabck to to get the result>
 * @return          int
 */
static int gps_handler_get_velocity(Handler *self, VelocityCallback vel_cb)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton>       gps_handler_get_last_velocity
 * <Description>    get the last velocity from the GPS receiver
 * @param           <self> <In> <Handler GObject>
 * @param           <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return          int
 */
static int gps_handler_get_last_velocity(Handler *self, Velocity *velocity, Accuracy *accuracy)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton>       gps_handler_get_accuracy
 * <Description>    get the accuracy details of the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @param           <AccuracyCallback> <In> <Callabck to to get the result>
 * @return          int
 */
static int gps_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_power_requirement
 * <Description>    get the power requirement of the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @param           <power> <In> <power requireemnt of the GPS handler>
 * @return          int
 */
static int gps_handler_get_power_requirement(Handler *self, int *power)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_time_to_first_fix
 * <Description>    get the gps TTFF value of GPS
 * @param           <self> <In> <Handler Gobject>
 * @param           <ttff> <In> <resuting TTFF value>
 * @return          int
 */
static int gps_handler_get_time_to_first_fix(Handler *self, double *ttff)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_gps_satellite_data
 * <Description>    get the satellite data by  gps by given callback
 * @param           <self> <In> <Handler Gobject>
 * @param           <SatelliteCallback> <In> <callabck function to get the result>
 * @return          int
 */
static int gps_handler_get_gps_satellite_data(Handler *self, gboolean enable_satellite, SatelliteCallback sat_cb)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate* priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable_satellite) {
        if (priv->api_progress_flag & SATELLITE_GET_DATA_ON)
            return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(priv->gps_plugin->ops.get_gps_data, ERROR_NOT_AVAILABLE);
        g_return_val_if_fail(sat_cb, ERROR_NOT_AVAILABLE);
        priv->sat_cb = sat_cb;

        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_satellite, gps_handler_satellite_cb, HANDLER_DATA_SATELLITE);
        if (ret == ERROR_NONE) {
            priv->api_progress_flag |= SATELLITE_GET_DATA_ON;
        }
    } else {
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_satellite, gps_handler_satellite_cb, HANDLER_DATA_SATELLITE);
        if (ret == ERROR_NONE)
            priv->api_progress_flag &= ~SATELLITE_GET_DATA_ON;
    }

    return ret;
}

/**
 * <Funciton>       gps_handler_get_nmea_data
 * <Description>    get the nmea data from GPS by the given callabck
 * @param           <self> <In> <Handler GObject>
 * @param           <NmeaCallback> <In> <callback function to get result>
 * @return          int
 */
static int gps_handler_get_nmea_data(Handler *self, gboolean enable_nmea, NmeaCallback nmea_cb, gpointer agent_userdata)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable_nmea) {
        if (priv->api_progress_flag & NMEA_GET_DATA_ON) {
            return ERROR_DUPLICATE_REQUEST;
        }
        g_return_val_if_fail(priv->gps_plugin->ops.get_gps_data, ERROR_NOT_AVAILABLE);
        g_return_val_if_fail (nmea_cb, ERROR_NOT_AVAILABLE);
        priv->nmea_cb = nmea_cb;

        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb, HANDLER_DATA_NMEA);
        if (ret == ERROR_NONE) {
            priv->api_progress_flag |= NMEA_GET_DATA_ON;
        }
    } else {
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb, HANDLER_DATA_NMEA);
        if (ret == ERROR_NONE)
            priv->api_progress_flag &= ~NMEA_GET_DATA_ON;
    }

    return ret;
}

/**
 * <Funciton>       gps_handler_send_extra_command
 * <Description>    send the extra command
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <Handler GObject>
 * @param           <ExtraCmdCallback> <In> <callabck function to get result>
 * @return          int
 */
static int gps_handler_send_extra_command(Handler *self, int *command, ExtraCmdCallback extra_cb)
{
    int ret = 0;
    return ret;
}

/**
 * <Funciton>       gps_handler_set_property
 * <Description>    set the proeprty value for the given property id
 * @param           <self> <In> <Handler GObject>
 * @param           <object> <In> < GObject>
 * @param           <property_id> <In> <property key>
 * @param           <pspec> <In> <pspec>
 * @return          int
 */
static int gps_handler_set_property(Handler* self ,GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton>       gps_handler_get_property
 * <Description>    get the property value for the given property id
 * @param           <self> <In> <Handler GObject>
 * @param           <object> <In> <Handler GObject>
 * @param           <property_id> <In> <property id to change>
 * @param           <value> <In> <value to set>
 * @param           <pspec> <In> <pspec>
 * @return          int
 */
static int gps_handler_get_property(Handler *self, GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_current_handler
 * <Description>    get the current handler used in Hrbrid
 * @param           <self> <In> <Handler GObject>
 * @param           <handlerType> <In> <id of current handler used>
 * @return          int
 */
static int gps_handler_get_current_handler(Handler *self, int handlerType)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_set_current_handler
 * <Description>    set the current handler to Hybrid handler
 * @param           <self> <In> <Handler GObject>
 * @param           <handlerType> <In> <id of handler to set>
 * @return          int
 */
static int gps_handler_set_current_handler(Handler *self, int handlerType)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_compare_handler
 * <Description>    compare the handler1 & handler 2 and find the best accurate result
 * @param           <self> <In> <Handler GObject>
 * @param           <handler1> <In> <id of handler1>
 * @param           <handler2> <In> <id of handler2>
 * @return          int
 */
static int gps_handler_compare_handler(Handler *self, int handler1, int handler2)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_geo_code
 * <Description>    Convert the given address to Latitide & longitude
 * @param           <Address> <In> <address to geocodet>
 * @param           <GeoCodeCallback> <In> <callback function to get result>
 * @return          int
 */
static int gps_handler_get_geo_code(Handler *self, Address *address, GeoCodeCallback geo_cb)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       gps_handler_get_reverse_geo_code
 * <Description>    Convert the given position values to the Address
 *                  and provide the result in callback
 * @param           <self> <In> <Handler GObject>
 * @param           <Position> <In> <position to rev geocode>
 * @param           <RevGeocodeCallback> <In> <Handler GObject>
 * @return          int
 */
static int gps_handler_get_reverse_geo_code(Handler *self, Position *pos, RevGeocodeCallback rev_geo_cb)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton>       dispose
 * <Description>    dispose the gobject
 * @param           <gobject> <In> <GObject>
 * @return          void
 */
static void dispose(GObject *gobject)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(gobject);

    if (priv->pos_timer)
        g_source_remove(priv->pos_timer);

    priv->pos_timer = 0;

    G_OBJECT_CLASS(gps_handler_parent_class)->dispose(gobject);
}

/**
 * <Funciton>       finalize
 * <Description>    finalize
 * @param           <gobject> <In> <Handler GObject>
 * @return          void
 */
static void finalize(GObject *gobject)
{
    GpsHandlerPrivate* priv = GPS_HANDLER_GET_PRIVATE(gobject);
    g_return_if_fail(priv);

    LS_LOG_DEBUG("finalizing gps handler\n");

    if (priv->gps_plugin) {
        plugin_free(priv->gps_plugin, "gps");
        priv->gps_plugin = NULL;
    }

    if (priv->pos) {
        position_free(priv->pos);
        priv->pos = NULL;
    }

    if (priv->vel) {
        velocity_free(priv->vel);
        priv->vel = NULL;
    }

    g_log_remove_handler(NULL, priv->log_handler); 
    
    priv->pos_cb = NULL;
    priv->status_cb = NULL;
    priv->vel_cb = NULL;
    priv->sat_cb = NULL;

    priv->nmea_cb = NULL;
    priv->track_cb = NULL;

    G_OBJECT_CLASS(gps_handler_parent_class)->finalize(gobject);
}

/**
 * <Funciton>       handler_interface_init
 * <Description>    gps interface init, map all th local function to GPS interface
 * @param           <HandlerInterface> <In> <HandlerInterface instance>
 * @return          void
 */
static void handler_interface_init(HandlerInterface *iface)
{
    iface->start = (TYPE_START_FUNC) gps_handler_start;
    iface->stop = (TYPE_STOP_FUNC) gps_handler_stop;
    iface->get_position = (TYPE_GET_POSITION) gps_handler_get_position;
    iface->start_tracking = (TYPE_START_TRACK) gps_handler_start_tracking;
    iface->get_last_position = (TYPE_GET_LAST_POSITION) gps_handler_get_last_position;
    iface->get_velocity = (TYPE_GET_VELOCITY) gps_handler_get_velocity;
    iface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) gps_handler_get_last_velocity;
    iface->get_accuracy = (TYPE_GET_ACCURACY) gps_handler_get_accuracy;
    iface->get_power_requirement = (TYPE_GET_POWER_REQ) gps_handler_get_power_requirement;
    iface->get_ttfx = (TYPE_GET_TTFF) gps_handler_get_time_to_first_fix;
    iface->get_sat_data = (TYPE_GET_SAT) gps_handler_get_gps_satellite_data;
    iface->get_nmea_data = (TYPE_GET_NMEA) gps_handler_get_nmea_data;
    iface->send_extra_cmd = (TYPE_SEND_EXTRA) gps_handler_send_extra_command;
    iface->get_cur_handler = (TYPE_GET_CUR_HANDLER) gps_handler_get_current_handler;
    iface->set_cur_handler= (TYPE_SET_CUR_HANDLER) gps_handler_set_current_handler;
    iface->compare_handler= (TYPE_COMP_HANDLER) gps_handler_compare_handler;
    iface->get_geo_code= (TYPE_GEO_CODE) gps_handler_get_geo_code;
    iface->get_rev_geocode= (TYPE_REV_GEO_CODE) gps_handler_get_reverse_geo_code;
}

/**
 * <Funciton>       gps_handler_init
 * <Description>    GPS handler init GObject function
 * @param           <self> <In> <GPS Handler GObject>
 * @return          void
 */
static void gps_handler_init(GpsHandler *self)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_if_fail(priv);

    memset(priv, 0, sizeof(GpsHandlerPrivate));
    priv->gps_plugin = (GpsPlugin*)plugin_new("gps");
    if (priv->gps_plugin == NULL) {
        LS_LOG_ERROR("Failed to load gps plugin\n");
    }

    priv->log_handler = g_log_set_handler(NULL,
            G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
            lsloghandler, NULL);
}

/**
 * <Funciton>       gps_handler_class_init
 * <Description>    GObject class init
 * @param           <GpsHandlerClass> <In> <GpsClass instance>
 * @return          void
 */
static void gps_handler_class_init(GpsHandlerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = dispose;
    gobject_class->finalize = finalize;

    g_type_class_add_private(klass, sizeof(GpsHandlerPrivate));
}
