/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : LBS_Plugin.c
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
#include <Position.h>
#include <Accuracy.h>
#include <geoclue/geoclue-geocode.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <LocationService_Log.h>

// PmLogging
#define LS_LOG_CONTEXT_NAME     "avocado.location.lbs_plugin"
PmLogContext gLsLogContext;

/**
 * Local GPS plugin structure
 */
typedef struct{
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeoclueGeocode* geocoder;
    /*
     * CallBacks from GPS Handler ,Location_Plugin.h
     */

    GeoCodeCallback geocode_callback;
    gpointer userdata;
    guint log_handler;
} GeoclueLbs;

#define LGE_NOMINATIM_SERVICE_NAME "org.freedesktop.Geoclue.Providers.Nominatim"
#define LGE_NOMINATIM_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/Nominatim"





    static void
geocode_cb (GeoclueGeocode       *geocode,
        GeocluePositionFields fields,
        double                latitude,
        double                longitude,
        double                altitude,
        GeoclueAccuracy      *accuracy,
        GError               *error,
        gpointer              userdata)
{
    GeoclueLbs* geoclueLbs = (GeoclueLbs*) userdata;


    g_return_if_fail(geoclueLbs->geocode_callback);

    Position *pos = NULL;
    Accuracy *acc = NULL;
    int ret = ERROR_NONE;
    GeoclueAccuracyLevel level;
    double hor_acc = 0;
    double vert_acc = 0;
    if (error) {
        MOD_LOGW ("Error getting geocode: %s", error->message);
        ret = ERROR_NOT_AVAILABLE;
    } else {

        pos =position_create(0, latitude, longitude, altitude, 0.0, 0.0, 0.0 , fields);
        g_return_if_fail(pos);
        if (accuracy) {
            geoclue_accuracy_get_details(accuracy, &level, &hor_acc, &vert_acc);
            acc = accuracy_create(ACCURACY_LEVEL_DETAILED, hor_acc, vert_acc);
            /* if(accuracy){
            // Free accuracy object if created
            geoclue_accuracy_free(accuracy);
            }*/
        } else {
            acc = accuracy_create(ACCURACY_LEVEL_NONE, hor_acc, vert_acc);
        }
    }
    geoclueLbs->geocode_callback (TRUE, pos, acc, geoclueLbs->userdata);
    g_free (geoclueLbs);
}


/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueLbs> <In> <wifi plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueLbs* geoclueLbs)
{
    if (geoclueLbs->geocoder) {
        g_object_unref (geoclueLbs->geocoder);
        geoclueLbs->geocoder = NULL;
    }


}


static GHashTable* get_address_details (const Address *addr)
{

    GHashTable *geoclue_addrress = geoclue_address_details_new();
    if (addr->street) {
        if (addr->building_number) {
            char *street_detail = g_strdup_printf ("%s %s", addr->building_number, addr->street);
            geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_STREET, street_detail);
            g_free (street_detail);
        }
        else
            geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_STREET, addr->street);
    }

    if (addr->locality != NULL)
        geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_LOCALITY, addr->locality);
    if (addr->region != NULL)
        geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_REGION, addr->region);
    if (addr->countrycode != NULL)
        geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_COUNTRY, addr->countrycode);
    if (addr->postcode != NULL)
        geoclue_address_details_insert (geoclue_addrress, GEOCLUE_ADDRESS_KEY_POSTALCODE, addr->postcode);

    return geoclue_addrress;
}

int get_geocode (gpointer handle,
        const Address * address,
        GeoCodeCallback geo_cb,
        gpointer userdata)
{
    GeoclueLbs* geoclueLbs = (GeoclueLbs*) handle;
    GHashTable*  addr = get_address_details (address);

    geoclueLbs->geocode_callback = geo_cb;
    geoclueLbs->userdata = userdata;
    geoclue_geocode_address_to_position_async (geoclueLbs->geocoder, addr, geocode_cb, geoclueLbs);
    g_hash_table_destroy (addr);
    return ERROR_NONE;
}

/**
 * <Funciton >   intialize_wifi_geoclue_service
 * <Description>  create the GPS geoclue service instance
 * @param     <GeoclueLbs> <In> <wifi plugin structure>
 * @return    gboolean
 */
static gboolean intialize_wifi_geoclue_service(GeoclueLbs* geoclueLbs)
{
    g_return_val_if_fail(geoclueLbs, FALSE);
    //Check its alreasy started
    if(geoclueLbs->geocoder){
        return TRUE;
    }
    /*
     *  Create Geoclue geocode
     */

    geoclueLbs->geocoder = geoclue_geocode_new (LGE_NOMINATIM_SERVICE_NAME, LGE_NOMINATIM_SERVICE_PATH);
    if(geoclueLbs->geocoder == NULL){
        LS_LOG_DEBUG("[DEBUG] Error while creating LG Lbs geoclue object !!");
        unreference_geoclue(geoclueLbs);
        return FALSE;
    }
    LS_LOG_DEBUG("[DEBUG] intialize_Lbs_geoclue_service  done\n");
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
    LS_LOG_DEBUG("[DEBUG] wifi plugin start  plugin_data : %d  ,handler_data :%d \n" , plugin_data ,handler_data);

    GeoclueLbs* geoclueLbs = (GeoclueLbs*) plugin_data;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);
    geoclueLbs->userdata = handler_data;


    if(intialize_wifi_geoclue_service(geoclueLbs) == FALSE){

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
    LS_LOG_DEBUG("[DEBUG]lbs plugin stop\n");
    GeoclueLbs* geoclueLbs = (GeoclueLbs*) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);
    unreference_geoclue(geoclueLbs);
    return ERROR_NONE;
}




/**
 * <Funciton >   init
 * <Description>  intialize the plugin
 * @param     <WifiPluginOps> <In> <enable/disable the callback>
 * @return    int
 */
EXPORT_API gpointer init(LbsPluginOps* ops)
{
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);

    LS_LOG_DEBUG("[DEBUG]Lbs plugin init\n");
    ops->start = start;
    ops->stop = stop ;
    ops->get_geocode = get_geocode;


    /*
     * Handler for plugin
     */
    GeoclueLbs* lbs = g_new0(GeoclueLbs, 1);
    if(lbs == NULL)
        return NULL;
    lbs->log_handler = g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
            | G_LOG_FLAG_RECURSION, lsloghandler, NULL);
    return (gpointer)lbs;
}

/**
 * <Funciton >   shutdown
 * <Description>  stop the plugin & free the variables.
 * @handle     <enable_cb> <In> <enable/disable the callback>
 * @return    int
 */
    EXPORT_API
void shutdown(gpointer handle)
{
    LS_LOG_DEBUG("[DEBUG]wifi plugin shutdown\n");

    g_return_if_fail(handle);
    GeoclueLbs* geoclueLbs = (GeoclueLbs*) handle;
    g_log_remove_handler(NULL, geoclueLbs->log_handler);
    unreference_geoclue(geoclueLbs);
    g_free(geoclueLbs);
}


