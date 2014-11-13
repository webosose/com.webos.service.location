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
#include <loc_log.h>
#include <time.h>
#include <Location.h>

#define GPS_UPDATE_INTERVAL_MAX     12*60

typedef struct _GpsHandlerPrivate {
    GpsPlugin *gps_plugin;
    gboolean is_started;
    PositionCallback pos_cb;
    StatusCallback status_cb;
    SatelliteCallback sat_cb;

    NmeaCallback nmea_cb;
    StartTrackingCallBack track_cb;
    GeofenceAddCallBack add_cb;
    GeofenceResumeCallback resume_cb;
    GeofencePauseCallback pause_cb;
    GeofenceRemoveCallback remove_cb;
    GeofenceBreachCallback breach_cb;
    GeofenceStatusCallback geofence_status_cb;
    StartTrackingCallBack loc_update_cb;
    guint pos_timer;
    gint status;
    gint api_progress_flag;
    gint64 fixrequesttime;
    gint64 ttff;
    gint64 lastfixtime;
    gboolean ttffstate;
    GMutex mutex;

} GpsHandlerPrivate;

#define GPS_HANDLER_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_GPS, GpsHandlerPrivate))

static void handler_interface_init(HandlerInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GpsHandler, gps_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, handler_interface_init));

static gboolean gps_timeout_cb(gpointer user_data)
{
    LS_LOG_INFO("timeout of gps position update\n");
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

    if (priv->loc_update_cb)
         (*priv->loc_update_cb)(TRUE, position, accuracy, ERROR_TIMEOUT, priv, HANDLER_GPS);

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
    LS_LOG_INFO("gps_handler_status_cb status = %d", status);
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);

    priv->status = status;

    if (priv->status_cb == NULL)
        return;

    priv->status_cb(enable_cb, status, user_data);
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
void gps_handler_nmea_cb(gboolean enable_cb, int64_t timestamp, char *data, int length, gpointer user_data)
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
void gps_handler_tracking_cb(gboolean enable_cb,
                             Position *position,
                             Accuracy *accuracy,
                             int error,
                             gpointer user_data,
                             int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);


    if (priv->ttffstate == FALSE) {
        struct timeval tv;
        gettimeofday(&tv, (struct timezone *) NULL);
        priv->lastfixtime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
        priv->ttffstate = TRUE;
    }

    if (position != NULL) {
        LS_LOG_DEBUG("GPS Handler: tracking_cb latitude = %f, longitude = %f, accuracy = %f\n",
                      position->latitude, position->longitude,
                      accuracy->horizAccuracy);
    }

    if (priv->track_cb != NULL)
        (*priv->track_cb)(enable_cb, position, accuracy, error, user_data, type);

    if (priv->loc_update_cb != NULL)
        (*priv->loc_update_cb)(enable_cb, position, accuracy, error, user_data, type);

    if (priv->pos_timer) {
        g_source_remove(priv->pos_timer);
        priv->pos_timer = 0;
    }
}

/**
 * <Funciton>       gps_handler_position_cb
 * <Description>    Callback function for Position, as per Location_Plugin.h
 * @param           <Position> <In> <Position data stricture>
 * @param           <Accuracy> <In> <Accuracy data structure>
 * @param           <user_data> <In> <instance of handler>
 * @return          void
 */
void gps_handler_position_cb(gboolean enable_cb,
                             Position *position,
                             Accuracy *accuracy,
                             int error,
                             gpointer user_data,
                             int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);

    if (priv->ttffstate == FALSE) {
        struct timeval tv;
        gettimeofday(&tv, (struct timezone *) NULL);
        priv->lastfixtime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
        priv->ttffstate == TRUE;
    }

    g_return_if_fail(priv->pos_cb);

    LS_LOG_INFO("GPS Handler: position_cb %d\n", enable_cb);

    (*priv->pos_cb)(enable_cb, position, accuracy, error, user_data, type); //call SA position callback
}

void gps_handler_geofence_status_cb(int32_t status,
                                    Position *last_position,
                                    Accuracy *accuracy,
                                    gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->geofence_status_cb);

    (*priv->geofence_status_cb)(status, last_position, accuracy, user_data);
}

void gps_handler_geofence_breach_cb(int32_t geofence_id,
                                    int32_t status,
                                    int64_t timestamp,
                                    double latitude,
                                    double longitude,
                                    gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->breach_cb);

    (*priv->breach_cb)( geofence_id, status, timestamp, latitude, longitude, user_data);
}

void gps_handler_geofence_add_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    LS_LOG_INFO("gps_handler_geofence_add_cb..\n");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->add_cb);

    (*priv->add_cb)( geofence_id, status, user_data);
}

void gps_handler_geofence_remove_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);
    g_return_if_fail(priv->remove_cb);

    (*priv->remove_cb)(geofence_id, status, user_data);
}

void gps_handler_geofence_resume_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);
    g_return_if_fail(priv->resume_cb);

    (*priv->resume_cb)( geofence_id, status, user_data);
}

void gps_handler_geofence_pause_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);

    g_return_if_fail(priv);
    g_return_if_fail(priv->pause_cb);

    (*priv->pause_cb)( geofence_id, status, user_data);
}

/**
 * <Funciton>       gps_handler_start
 * <Description>    start the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
static int gps_handler_start(Handler *self, int handler_type, const char* license_key)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("gps_handler_start\n");

    g_mutex_lock(&priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(&priv->mutex);
        return ERROR_NONE;
    }

    ret = priv->gps_plugin->ops.start(priv->gps_plugin->plugin_handler, gps_handler_status_cb, self);

    if (ret == ERROR_NONE) {
        if (priv->ttffstate == FALSE) {
            priv->ttff = 0;
            priv->lastfixtime = 0;
            priv->fixrequesttime = 0;
            struct timeval tv;
            gettimeofday(&tv, (struct timezone *) NULL);
            priv->fixrequesttime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
        }

        priv->is_started = TRUE;
    }

    g_mutex_unlock(&priv->mutex);

    return ret;
}

/**
 * <Funciton>       gps_handler_stop
 * <Description>    stop the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
static int gps_handler_stop(Handler *self,  int handler_type, gboolean forcestop)
{
    int ret = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    LS_LOG_INFO("gps_handler_stop() api progress %d\n", priv->api_progress_flag);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE)
        priv->api_progress_flag = 0;

    if (priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->pos_timer) {
        g_source_remove(priv->pos_timer);
        priv->pos_timer = 0;
    }

    if (priv->is_started == TRUE) {

        g_return_val_if_fail(priv->gps_plugin->ops.stop, ERROR_NOT_AVAILABLE);

        ret = priv->gps_plugin->ops.stop(priv->gps_plugin->plugin_handler);

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
            priv->ttffstate = FALSE;
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
static int gps_handler_get_position(Handler *self,
                                    gboolean enable,
                                    PositionCallback pos_cb,
                                    gpointer handlerobj,
                                    int handlertype,
                                    LSHandle *sh)
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
                priv->pos_timer = g_timeout_add_seconds(
                GPS_UPDATE_INTERVAL_MAX, gps_timeout_cb, self);

            priv->api_progress_flag |= GET_POSITION_ON;
        }

    } else {
        // Clear the Flag
        priv->api_progress_flag &= ~GET_POSITION_ON;
        LS_LOG_DEBUG("result: gps_handler_get_position Clearing Position\n");
    }

    LS_LOG_INFO("result: gps_handler_get_position %d\n", result);
    return result;
}

static int gps_handler_add_geofence_area(Handler *self,
                                         gboolean enable,
                                         int32_t *geofence_id,
                                         gdouble *latitude,
                                         gdouble *longitude,
                                         gdouble *radius_meters,
                                         GeofenceAddCallBack add_cb,
                                         GeofenceBreachCallback breach_cb,
                                         GeofenceStatusCallback status_cb)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    int result = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    g_return_val_if_fail(add_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(breach_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(status_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.geofence_add_area, ERROR_NOT_AVAILABLE);

    priv->add_cb = add_cb;
    priv->breach_cb = breach_cb;
    priv->geofence_status_cb = status_cb;
    result = priv->gps_plugin->ops.geofence_add_area(priv->gps_plugin->plugin_handler,
                                                     enable,
                                                     geofence_id,
                                                     latitude,
                                                     longitude,
                                                     radius_meters,
                                                     gps_handler_geofence_add_cb,
                                                     gps_handler_geofence_breach_cb,
                                                     gps_handler_geofence_status_cb);

    return result;
}

static int gps_handler_remove_geofence(Handler *self, gboolean enable, int32_t *geofence_id, GeofenceRemoveCallback remove_cb)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    int result = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(remove_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.geofence_process_request, ERROR_NOT_AVAILABLE);

    priv->remove_cb = remove_cb;
    result = priv->gps_plugin->ops.geofence_process_request(priv->gps_plugin->plugin_handler, enable, geofence_id, 0, gps_handler_geofence_remove_cb, HANLDER_GEOFENCE_REMOVE);

    return result;
}

static int gps_handler_resume_geofence(Handler *self, gboolean enable, int32_t *geofence_id, int32_t *monitor, GeofenceResumeCallback resume_cb)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    int result = ERROR_NONE;
    LS_LOG_INFO("abhishek resume called");
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(resume_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.geofence_process_request, ERROR_NOT_AVAILABLE);

    priv->resume_cb = resume_cb;
    result = priv->gps_plugin->ops.geofence_process_request(priv->gps_plugin->plugin_handler, enable, geofence_id, monitor, gps_handler_geofence_resume_cb, HANLDER_GEOFENCE_RESUME);

    return result;
}

static int gps_handler_pause_geofence(Handler *self, gboolean enable, int32_t *geofence_id, GeofencePauseCallback pause_cb)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    int result = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(pause_cb, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.geofence_process_request, ERROR_NOT_AVAILABLE);

    priv->pause_cb = pause_cb;
    result = priv->gps_plugin->ops.geofence_process_request(priv->gps_plugin->plugin_handler, enable, geofence_id, 0, gps_handler_geofence_pause_cb, HANLDER_GEOFENCE_PAUSE);

    return result;
}
/**
 * <Funciton>       gps_handler_start_tracking
 * <Description>    get the position from GPS receiver
 * @param           <self> <In> <Handler Gobject>
 * @param           <self> <In> <Position callback function to get result>
 * @return          int
 */
static void gps_handler_start_tracking(Handler *self,
                                       gboolean enable,
                                       StartTrackingCallBack track_cb,
                                       gpointer handlerobj,
                                       int handlertype,
                                       LSHandle *sh)
{
    LS_LOG_INFO("GPS handler start Tracking called\n");
    int result = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    if ((priv == NULL) || (priv->gps_plugin->ops.get_gps_data == NULL)) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        return;
    }

    if (enable == TRUE && priv->api_progress_flag & START_TRACKING_ON)
        return;


    if (enable) {

        priv->track_cb = track_cb;
        LS_LOG_DEBUG("gps_handler_start_tracking: priv->track_cb %d\n", priv->track_cb);
        if ((!(priv->api_progress_flag & LOCATION_UPDATES_ON))) {

            result = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler,
                                                        enable,
                                                        gps_handler_tracking_cb,
                                                        HANDLER_DATA_POSITION);

            if (result == ERROR_NONE) {
                if (!priv->pos_timer)
                    priv->pos_timer = g_timeout_add_seconds(
                GPS_UPDATE_INTERVAL_MAX, gps_timeout_cb, self);
            }

        } else {
            track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        }
        priv->api_progress_flag |= START_TRACKING_ON;

    } else {
        if ((!(priv->api_progress_flag & LOCATION_UPDATES_ON) )) {

            if (priv->pos_timer) {
               g_source_remove(priv->pos_timer);
               priv->pos_timer = 0;

            }

            priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler,
                                              enable,
                                              gps_handler_tracking_cb,
                                              HANDLER_DATA_POSITION);
        }

        priv->track_cb = NULL;
        priv->api_progress_flag &= ~START_TRACKING_ON;
        LS_LOG_DEBUG("gps_handler_start_tracking Clearing bit\n");
    }
}

static gboolean gps_handler_get_handler_status(Handler *self, int handler_type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, 0);
    return priv->is_started;
}


static void gps_handler_get_location_updates(Handler *self,
                                                gboolean enable,
                                                StartTrackingCallBack loc_update_cb,
                                                gpointer handlerobj,
                                                int handlertype,
                                                LSHandle *sh)
{
    LS_LOG_INFO("GPS handler get location updates called\n");
    int result = ERROR_NONE;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    if ((priv == NULL)) {
        loc_update_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        return;
    }


    if ((enable == TRUE)&& (priv->api_progress_flag & LOCATION_UPDATES_ON))
        return;


    if (enable) {

        priv->loc_update_cb = loc_update_cb;
        LS_LOG_DEBUG("gps_handler_get_location_updates: priv->loc_update_cb %d\n", priv->loc_update_cb);

        if ((!(priv->api_progress_flag & START_TRACKING_ON) )){
            result = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb, HANDLER_DATA_POSITION);

            if (result == ERROR_NONE) {
                if (!priv->pos_timer)
                   priv->pos_timer = g_timeout_add_seconds(GPS_UPDATE_INTERVAL_MAX, gps_timeout_cb, self);
            } else {
                loc_update_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
            }

        }
        priv->api_progress_flag |= LOCATION_UPDATES_ON;

    } else {
        if  ((!(priv->api_progress_flag & START_TRACKING_ON))) {

            if (priv->pos_timer) {
                g_source_remove(priv->pos_timer);
                priv->pos_timer = 0;
            }
            priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb, HANDLER_DATA_POSITION);
        }

        priv->loc_update_cb = NULL;
        priv->api_progress_flag &= ~LOCATION_UPDATES_ON;
        LS_LOG_DEBUG("gps_handler_get_location_updates Clearing bit\n");
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
    if (get_stored_position(position, accuracy, LOCATION_DB_PREF_PATH) == ERROR_NOT_AVAILABLE) {
        LS_LOG_INFO("get last poistion Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

/**
 /**
 * <Funciton >   gps_handler_get_time_to_first_fix
 * <Description>  get the gps TTFF value of GPS
 * @param     <self> <In> <Handler Gobject>
 * @param     <ttff> <In> <resuting TTFF value>
 * @return    int
 */
static guint64 gps_handler_get_time_to_first_fix(Handler *self)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (priv->lastfixtime > priv->fixrequesttime) {
        priv->ttff = priv->lastfixtime - priv->fixrequesttime;
    } else {
        LS_LOG_INFO("[DEBUG] gps_handler_get_time_to_first_fix %lld ", priv->ttff);
    }

    return priv->ttff;
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
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable_satellite) {
        if (priv->api_progress_flag & SATELLITE_GET_DATA_ON)
            return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(priv->gps_plugin->ops.get_gps_data, ERROR_NOT_AVAILABLE);
        g_return_val_if_fail(sat_cb, ERROR_NOT_AVAILABLE);

        priv->sat_cb = sat_cb;
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_satellite, gps_handler_satellite_cb,
                HANDLER_DATA_SATELLITE);

        if (ret == ERROR_NONE) {
            priv->api_progress_flag |= SATELLITE_GET_DATA_ON;
        }
    } else {
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_satellite, gps_handler_satellite_cb,
                HANDLER_DATA_SATELLITE);
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
        g_return_val_if_fail(nmea_cb, ERROR_NOT_AVAILABLE);

        priv->nmea_cb = nmea_cb;
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb, HANDLER_DATA_NMEA);

        if (ret == ERROR_NONE) {
            priv->api_progress_flag |= NMEA_GET_DATA_ON;
        }
    } else {
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb, HANDLER_DATA_NMEA);
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
static int gps_handler_send_extra_command(Handler *self , char *command)
{
    int ret = ERROR_NONE;
    LS_LOG_INFO("gps_handler_send_extra_command");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.send_extra_command, ERROR_NOT_AVAILABLE);

    ret = priv->gps_plugin->ops.send_extra_command(priv->gps_plugin->plugin_handler, command);

    if (strcmp("delete_aiding_data", command) == 0)
        priv->ttffstate = FALSE;

    return ret;
}

/**
 * <Funciton>       gps_handler_get_gps_status
 * <Description>    Register for gps status
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <Handler GObject>
 * @param           <ExtraCmdCallback> <In> <callabck function to get result>
 * @return          int
 */
static int gps_handler_get_gps_status(Handler *self , StatusCallback status_cb)
{
    LS_LOG_INFO("gps_handler_get_gps_status");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(status_cb, ERROR_NOT_AVAILABLE);

    priv->status_cb = status_cb;

    return priv->status;
}
/**
 * <Funciton>       gps_handler_set_gps_parameters
 * <Description>    Set gps parameters
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <Handler GObject>
 * @param           <ExtraCmdCallback> <In> <callabck function to get result>
 * @return          int
 */
static int gps_handler_set_gps_parameters(Handler *self , char *command)
{
    int ret = ERROR_NONE;
    LS_LOG_INFO("gps_handler_set_gps_parameters");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.set_gps_parameters, ERROR_NOT_AVAILABLE);

    ret = priv->gps_plugin->ops.set_gps_parameters(priv->gps_plugin->plugin_handler, command);

    return ret;
}
/**
 * <Funciton >   gps_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
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
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(gobject);

    g_return_if_fail(priv);
    LS_LOG_DEBUG("finalizing gps handler\n");

    if (priv->gps_plugin) {
        plugin_free(priv->gps_plugin, "gps");
        priv->gps_plugin = NULL;
    }

    g_mutex_clear(&priv->mutex);

    memset(priv, 0x00, sizeof(GpsHandlerPrivate));

    G_OBJECT_CLASS(gps_handler_parent_class)->finalize(gobject);
}

static int gps_handler_function_not_implemented(Handler *self, Position *pos, Address *address)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
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
    iface->get_ttfx = (TYPE_GET_TTFF) gps_handler_get_time_to_first_fix;
    iface->get_sat_data = (TYPE_GET_SAT) gps_handler_get_gps_satellite_data;
    iface->get_nmea_data = (TYPE_GET_NMEA) gps_handler_get_nmea_data;
    iface->send_extra_cmd = (TYPE_SEND_EXTRA) gps_handler_send_extra_command;
#ifdef NOMINATIUM_LBS
    iface->get_geo_code = (TYPE_GEO_CODE) gps_handler_function_not_implemented;
    iface->get_rev_geocode = (TYPE_REV_GEO_CODE) gps_handler_function_not_implemented;
#else
    iface->get_google_geo_code = (TYPE_GOOGLE_GEO_CODE) gps_handler_function_not_implemented;
    iface->get_rev_google_geocode = (TYPE_REV_GOOGLE_GEO_CODE) gps_handler_function_not_implemented;
#endif
    iface->get_gps_status = (TYPE_GET_GPS_STATUS) gps_handler_get_gps_status;
    iface->set_gps_params = (TYPE_SET_GPS_PARAMETERS) gps_handler_set_gps_parameters;
    iface->add_geofence_area = (TYPE_ADD_GEOFENCE_AREA) gps_handler_add_geofence_area;
    iface->remove_geofence = (TYPE_REMOVE_GEOFENCE) gps_handler_remove_geofence;
    iface->resume_geofence = (TYPE_RESUME_GEOFENCE) gps_handler_resume_geofence;
    iface->pause_geofence = (TYPE_PAUSE_GEOFENCE) gps_handler_pause_geofence;
    iface->get_location_updates = (TYPE_GET_LOCATION_UPDATES) gps_handler_get_location_updates;
    iface->get_handler_status = (TYPE_GET_HANDLER_STATUS) gps_handler_get_handler_status;
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

    priv->gps_plugin = (GpsPlugin *) plugin_new("gps");
    priv->status = GPS_STATUS_UNAVAILABLE;

    if (priv->gps_plugin == NULL) {
        LS_LOG_ERROR("Failed to load gps plugin\n");
    }

    g_mutex_init(&priv->mutex);
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
