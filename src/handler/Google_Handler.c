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
 * Filename  : Google_Handler.c
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
#include <Google_Handler.h>
#include <Plugin_Loader.h>
#include <curl/curl.h>
#include <cjson/json.h>
#include <Position.h>
#include <Accuracy.h>
#include <LocationService_Log.h>

typedef struct _GoogleHandlerPrivate
{
    gboolean is_started;
    PositionCallback pos_cb;
    StatusCallback status_cb;
    StartTrackingCallBack track_cb;
    gpointer useradata;
    gpointer googledata;
    gpointer nwhandler;
    gint api_progress_flag;
} GoogleHandlerPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HANDLER_TYPE_GOOGLE, GoogleHandlerPrivate))

static void google_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(GoogleHandler, google_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, google_handler_interface_init));

#define GOOGLE_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_GOOGLE, GoogleHandlerPrivate))

static gboolean queryuploader(gpointer userdata);
static gboolean
parse_send_response(char *body, gpointer self, GoogleHandlerMethod method);
static size_t WriteMemoryCallback(void *, size_t, size_t, void *);
struct MemoryStruct
{
    char *memory;
    size_t size;
};

struct MemoryStruct chunk;
/**
 * <Funciton >   google_handler_start
 * <Description>  Callback function for Status ,as per Location_Plugin.h
 * @param     <enable_cb> <In> <Handler Gobject>
 * @param     <status> <In> <Status Info>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void google_handler_status_cb(gboolean enable_cb, Status *status, gpointer privateIns)
{
    LS_LOG_DEBUG("[DEBUG]status_cb callback called  %d \n", enable_cb);
    GoogleHandlerPrivate* priv = GET_PRIVATE(privateIns);
    g_return_if_fail(priv);

}

/**
 * <Funciton >   google_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void google_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_DEBUG(
            "[DEBUG] GPS Handler : tracking_cb  latitude =%f , longitude =%f , accuracy =%f\n", position->latitude, position->longitude, accuracy->horizAccuracy);
    GoogleHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    (*(priv->track_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);

}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void google_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns)
{
    LS_LOG_DEBUG( "[DEBUG]google  haposition_cb callback called  %d \n", enable_cb);
    GoogleHandlerPrivate* priv = GET_PRIVATE(privateIns);

    g_return_if_fail(priv->pos_cb);
    (*priv->pos_cb)(enable_cb, position, accuracy, error, priv->nwhandler, HANDLER_GOOGLE_WIFI_CELL); //call SA position callback

    priv->api_progress_flag &= ~GOOGLE_GET_POSITION_ON;

}
/**
 * <Funciton >   google_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int google_handler_start(Handler *handler_data)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_start Called");
    GoogleHandlerPrivate* priv = GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    return ret;
}

/**o
 * <Funciton >   google_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int google_handler_stop(Handler *self)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_stop() \n");
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    return ret;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from google receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int google_handler_get_position(Handler *self, PositionCallback pos_cb, gpointer agent_userdata, gpointer handler)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_get_position\n");
    int result = ERROR_NONE;
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);

    if (priv->api_progress_flag & GOOGLE_GET_POSITION_ON) {
        return ERROR_DUPLICATE_REQUEST;
    }

    priv->pos_cb = pos_cb;
    priv->nwhandler = handler;
    if (queryuploader(agent_userdata)) {
        parse_send_response(chunk.memory, self, GOOGLE_GET_POSITION);
        // Reviewed by Sathya & Abhishek
        // Freeing the memory once the data from Server is parsed and assinged to local variables.
        free(chunk.memory);
    }
    // To do post quert to curl and get reply

    if (result == ERROR_NONE) {
        priv->api_progress_flag |= GOOGLE_GET_POSITION_ON;
    }
    LS_LOG_DEBUG("[DEBUG]result : google_handler_get_position %d  \n", result);
    return result;
}

static gboolean parse_send_response(char *body, gpointer self, GoogleHandlerMethod method)
{
    double latitude;
    double longitude;
    double accuracy;
    int field_flags = POSITION_FIELDS_NONE;
    int timestamp = 0, altitude = 0.0;
    struct json_object *new_obj;
    struct json_object *acc_obj;
    struct json_object *loc_obj;
    Accuracy* ac;
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);
    LS_LOG_DEBUG("body %s\n", body);
    new_obj = json_tokener_parse(body);

    LS_LOG_DEBUG( "new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));
    acc_obj = json_tokener_parse(body);
    acc_obj = json_object_object_get(acc_obj, "accuracy");
    LS_LOG_DEBUG( "new_obj.to_string()accur=%s\n", json_object_to_json_string(new_obj));
    accuracy = json_object_get_double(acc_obj);
    new_obj = json_object_object_get(new_obj, "location");

    loc_obj = json_object_object_get(new_obj, "lat");

    latitude = json_object_get_double(loc_obj);

    loc_obj = json_object_object_get(new_obj, "lng");

    longitude = json_object_get_double(loc_obj);

    LS_LOG_DEBUG("[DEBUG]lat %f\n", latitude);
    // Reviewed by Sathya & Abhishek
    // Freeing json object
    json_object_put(new_obj);
    json_object_put(acc_obj);

    if (accuracy) {
        ac = accuracy_create(ACCURACY_LEVEL_DETAILED, accuracy, 0);
    } else {
        ac = accuracy_create(ACCURACY_LEVEL_NONE, accuracy, 0);
    }
    LS_LOG_DEBUG("[DEBUG]accuracy %d\n", accuracy);
    field_flags = (POSITION_FIELDS_LATITUDE | POSITION_FIELDS_LONGITUDE | POSITION_FIELDS_ALTITUDE);

    Position* ret_pos = position_create(timestamp, latitude, longitude, altitude, 0.0, 0.0, 0.0, (LocationFields) field_flags);
    if (method == GOOGLE_GET_POSITION) google_handler_position_cb(TRUE, ret_pos, ac, ERROR_NONE, self);
    else google_handler_tracking_cb(TRUE, ret_pos, ac, 0, self, HANDLER_GOOGLE_WIFI_CELL);

    return TRUE;

}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) { /* out of memory! */
        LS_LOG_DEBUG("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static gboolean queryuploader(gpointer userdata)
{

    int *timestamp;
    CURL *curl_handle;
    int res;
    struct curl_slist *slist = NULL;
    curl_handle = curl_easy_init();
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");

    curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyBoS6t0AeIuEYkPcmIsK_bBAVFOWFnoLsw");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
    LS_LOG_DEBUG("[DEBUG] before post query data %s\n", userdata);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, userdata);

    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);
    res = curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);
    LS_LOG_DEBUG("[DEBUG] result=%d data %s\n", res, chunk.memory);
    if (chunk.memory == NULL && res != 0) return FALSE;

    return TRUE;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void google_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer agent_userdata,
                                          gpointer handlerobj)
{
    LS_LOG_DEBUG( "[DEBUG]google handler start Tracking called agent_userdata=%s", (char*)agent_userdata);
    int result = ERROR_NONE;
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);
    if (priv == NULL) track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, NULL, 4);
    priv->nwhandler = handlerobj;

    if (enable) {
        if (priv->api_progress_flag & GOOGLE_START_TRACKING_ON) {
            return;
        }
        priv->track_cb = track_cb;
        // TO DO store it in DB for cache wifi ap list

        // Compare with latest data

        // Post query to curl.
        if (queryuploader(agent_userdata)) {
            parse_send_response(chunk.memory, self, GOOGLE_START_TRACK);
            // Reviewed by Sathya & Abhishek
            // Freeing the memory once the data from Server is parsed and assinged to local variables.
            free(chunk.memory);
        }
        // LS_LOG_DEBUG("[DEBUG] google_handler_start_tracking : priv->track_cb %d  priv->useradata %d \n " ,priv->track_cb ,priv->useradata);

        if (result == ERROR_NONE) {
            priv->api_progress_flag |= GOOGLE_START_TRACKING_ON;
        }
    } else {

        priv->track_cb = NULL;
        //result = pluginOps.start_tracking(priv->google_plugin->plugin_handler, enable ,google_handler_tracking_cb, agent_userdata);
        if (result == ERROR_NONE) {

            priv->api_progress_flag &= ~GOOGLE_START_TRACKING_ON;
        }

    }
    LS_LOG_DEBUG( "[DEBUG] return from google_handler_start_tracking , %d  \n", result);
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int google_handler_get_last_position(Handler *self, Position **position, Accuracy **accuracy)
{
    int ret = 0;
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    //googlePluginOps pluginOps = priv->google_plugin->ops;

    // TO DO read from DB
    return ret;
}

/**
 * <Funciton >   google_handler_get_velocity
 * <Description>  get the velocity value from  the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <VelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int google_handler_get_velocity(Handler *self, VelocityCallback vel_cb)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_get_last_velocity
 * <Description>  get the last velocity from the GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastVelocityCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int google_handler_get_last_velocity(Handler *self, Velocity **velocity, Accuracy **accuracy)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_get_accuracy
 * <Description>  get the accuracy details of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <AccuracyCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int google_handler_get_accuracy(Handler *self, AccuracyCallback acc_cb)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_get_power_requirement
 * <Description>  get the power requirement of the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <power> <In> <power requireemnt of the GPS handler>
 * @return    int
 */
static int google_handler_get_power_requirement(Handler *self, int* power)
{
    int ret = 0;
    return ret;
}

/**
 * <Funciton >   google_handler_get_time_to_first_fix
 * <Description>  get the google TTFF value of GPS
 * @param     <self> <In> <Handler Gobject>
 * @param     <ttff> <In> <resuting TTFF value>
 * @return    int
 */
static int google_handler_get_time_to_first_fix(Handler *self, double* ttff)
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
static int google_handler_get_gps_satellite_data(Handler *self, gboolean enable_satellite, SatelliteCallback sat_cb)
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
static int google_handler_get_nmea_data(Handler *self, gboolean enable_nmea, NmeaCallback nmea_cb, gpointer agent_userdata)
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
static int google_handler_send_extra_command(Handler *self, int* command, ExtraCmdCallback extra_cb)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_set_property
 * <Description>  set the proeprty value for the given property id
 * @param     <self> <In> <Handler Gobject>
 * @param     <object> <In> < Gobject>
 * @param     <property_id> <In> <property key >
 * @param     <pspec> <In> <pspec>
 * @return    int
 */
static int google_handler_set_property(Handler* self, GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
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
static int google_handler_get_property(Handler* self, GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
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
static int google_handler_get_current_handler(Handler *self, int handlerType)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_set_current_handler
 * <Description>  set the current handler to Hybrid handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <handlerType> <In> <id of handler to set>
 * @return    int
 */
static int google_handler_set_current_handler(Handler *self, int handlerType)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_compare_handler
 * <Description>  compare the handler1 & handler 2 and find the best accurate result
 * @param     <self> <In> <Handler Gobject>
 * @param     <handler1> <In> <id of handler1>
 * @param     <handler2> <In> <id of handler2>
 * @return    int
 */
static int google_handler_compare_handler(Handler *self, int handler1, int handler2)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_geo_code
 * <Description>  Convert the given address to Latitide & longitude
 * @param     <Address> <In> <address to geocodet>
 * @param     <GeoCodeCallback> <In> <callback function to get result>
 * @return    int
 */
static int google_handler_geo_code(Handler *self, Address* address, Position *pos, Accuracy *ac)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_reverse_geo_code
 * <Description>  Convert the given position values to the Address
 *      and provide the result in callback
 * @param     <self> <In> <Handler Gobject>
 * @param     <Position> <In> <position to  rev geocode>
 * @param     <RevGeocodeCallback> <In> <Handler Gobject>
 * @return    int
 */
static int google_handler_reverse_geo_code(Handler *self, Position* pos, Address * address)
{
    int ret = ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
    return ret;
}

/**
 * <Funciton >   google_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void google_handler_dispose(GObject *gobject)
{
    GoogleHandlerPrivate* priv = GET_PRIVATE(gobject);

    G_OBJECT_CLASS(google_handler_parent_class)->dispose(gobject);

}

/**
 * <Funciton >   google_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void google_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_finalize");
    G_OBJECT_CLASS(google_handler_parent_class)->finalize(gobject);
}

/**
 * <Funciton >   google_handler_interface_init
 * <Description>  google interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void google_handler_interface_init(HandlerInterface *interface)
{
    interface->start = (TYPE_START_FUNC) google_handler_start;
    interface->stop = (TYPE_STOP_FUNC) google_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) google_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) google_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) google_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) google_handler_get_velocity;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) google_handler_get_last_velocity;
    interface->get_accuracy = (TYPE_GET_ACCURACY) google_handler_get_accuracy;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) google_handler_get_power_requirement;
    interface->get_ttfx = (TYPE_GET_TTFF) google_handler_get_time_to_first_fix;
    interface->get_sat_data = (TYPE_GET_SAT) google_handler_get_gps_satellite_data;
    interface->get_nmea_data = (TYPE_GET_NMEA) google_handler_get_nmea_data;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) google_handler_send_extra_command;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) google_handler_get_current_handler;
    interface->set_cur_handler = (TYPE_SET_CUR_HANDLER) google_handler_set_current_handler;
    interface->compare_handler = (TYPE_COMP_HANDLER) google_handler_compare_handler;
    interface->get_geo_code = (TYPE_GEO_CODE) google_handler_geo_code;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) google_handler_reverse_geo_code;
}

/**
 * <Funciton >   google_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void google_handler_init(GoogleHandler *self)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_init");
    GoogleHandlerPrivate* priv = GET_PRIVATE(self);
    memset(priv, 0x00, sizeof(priv));
}

/**
 * <Funciton >   google_handler_class_init
 * <Description>  Gobject class init
 * @param     <googleHandlerClass> <In> <googleClass instance>
 * @return    int
 */
static void google_handler_class_init(GoogleHandlerClass *klass)
{
    LS_LOG_DEBUG("[DEBUG]google_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = google_handler_dispose;
    gobject_class->finalize = google_handler_finalize;

    g_type_class_add_private(klass, sizeof(GoogleHandlerPrivate));

}
