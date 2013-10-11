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
#include <time.h>
#include <Location.h>
#include <cjson/json.h>

#define GPS_UPDATE_INTERVAL_MAX     12*60

typedef struct _GpsHandlerPrivate {
    GpsPlugin *gps_plugin;
    gboolean is_started;
    PositionCallback pos_cb;
    StatusCallback status_cb;
    VelocityCallback vel_cb;
    SatelliteCallback sat_cb;

    NmeaCallback nmea_cb;
    StartTrackingCallBack track_cb;

    guint pos_timer;
    guint log_handler;
    gint api_progress_flag;
    guint64 fixrequesttime;
    guint64 ttff;
    guint64 lastfixtime;
    gboolean ttffstate;
    GMutex *mutex;
    gboolean network_state;
    gboolean telephony_api_state;
    LSHandle *sh;
    LSMessage *message;
    LSMessageToken m_telInfoReq;
} GpsHandlerPrivate;

#define GPS_HANDLER_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_GPS, GpsHandlerPrivate))

static void handler_interface_init(HandlerInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GpsHandler, gps_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE,
                        handler_interface_init));

static gboolean gps_timeout_cb(gpointer user_data)
{
    LS_LOG_DEBUG("timeout of gps position update\n");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_val_if_fail(priv, G_SOURCE_REMOVE);
    // Post to agent that Gps timeout happend
    LocationFields field = (POSITION_FIELDS_LATITUDE | POSITION_FIELDS_LONGITUDE | POSITION_FIELDS_ALTITUDE);
    Position *position = position_create(0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, field);
    Accuracy *accuracy = accuracy_create(ACCURACY_LEVEL_NONE, 0.0, 0.0);

    if (priv->pos_cb)(*priv->pos_cb)(TRUE, position, accuracy, ERROR_TIMEOUT, priv, HANDLER_GPS);

    if (priv->track_cb)(*priv->track_cb)(TRUE, position, accuracy, ERROR_TIMEOUT, priv, HANDLER_GPS);

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
    LS_LOG_DEBUG("gps_handler_status_cb status = %d", status);
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->status_cb);
    priv->status_cb(enable_cb, status, user_data);

    if (status == GPS_STATUS_AVAILABLE) {
        // remove timeout timer.
        // This will be implemented with timeout and agent sync
        if (priv->pos_timer) {
            g_source_remove(priv->pos_timer);
            priv->pos_timer = 0;
        }
    }
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
void gps_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer user_data,
                             int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);

    if (priv->ttffstate == TRUE) {
        priv->lastfixtime = time(0);
        priv->ttffstate == FALSE;
    }

    if (position != NULL) {
        LS_LOG_DEBUG(
            "GPS Handler: tracking_cb latitude = %f, longitude = %f, accuracy = %f\n", position->latitude, position->longitude,
            accuracy->horizAccuracy);
    }

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
void gps_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer user_data,
                             int type)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(user_data);
    g_return_if_fail(priv);

    if (priv->ttffstate == TRUE) {
        priv->lastfixtime = time(0);
        priv->ttffstate == FALSE;
    }

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
    g_return_val_if_fail(priv->mutex, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->plugin_handler, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("gps_handler_start\n");
    g_mutex_lock(priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(priv->mutex);
        return ERROR_NONE;
    }
    ret = priv->gps_plugin->ops.start(priv->gps_plugin->plugin_handler, gps_handler_status_cb, self);
	if(ret == ERROR_NONE) {
      priv->fixrequesttime = time(0);
      priv->ttffstate = TRUE;
      priv->is_started = TRUE;
	}
    g_mutex_unlock(priv->mutex);
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
    LS_LOG_DEBUG("gps_handler_stop() api progress %d\n", priv->api_progress_flag);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE) priv->api_progress_flag = 0;

    if (priv->api_progress_flag != 0) return ERROR_REQUEST_INPROGRESS;

    priv->telephony_api_state = FALSE;

    if (priv->pos_timer) {
        g_source_remove(priv->pos_timer);
        priv->pos_timer = 0;
    }

    if (priv->is_started == TRUE) {
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
static int gps_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handlerobj,
                                    int handlertype,
                                    LSHandle *sh)
{
    int result = ERROR_NONE;
    gboolean mRet;
    LSError lserror;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    LS_LOG_DEBUG("gps_handler_get_position\n");

    if (enable) {
        if (priv->api_progress_flag & GET_POSITION_ON) return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(pos_cb, ERROR_NOT_AVAILABLE);
        g_return_val_if_fail(priv->gps_plugin->ops.get_position, ERROR_NOT_AVAILABLE);

        if (priv->telephony_api_state == FALSE) {
            if (TRUE == request_data_conn_state(sh, self, TRUE))
                priv->telephony_api_state = TRUE;
        }

        priv->pos_cb = pos_cb;
        result = priv->gps_plugin->ops.get_position(priv->gps_plugin->plugin_handler, gps_handler_position_cb);

        if (result == ERROR_NONE) {
            if (!priv->pos_timer) priv->pos_timer = g_timeout_add_seconds(
                        GPS_UPDATE_INTERVAL_MAX,
                        gps_timeout_cb, self);

            priv->api_progress_flag |= GET_POSITION_ON;
        }

        LS_LOG_DEBUG("result: gps_handler_get_position %d\n", result);
    } else {
        // Clear the Flag
        priv->api_progress_flag &= ~GET_POSITION_ON;

        if (priv->m_telInfoReq != DEFAULT_VALUE && !(priv->api_progress_flag & START_TRACKING_ON)) {
            mRet = LSCallCancel(priv->sh, priv->m_telInfoReq, &lserror);
            LS_LOG_DEBUG("[DEBUG] LSCallCancel return from GPS_handler_start_tracking   %d \n", mRet);
            priv->m_telInfoReq = DEFAULT_VALUE;
            priv->telephony_api_state = FALSE;
        }

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
static void gps_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb,
                                       gpointer handlerobj,
                                       int handlertype, LSHandle *sh)
{
    LS_LOG_DEBUG("GPS handler start Tracking called\n");
    int result = ERROR_NONE;
    gboolean mRet;
    LSError lserror;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);

    if ((priv == NULL) || (priv->gps_plugin->ops.get_gps_data == NULL)) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        return;
    }

    if (enable) {
        if (priv->api_progress_flag & START_TRACKING_ON) return;
    }

    priv->track_cb = NULL;

    if (enable) {
        priv->track_cb = track_cb;
        LS_LOG_DEBUG("gps_handler_start_tracking: priv->track_cb %d\n", priv->track_cb);

        if (priv->telephony_api_state == FALSE) {
            LS_LOG_DEBUG("gps_handler_start_tracking: telephony_api_state");

            if (TRUE == request_data_conn_state(sh, self, TRUE)) {
                LS_LOG_DEBUG("gps_handler_start_tracking: telephony_api_state ..2");
                priv->telephony_api_state = TRUE;
            }
        }

        result = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb,
                 HANDLER_DATA_POSITION);

        if (result == ERROR_NONE) {
            if (!priv->pos_timer) priv->pos_timer = g_timeout_add_seconds(
                        GPS_UPDATE_INTERVAL_MAX,
                        gps_timeout_cb, self);

            priv->api_progress_flag |= START_TRACKING_ON;
        } else {
            track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, HANDLER_GPS);
        }
    } else {
        if (priv->pos_timer) {
            g_source_remove(priv->pos_timer);
            priv->pos_timer = 0;
        }

        priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable, gps_handler_tracking_cb,
                                           HANDLER_DATA_POSITION);
        priv->api_progress_flag &= ~START_TRACKING_ON;

        if (priv->m_telInfoReq != DEFAULT_VALUE && !(priv->api_progress_flag & GET_POSITION_ON)) {
            mRet = LSCallCancel(priv->sh, priv->m_telInfoReq, &lserror);
            LS_LOG_DEBUG("[DEBUG] LSCallCancel return from GPS_handler_start_tracking   %d \n", mRet);
            priv->m_telInfoReq = DEFAULT_VALUE;
            priv->telephony_api_state = FALSE;
        }

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
    if (gps_plugin_get_stored_position(position, accuracy) == ERROR_NOT_AVAILABLE) {
        LS_LOG_DEBUG("get last poistion Failed to read\n");
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
        LS_LOG_DEBUG("[DEBUG] gps_handler_get_time_to_first_fix %lld ", priv->ttff);
    }

    return priv->ttff;
}

gboolean data_conn_cb(LSHandle *sh, LSMessage *reply, void *ctx)
{
    struct json_object *new_obj;
    struct json_object *return_value;
    struct json_object *datastate;
    gboolean ret;
    char *state;
    int error = ERROR_NONE;
    struct json_object *error_code = NULL;
    LS_LOG_DEBUG("result: data_conn_cb \n");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE((Handler *) ctx);
    g_return_if_fail(priv);
    const char *str = LSMessageGetPayload(reply);
    struct json_object *data_obj = json_object_new_object();
    new_obj = json_tokener_parse(str);
    return_value = json_object_object_get(new_obj, "returnValue");
    ret = json_object_get_boolean(return_value);

    if (ret == FALSE) {
        LS_LOG_DEBUG("result: data_conn_cb not aviavle \n");
        priv->network_state = FALSE;
        error_code = json_object_object_get(new_obj, "errorCode");
        error = json_object_get_int(error_code);

        if (error == -1) {
            if (priv->api_progress_flag & START_TRACKING_ON) {
                request_data_conn_state(sh, ctx, TRUE);

                if (priv->api_progress_flag & GET_POSITION_ON) {
                    g_return_if_fail(priv->pos_cb);
                    (*priv->pos_cb)(TRUE , NULL, NULL, ERROR_NETWORK_ERROR, priv, HANDLER_GPS);//call SA position callback
                    priv->api_progress_flag &= ~GET_POSITION_ON;
                }
            }

            json_object_object_add(data_obj, "network", json_object_new_boolean(priv->network_state));
            json_object_object_add(data_obj, "apn", json_object_new_string("dummy"));
            json_object_object_add(data_obj, "pdptype", json_object_new_string("IPV4"));
            priv->gps_plugin->ops.send_network_data(priv->gps_plugin->plugin_handler,
                                                    (gpointer) json_object_to_json_string(data_obj));
        }
    } else {
        datastate = json_object_object_get(new_obj, "state");
        state = json_object_get_string(datastate);

        if (strcmp(state, "active") == 0)  {
            LS_LOG_DEBUG("result: data_conn_cb avialable \n");
            priv->network_state = TRUE;
            request_data_conn_state(sh, ctx, FALSE);
        }
    }

    json_object_put(new_obj);
    json_object_put(data_obj);
    return TRUE;
}
gboolean apn_cb(LSHandle *sh, LSMessage *reply, void *ctx)
{
    struct json_object *new_obj;
    struct json_object *return_value;
    struct json_object *extended;
    struct json_object *apndata;
    struct json_object *pdptype;
    char *apn;
    char *pdp;
    gboolean ret;
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE((Handler *) ctx);
    g_return_if_fail(priv);
    const char *str = LSMessageGetPayload(reply);
    new_obj = json_tokener_parse(str);
    return_value = json_object_object_get(new_obj, "returnValue");
    ret = json_object_get_boolean(return_value);

    if (ret == FALSE) {
        json_object_put(new_obj);
        return ret;
    }

    extended = json_object_object_get(new_obj, "extended");
    apndata = json_object_object_get(extended, "apn");
    pdptype = json_object_object_get(extended, "pdp_type");
    apn = json_object_get_string(apndata);
    pdp = json_object_get_string(pdptype);
    LS_LOG_DEBUG("apn_cb apn %s pdp %s ", apn, pdp);
    // send apn name and type to lower layer
    struct json_object *data_obj = json_object_new_object();
    json_object_object_add(data_obj, "network", json_object_new_boolean(priv->network_state));
    json_object_object_add(data_obj, "apn", json_object_new_string(apn));
    json_object_object_add(data_obj, "pdptype", json_object_new_string(pdp));
    priv->gps_plugin->ops.send_network_data(priv->gps_plugin->plugin_handler,
                                            (gpointer) json_object_to_json_string(data_obj));
    json_object_put(new_obj);
    json_object_put(data_obj);
    return ret;
}
gboolean request_data_conn_state(LSHandle *sh, gpointer self, gboolean data)
{
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, FALSE);

    if (data == TRUE) {
        LS_LOG_DEBUG("request_data_conn_state");
        return LSCall(sh, "palm://com.palm.telephony/dataConnectionStatusQuery", "{\"subscribe\":true}", data_conn_cb, self,
                      &priv->m_telInfoReq, NULL);
    }

    LS_LOG_DEBUG("request_data_conn_state...1");
    return LSCall(sh, "palm://com.palm.telephony/getRuntimeSetting", "{}", apn_cb, self, NULL, NULL);
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
        if (priv->api_progress_flag & SATELLITE_GET_DATA_ON) return ERROR_DUPLICATE_REQUEST;

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
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb,
                HANDLER_DATA_NMEA);

        if (ret == ERROR_NONE) {
            priv->api_progress_flag |= NMEA_GET_DATA_ON;
        }
    } else {
        ret = priv->gps_plugin->ops.get_gps_data(priv->gps_plugin->plugin_handler, enable_nmea, gps_handler_nmea_cb,
                HANDLER_DATA_NMEA);
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
    LS_LOG_DEBUG("gps_handler_send_extra_command");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->gps_plugin->ops.send_extra_command, ERROR_NOT_AVAILABLE);
    ret = priv->gps_plugin->ops.send_extra_command(priv->gps_plugin->plugin_handler, command);
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
    LS_LOG_DEBUG("gps_handler_get_gps_status");
    GpsHandlerPrivate *priv = GPS_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(status_cb, ERROR_NOT_AVAILABLE);
    priv->status_cb = status_cb;
    return  ERROR_NONE;
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

    if (priv->pos_timer) g_source_remove(priv->pos_timer);

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

    if (priv->mutex != NULL) {
        g_mutex_free(priv->mutex);
    }

    g_log_remove_handler(NULL, priv->log_handler);
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
    iface->get_velocity = (TYPE_GET_VELOCITY) gps_handler_function_not_implemented;
    iface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) gps_handler_function_not_implemented;
    iface->get_accuracy = (TYPE_GET_ACCURACY) gps_handler_function_not_implemented;
    iface->get_power_requirement = (TYPE_GET_POWER_REQ) gps_handler_function_not_implemented;
    iface->get_ttfx = (TYPE_GET_TTFF) gps_handler_get_time_to_first_fix;
    iface->get_sat_data = (TYPE_GET_SAT) gps_handler_get_gps_satellite_data;
    iface->get_nmea_data = (TYPE_GET_NMEA) gps_handler_get_nmea_data;
    iface->send_extra_cmd = (TYPE_SEND_EXTRA) gps_handler_send_extra_command;
    iface->get_cur_handler = (TYPE_GET_CUR_HANDLER) gps_handler_function_not_implemented;
    iface->set_cur_handler = (TYPE_SET_CUR_HANDLER) gps_handler_function_not_implemented;
    iface->compare_handler = (TYPE_COMP_HANDLER) gps_handler_function_not_implemented;
    iface->get_geo_code = (TYPE_GEO_CODE) gps_handler_function_not_implemented;
    iface->get_rev_geocode = (TYPE_REV_GEO_CODE) gps_handler_function_not_implemented;
    iface->get_gps_status = (TYPE_GET_GPS_STATUS) gps_handler_get_gps_status;
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

    if (priv->gps_plugin == NULL) {
        LS_LOG_ERROR("Failed to load gps plugin\n");
    }

    priv->mutex = g_mutex_new();

    if (priv->mutex == NULL)
        LS_LOG_ERROR("Failed in allocating mutex\n");

    priv->log_handler = g_log_set_handler(NULL, G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
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
