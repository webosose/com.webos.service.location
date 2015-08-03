/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Google_LBS_Plugin.c
 * Purpose  : Plugin to the LBSHandler ,implementation based on  D-Bus Geoclue LBS service.
 * Platform  : RedRose
 * Author(s) : Abhishek Srivastava
 * Email ID. : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Location_Plugin.h>
#include <Location.h>
#include <Address.h>
#include <Position.h>
#include <geoclue/geoclue-google-geocoder.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <loc_log.h>

/*All g-signals indexes*/
enum {
    SIGNAL_GEO_RESPONSE,
    SIGNAL_REVGEO_RESPONSE,
    SIGNAL_END
};

/**
 * Local GPS plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeoclueGoogleGeocoder *google_geocoder;
    GeoCodeCallback geocode_cb;
    GeoCodeCallback rev_geocode_cb;
    char *license_key;
    gpointer userdata;
    gulong signal_handler_ids[SIGNAL_END];
} GeoclueLbs;

#define LGE_GOOGLE_LBS_SERVICE_NAME "org.freedesktop.Geoclue.Providers.LgeLbsService"
#define LGE_GOOGLE_LBS_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/LgeLbsService"

static void geocoder_cb(GeoclueGoogleGeocoder *geocoder, int error, char *response, gpointer userdata);
static void rev_geocoder_cb(GeoclueGoogleGeocoder *geocoder, int error, char *response, gpointer userdata);

static void signal_connection(GeoclueLbs *geoclueLbs, int signal_type, gboolean connect)
{
    gpointer signal_instance = NULL;
    gchar *signal_name = NULL;
    GCallback signal_cb = NULL;

    if (geoclueLbs == NULL)
        return;

    if (signal_type < 0 || signal_type >= SIGNAL_END)
        return;

    switch (signal_type) {
        case SIGNAL_GEO_RESPONSE:
            signal_instance = G_OBJECT(geoclueLbs->google_geocoder);
            signal_name = "response-changed";
            signal_cb = G_CALLBACK(geocoder_cb);
            break;
        case SIGNAL_REVGEO_RESPONSE:
            signal_instance = G_OBJECT(geoclueLbs->google_geocoder);
            signal_name = "response-changed";
            signal_cb = G_CALLBACK(rev_geocoder_cb);
            break;
        default:
            break;
    }

    if (signal_instance == NULL)
        return;

    if (connect) {
        if (!g_signal_handler_is_connected(signal_instance,
                                           geoclueLbs->signal_handler_ids[signal_type])) {
            geoclueLbs->signal_handler_ids[signal_type] = g_signal_connect(signal_instance,
                                                                           signal_name,
                                                                           signal_cb,
                                                                           geoclueLbs);
            LS_LOG_INFO("Connecting singal [%d:%s] %lu\n",
                        signal_type,
                        signal_name,
                        geoclueLbs->signal_handler_ids[signal_type]);
        } else {
            LS_LOG_INFO("Already connected signal [%d:%s] %lu\n",
                        signal_type,
                        signal_name,
                        geoclueLbs->signal_handler_ids[signal_type]);
        }
    } else {
        g_signal_handler_disconnect(signal_instance,
                                    geoclueLbs->signal_handler_ids[signal_type]);
        LS_LOG_INFO("Disconnecting singal [%d:%s] %lu\n",
                    signal_type,
                    signal_name,
                    geoclueLbs->signal_handler_ids[signal_type]);

        geoclueLbs->signal_handler_ids[signal_type] = 0;
    }
}

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueLbs> <In> <LBS plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueLbs *geoclueLbs)
{
    if (geoclueLbs->google_geocoder != NULL) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(geoclueLbs->google_geocoder),
                                             G_CALLBACK(rev_geocoder_cb),
                                             geoclueLbs);

        g_signal_handlers_disconnect_by_func(G_OBJECT(geoclueLbs->google_geocoder),
                                             G_CALLBACK(geocoder_cb),
                                             geoclueLbs);

        g_object_unref(geoclueLbs->google_geocoder);
        geoclueLbs->google_geocoder = NULL;
    }

    memset(geoclueLbs->signal_handler_ids, 0, sizeof(gulong)*SIGNAL_END);
}

static void geocoder_cb (GeoclueGoogleGeocoder *geocoder, int error , char *response, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error == 0){
        (*geoclueLbs->geocode_cb)(TRUE, NULL, ERROR_NETWORK_ERROR, geoclueLbs->userdata, HANDLER_LBS);
    } else if (error == 200) {
        (*geoclueLbs->geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
    } else {
        (*geoclueLbs->geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
    }
}

static void rev_geocoder_cb (GeoclueGoogleGeocoder *geocoder, int error , char *response, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error == 0){
        (*geoclueLbs->rev_geocode_cb)(TRUE, NULL, ERROR_NETWORK_ERROR, geoclueLbs->userdata, HANDLER_LBS);
    } else if (error == 200) { //when data connection is slow
        (*geoclueLbs->rev_geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
    } else {
        (*geoclueLbs->rev_geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
    }

}

void geocoder_cb_async (GeoclueGoogleGeocoder *geocoder, char *response, GError *error, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error) {
        if (error->code == 4) { //dbus timeout
            signal_connection(geoclueLbs, SIGNAL_GEO_RESPONSE, TRUE);
        } else {
            (*geoclueLbs->geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
        }

        LS_LOG_DEBUG("[DEBUG] geocoder_cb_async  %d%s\n",error->code, error->message);
        g_error_free(error);
    } else {
        (*geoclueLbs->geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
        if (response) {
            g_free(response);
            response = NULL;
        }
    }

}

void rev_geocoder_cb_async (GeoclueGoogleGeocoder *geocoder, char *response, GError *error, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error) {
        if (error->code == 4) {
            signal_connection(geoclueLbs, SIGNAL_REVGEO_RESPONSE, TRUE);
        } else {
            (*geoclueLbs->rev_geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
        }

        LS_LOG_DEBUG("[DEBUG] rev_geocoder_cb_async  %d%s\n", error->code, error->message);
        g_error_free(error);
    } else {
        (*geoclueLbs->rev_geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
        if (response) {
            g_free(response);
            response = NULL;
        }
    }

}

/**
 **
 * <Funciton >   send_geoclue_command
 * <Description>  send geoclue command
 * @return    int
 */
static gboolean send_geoclue_command(GeoclueGoogleGeocoder *instance, gchar *key, gchar *value)
{
    GHashTable *options;
    GValue *gvalue;
    GError *error = NULL;
    options = g_hash_table_new(g_str_hash, g_str_equal);
    gvalue = g_new0(GValue, 1);

    g_value_init(gvalue, G_TYPE_STRING);
    g_value_set_string(gvalue, value);
    g_hash_table_insert(options, key, gvalue);

    if (!geoclue_provider_set_options(GEOCLUE_PROVIDER(instance), options, &error)) {
        LS_LOG_ERROR("[LBS Error geoclue_provider_set_options(%s) : %s", key, error->message);
        g_value_unset(gvalue);
        g_error_free(error);
        g_free(gvalue);
        g_hash_table_destroy(options);
        return FALSE;
    }

    LS_LOG_INFO("LBS Success to geoclue_provider_set_options(%s)", key);
    g_value_unset(gvalue);
    g_free(gvalue);
    g_hash_table_destroy(options);

    return TRUE;
}

int get_google_geocode(gpointer handle, const char *data, GeoCodeCallback geocode_cb)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueLbs->google_geocoder, "GEOCODING", geoclueLbs->license_key) == TRUE) {
        LS_LOG_WARNING("License key Sent Success\n");
    } else
        return ERROR_NOT_AVAILABLE;

    geoclueLbs->geocode_cb = geocode_cb;
    geoclue_google_geocoder_process_geocoder_request_async (geoclueLbs->google_geocoder,
            data,
            (GeoclueGoogleGeocoderCallback)geocoder_cb_async,
            geoclueLbs);

    return ERROR_NONE;
}

int get_reverse_google_geocode(gpointer handle, const char *data, GeoCodeCallback rev_geocode_cb)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    if (send_geoclue_command(geoclueLbs->google_geocoder, "GEOCODING", geoclueLbs->license_key) == TRUE) {
        LS_LOG_WARNING("License key Sent Success\n");
    } else
        return ERROR_NOT_AVAILABLE;

    geoclueLbs->rev_geocode_cb = rev_geocode_cb;
    geoclue_google_geocoder_process_geocoder_request_async (geoclueLbs->google_geocoder,
            data,
            (GeoclueGoogleGeocoderCallback)rev_geocoder_cb_async,
            geoclueLbs);

    return ERROR_NONE;
}

/**
 * <Funciton >   intialize_lbs_geoclue_service
 * <Description>  create the GPS geoclue service instance
 * @param     <GeoclueLbs> <In> <LBS plugin structure>
 * @return    gboolean
 */
static gboolean intialize_lbs_geoclue_service(GeoclueLbs *geoclueLbs)
{
    g_return_val_if_fail(geoclueLbs, FALSE);

    //Check its alreasy started
    if (geoclueLbs->google_geocoder) {
        return TRUE;
    }

    /*
     *  Create Geoclue geocoder
     */

    geoclueLbs->google_geocoder = geoclue_google_geocoder_new(LGE_GOOGLE_LBS_SERVICE_NAME, LGE_GOOGLE_LBS_SERVICE_PATH);

    if (geoclueLbs->google_geocoder == NULL) {
        LS_LOG_ERROR("[DEBUG] Error while creating LG Lbs geoclue object !!");
        unreference_geoclue(geoclueLbs);
        return FALSE;
    }

    LS_LOG_INFO("[DEBUG] intialize_Lbs_geoclue_service  done\n");

    return TRUE;
}

/**
 * <Funciton >   start
 * <Description>  start the plugin by resgistering the callbacks
 * @param     <StatusCallback> <In> <enable/disable the callback>
 * @param     <PositionCallback> <In> <extra commad identifier>
 * @param     <VelocityCallback> <In> <Gobject private instance>
 * @param     <SatelliteCallback> <In> <Gobject private instance>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
static int start(gpointer plugin_data, gpointer handler_data, const char *license_key)
{
    LS_LOG_INFO("[DEBUG] LBS plugin start  plugin_data : %p  ,handler_data :%p \n", plugin_data, handler_data);
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) plugin_data;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    geoclueLbs->userdata = handler_data;

    if (intialize_lbs_geoclue_service(geoclueLbs) == FALSE) {
        return ERROR_NOT_AVAILABLE;
    }

    geoclueLbs->license_key = g_strdup(license_key);

    return ERROR_NONE;
}

/**
 * <Funciton >   stop
 * <Description>  stop the plugin
 * @param     <handle> <In> <enable/disable the callback>
 * @return    int
 */
static int stop(gpointer handle)
{
    LS_LOG_INFO("[DEBUG]lbs plugin stop\n");
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;

    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    unreference_geoclue(geoclueLbs);

    if (geoclueLbs->license_key != NULL)
        g_free(geoclueLbs->license_key);

    return ERROR_NONE;
}

/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <LBSPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(LbsPluginOps *ops)
{
    LS_LOG_INFO("[DEBUG]Lbs plugin init\n");
    ops->start = start;
    ops->stop = stop;
    ops->get_google_geocode = get_google_geocode;
    ops->get_reverse_google_geocode = get_reverse_google_geocode;

    /*
     * Handler for plugin
     */
    GeoclueLbs *lbs = g_new0(GeoclueLbs, 1);

    if (lbs == NULL)
        return NULL;

    return (gpointer) lbs;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API void shutdown(gpointer handle)
{
    LS_LOG_INFO("[DEBUG]lbs plugin shutdown\n");
    g_return_if_fail(handle);

    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    unreference_geoclue(geoclueLbs);

    g_free(geoclueLbs);
}

