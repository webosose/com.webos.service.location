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
#include <Location_Plugin.h>
#include <Location.h>
#include <Address.h>
#include <Position.h>
#include <geoclue/geoclue-google-geocoder.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <LocationService_Log.h>

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

    gpointer userdata;
} GeoclueLbs;

#define LGE_GOOGLE_LBS_SERVICE_NAME "org.freedesktop.Geoclue.Providers.LgeLbsService"
#define LGE_GOOGLE_LBS_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/LgeLbsService"

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueLbs> <In> <LBS plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueLbs *geoclueLbs)
{
    if (geoclueLbs->google_geocoder != NULL) {
        g_object_unref(geoclueLbs->google_geocoder);
        geoclueLbs->google_geocoder = NULL;
    }
}

void geocoder_cb (GeoclueGoogleGeocoder *geocoder, int error ,const char *response, gpointer userdata)
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


void rev_geocoder_cb (GeoclueGoogleGeocoder *geocoder, int error ,const char *response, gpointer userdata)
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

void geocoder_cb_async (GeoclueGoogleGeocoder *geocoder, const char *response, GError *error, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error) {
        if (error->code == 4) { //dbus timeout
            g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueLbs->google_geocoder)),
                             "response-changed",
                             G_CALLBACK(geocoder_cb), geoclueLbs);
        } else {
            (*geoclueLbs->geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
        }

        LS_LOG_DEBUG("[DEBUG] geocoder_cb_async  %d%s\n",error->code, error->message);
        g_error_free(error);
    } else {
        (*geoclueLbs->geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
    }
}

void rev_geocoder_cb_async (GeoclueGoogleGeocoder *geocoder, const char *response, GError *error, gpointer userdata)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) userdata;
    g_return_if_fail(geoclueLbs);

    if (error) {
        if (error->code == 4) {
            g_signal_connect(G_OBJECT(GEOCLUE_PROVIDER(geoclueLbs->google_geocoder)),
                             "response-changed",
                             G_CALLBACK(rev_geocoder_cb), geoclueLbs);
        } else {
            (*geoclueLbs->rev_geocode_cb)(TRUE, NULL, ERROR_NOT_AVAILABLE, geoclueLbs->userdata, HANDLER_LBS);
        }

        LS_LOG_DEBUG("[DEBUG] rev_geocoder_cb_async  %d%s\n", error->code, error->message);
        g_error_free(error);
    } else {
        (*geoclueLbs->rev_geocode_cb)(TRUE, response, ERROR_NONE, geoclueLbs->userdata, HANDLER_LBS);
    }
}


int get_google_geocode(gpointer handle, const char *data, GeoCodeCallback geocode_cb)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

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
static int start(gpointer plugin_data, gpointer handler_data)
{
    LS_LOG_INFO("[DEBUG] LBS plugin start  plugin_data : %d  ,handler_data :%d \n", plugin_data, handler_data);
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) plugin_data;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    geoclueLbs->userdata = handler_data;

    if (intialize_lbs_geoclue_service(geoclueLbs) == FALSE) {
        return ERROR_NOT_AVAILABLE;
    }

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

    g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueLbs->google_geocoder)),
                                         G_CALLBACK(rev_geocoder_cb),
                                         geoclueLbs);

    g_signal_handlers_disconnect_by_func(G_OBJECT(GEOCLUE_PROVIDER(geoclueLbs->google_geocoder)),
                                         G_CALLBACK(geocoder_cb),
                                         geoclueLbs);
    unreference_geoclue(geoclueLbs);

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

