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
#include <Address.h>
#include <Position.h>
#include <geoclue/geoclue-geocode.h>
#include <geoclue/geoclue-reverse-geocode.h>
#include <geoclue/geoclue-accuracy.h>
#include <geoclue/geoclue-provider.h>
#include <geoclue/geoclue-types.h>
#include <loc_log.h>

/**
 * Local GPS plugin structure
 */
typedef struct {
    /*
     * Geoclue location structures Holds the D-Bus i/f instance
     */
    GeoclueGeocode *geocode;
    GeoclueReverseGeocode *reverse_geocode;
    gpointer userdata;
} GeoclueLbs;

#define LGE_NOMINATIM_SERVICE_NAME "org.freedesktop.Geoclue.Providers.Nominatim"
#define LGE_NOMINATIM_SERVICE_PATH "/org/freedesktop/Geoclue/Providers/Nominatim"

/**
 * <Funciton >   unreference_geoclue
 * <Description>  free the geoclue instance
 * @param     <GeoclueLbs> <In> <LBS plugin structure>
 * @return    void
 */
static void unreference_geoclue(GeoclueLbs *geoclueLbs)
{
    if (geoclueLbs->geocode != NULL) {
        g_object_unref(geoclueLbs->geocode);
        geoclueLbs->geocode = NULL;
    }

    if (geoclueLbs->reverse_geocode != NULL) {
        g_object_unref(geoclueLbs->reverse_geocode);
        geoclueLbs->reverse_geocode = NULL;
    }
}

int get_geocode_freeform(gpointer handle, const gchar *addrress, Position *pos, Accuracy *ac)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;

    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    GeoclueAccuracy *geoclue_acc = NULL;
    GError *error = NULL;
    double lat, lon, alt;
    GeocluePositionFields fields = geoclue_geocode_freeform_address_to_position(geoclueLbs->geocode,
                                                                                addrress,
                                                                                &lat,
                                                                                &lon,
                                                                                &alt,
                                                                                &geoclue_acc,
                                                                                &error);

    if (error != NULL) {
        LS_LOG_ERROR(" Error in getting data");
        g_error_free(error);
        return ERROR_NOT_AVAILABLE;
    }

    if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE && fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
        pos->latitude = lat;
        pos->longitude = lon;

        if (fields & GEOCLUE_POSITION_FIELDS_ALTITUDE)
            pos->altitude = alt;
        else
            pos->altitude = DEFAULT_VALUE;
    } else {
        LS_LOG_WARNING(" latitude longitutde not present");
        return ERROR_NOT_AVAILABLE;
    }

    if (geoclue_acc != NULL) {
        geoclue_accuracy_get_details(geoclue_acc, &ac->level, &ac->horizAccuracy, &ac->vertAccuracy);
    } else {
        ac->level = ACCURACY_LEVEL_NONE;
        ac->horizAccuracy = DEFAULT_VALUE;
        ac->vertAccuracy = DEFAULT_VALUE;
    }

    return ERROR_NONE;
}

int get_geocode(gpointer handle, const Address *address, Position *pos, Accuracy *ac)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    double lat, lon, alt;
    GeoclueAccuracy *geoclue_acc = NULL;
    GError *error = NULL;
    int err = ERROR_NONE;
    GHashTable *geoclue_addrress = geoclue_address_details_new();

    g_return_val_if_fail(geoclue_addrress, ERROR_NOT_AVAILABLE);

    if (address->street != NULL)
        geoclue_address_details_insert(geoclue_addrress, GEOCLUE_ADDRESS_KEY_STREET, address->street);

    if (address->locality != NULL)
        geoclue_address_details_insert(geoclue_addrress, GEOCLUE_ADDRESS_KEY_LOCALITY, address->locality);

    if (address->region != NULL)
        geoclue_address_details_insert(geoclue_addrress, GEOCLUE_ADDRESS_KEY_REGION, address->region);

    if (address->countrycode != NULL)
        geoclue_address_details_insert(geoclue_addrress, GEOCLUE_ADDRESS_KEY_COUNTRY, address->countrycode);

    if (address->postcode != NULL)
        geoclue_address_details_insert(geoclue_addrress, GEOCLUE_ADDRESS_KEY_POSTALCODE, address->postcode);

    LS_LOG_DEBUG("value of locality  %s region %s country %s country coede %s area %s street %s postcode %s",
                  address->locality,
                  address->region,
                  address->country,
                  address->countrycode,
                  address->area,
                  address->street,
                  address->postcode);

    GeocluePositionFields fields = geoclue_geocode_address_to_position(geoclueLbs->geocode, geoclue_addrress, &lat, &lon, &alt, &geoclue_acc, &error);
    g_hash_table_destroy(geoclue_addrress);

    if (error != NULL) {
        g_error_free(error);
        LS_LOG_ERROR(" Error in return geocode");
        pos->latitude = DEFAULT_VALUE;
        pos->longitude = DEFAULT_VALUE;
        pos->altitude = DEFAULT_VALUE;
        return ERROR_NOT_AVAILABLE;
    }

    if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE && fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
        pos->latitude = lat;
        pos->longitude = lon;

        if (fields & GEOCLUE_POSITION_FIELDS_ALTITUDE)
            pos->altitude = alt;
        else
            pos->altitude = DEFAULT_VALUE;
    } else {
        LS_LOG_WARNING(" Error in return geocode no data avaible");
        err = ERROR_NOT_AVAILABLE;
    }

    if (geoclue_acc != NULL) {
        geoclue_accuracy_get_details(geoclue_acc, &ac->level, &ac->horizAccuracy, &ac->vertAccuracy);
    } else {
        ac->level = ACCURACY_LEVEL_NONE;
        ac->horizAccuracy = DEFAULT_VALUE;
        ac->vertAccuracy = DEFAULT_VALUE;
    }

    return err;
}

int get_reverse_geocode(gpointer handle, Position *pos, Address *address)
{
    GeoclueLbs *geoclueLbs = (GeoclueLbs *) handle;
    g_return_val_if_fail(geoclueLbs, ERROR_NOT_AVAILABLE);

    GeoclueAccuracy *addr_acc = NULL;
    GError *error = NULL;
    GHashTable *geoclue_addr = NULL;
    GeoclueAccuracy *pos_acc = geoclue_accuracy_new(GEOCLUE_ACCURACY_LEVEL_DETAILED, DEFAULT_VALUE, DEFAULT_VALUE);

    g_return_val_if_fail(pos_acc, ERROR_NOT_AVAILABLE);

    gboolean success = geoclue_reverse_geocode_position_to_address(geoclueLbs->reverse_geocode,
                                                                    pos->latitude,
                                                                    pos->longitude,
                                                                    pos_acc,
                                                                    &geoclue_addr,
                                                                    &addr_acc,
                                                                    &error);
    geoclue_accuracy_free(pos_acc);

    if (success == FALSE || error) {
        g_error_free(error);

        if (addr_acc != NULL)
            geoclue_accuracy_free(addr_acc);

        return ERROR_NOT_AVAILABLE;
    }

    if (geoclue_addr != NULL) {
        address->street = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_STREET);
        address->locality = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_LOCALITY);
        address->area = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_AREA);
        address->region = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_REGION);
        address->postcode = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_POSTALCODE);
        address->country = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_COUNTRY);
        address->countrycode = g_hash_table_lookup(geoclue_addr, GEOCLUE_ADDRESS_KEY_COUNTRYCODE);
    }

    if (addr_acc != NULL)
        geoclue_accuracy_free(addr_acc);

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
    if (geoclueLbs->geocode || geoclueLbs->reverse_geocode) {
        return TRUE;
    }

    /*
     *  Create Geoclue geocode
     */
    geoclueLbs->geocode = geoclue_geocode_new(LGE_NOMINATIM_SERVICE_NAME, LGE_NOMINATIM_SERVICE_PATH);
    geoclueLbs->reverse_geocode = geoclue_reverse_geocode_new(LGE_NOMINATIM_SERVICE_NAME, LGE_NOMINATIM_SERVICE_PATH);

    if (geoclueLbs->geocode == NULL || geoclueLbs->reverse_geocode == NULL) {
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
    ops->get_geocode = get_geocode;
    ops->get_reverse_geocode = get_reverse_geocode;
    ops->get_geocode_freetext = get_geocode_freeform;
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

