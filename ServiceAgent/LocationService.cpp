/**********************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Red Rose Platform location
 * Team   : SWF Team
 * Security  : Confidential
 * ***********************************************************/

/*********************************************************
 * @file
 * Filename  : LocationService.cpp
 * Purpose  : Provides location related API to application
 * Platform  : RedRose
 * Author(s)  : Mohammed Sameer Mulla
 * E-mail id. : sameer.mulla@lge.com
 * Creation date :
 *
 * Modifications:
 *
 * Sl No Modified by  Date  Version Description
 *
 **********************************************************/
#include <stdio.h>
#include <LocationService.h>
#include <Handler_Interface.h>
#include <Gps_Handler.h>
#include <Lbs_Handler.h>
#include <Network_Handler.h>
#include <glib.h>
#include <glib-object.h>
#include <JsonUtility.h>
#include <db_util.h>
#include <Address.h>
#include <unistd.h>
#include <LunaLocationServiceUtil.h>
#include <boost/lexical_cast.hpp>
#include <lunaprefs.h>
#include <math.h>
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include <loc_geometry.h>
using namespace std;
/**
 @var LSMethod LocationService::rootMethod[]
 methods belonging to root category
 */
LSMethod LocationService::rootMethod[] = {
    { "getNmeaData", LocationService::_getNmeaData },
    { "getReverseLocation",LocationService::_getReverseLocation },
    { "getGeoCodeLocation",LocationService::_getGeoCodeLocation },
    { "getAllLocationHandlers", LocationService::_getAllLocationHandlers },
    { "getGpsStatus", LocationService::_getGpsStatus },
    { "setState", LocationService::_setState },
    { "getState", LocationService::_getState },
    { "getLocationHandlerDetails",LocationService::_getLocationHandlerDetails },
    { "getGpsSatelliteData", LocationService::_getGpsSatelliteData },
    { "getTimeToFirstFix",LocationService::_getTimeToFirstFix },
    { "getLocationUpdates",LocationService::_getLocationUpdates},
    { "getCachedPosition",LocationService::_getCachedPosition},
    { 0, 0 }
};

/**
 @var LSMethod LocationService::prvMethod[]
 methods belonging to root private category
 */
LSMethod LocationService::prvMethod[] = {
    { "sendExtraCommand", LocationService::_sendExtraCommand },
    { "stopGPS",LocationService::_stopGPS },
    { "exitLocation", LocationService::_exitLocation },
    { "setGPSParameters", LocationService::_setGPSParameters },
    { 0, 0 }
};

/**
 @var LSMethod LocationService::geofenceMethod[]
 methods belonging to geofence category
 */
LSMethod LocationService::geofenceMethod[] = {
    { "addGeofenceArea", LocationService::_addGeofenceArea },
    { "removeGeofenceArea", LocationService::_removeGeofenceArea},
    /*{ "pauseGeofenceArea", LocationService::_pauseGeofence},
    { "resumeGeofenceArea", LocationService::_resumeGeofence},*/ //to uncommented later once geofence lib is available.
    { 0, 0 }
};

LocationService *LocationService::locService = NULL;
LocationService *LocationService::getInstance()
{
    if (locService == NULL) {
        locService = new(std::nothrow) LocationService();
        return locService;
    } else {
        return locService;
    }
}

LocationService::LocationService() :
                 mServiceHandle(0),
                 mlgeServiceHandle(0),
                 mGpsStatus(false),
                 mNwStatus(false),
                 mIsStartFirstReply(true),
                 mIsGetNmeaSubProgress(false),
                 mIsGetSatSubProgress(false),
                 mIsStartTrackProgress(false)
{
    LS_LOG_DEBUG("LocationService object created");
}

void LocationService::LSErrorPrintAndFree(LSError *ptrLSError)
{
    if (ptrLSError != NULL) {
        LSErrorPrint(ptrLSError, stderr);
        LSErrorFree(ptrLSError);
    }
}

bool LocationService::init(GMainLoop *mainLoop)
{
    if (locationServiceRegister(LOCATION_SERVICE_NAME, mainLoop, &mServiceHandle) == false) {
        LS_LOG_ERROR("com.palm.location service registration failed");
        return false;
    }

    if (locationServiceRegister(LOCATION_SERVICE_ALIAS_LGE_NAME, mainLoop, &mlgeServiceHandle) == false) {
        LS_LOG_ERROR("com.lge.location service registration failed");
        return false;
    }

    LS_LOG_DEBUG("mServiceHandle =%x mlgeServiceHandle = %x", mServiceHandle, mlgeServiceHandle);
    g_type_init();
    LS_LOG_DEBUG("[DEBUG] %s::%s( %d ) init ,g_type_init called \n", __FILE__, __PRETTY_FUNCTION__, __LINE__);
    //load all handlers
    handler_array[HANDLER_GPS] = static_cast<Handler *>(g_object_new(HANDLER_TYPE_GPS, NULL));
    handler_array[HANDLER_NW] =  static_cast<Handler *>(g_object_new(HANDLER_TYPE_NW, NULL));
    handler_array[HANDLER_LBS] = static_cast<Handler *>(g_object_new(HANDLER_TYPE_LBS, NULL));

    if ((handler_array[HANDLER_GPS] == NULL) || (handler_array[HANDLER_NW] == NULL) || (handler_array[HANDLER_LBS] == NULL))
        return false;

    //Load initial settings from DB
    mGpsStatus = loadHandlerStatus(GPS);
    mNwStatus = loadHandlerStatus(NETWORK);

    if (pthread_mutex_init(&lbs_geocode_lock, NULL) ||  pthread_mutex_init(&lbs_reverse_lock, NULL)
        || pthread_mutex_init(&nmea_lock, NULL) || pthread_mutex_init(&sat_lock, NULL)
        || pthread_mutex_init(&geofence_add_lock, NULL) || pthread_mutex_init(&geofence_pause_lock, NULL)
        || pthread_mutex_init(&geofence_remove_lock, NULL) || pthread_mutex_init(&geofence_resume_lock, NULL)) {
        return false;
    }

    LS_LOG_INFO("Successfully LocationService object initialized");
    if (LSMessageInitErrorReply() == false) {
        return false;
    }
    // For memory check tool
    mMainLoop = mainLoop;

    if (htPseudoGeofence == NULL) {
        htPseudoGeofence = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                 (GDestroyNotify)g_free, NULL);
    }

    securestorage_set(LSPalmServiceGetPrivateConnection(mServiceHandle), this);
    return true;
}

bool LocationService::locationServiceRegister(char *srvcname, GMainLoop *mainLoop, LSPalmService **msvcHandle)
{
    bool bRetVal; // changed to local variable from class variable
    LSError mLSError;
    LSErrorInit(&mLSError);
    // Register palm service
    bRetVal = LSRegisterPalmService(srvcname, msvcHandle, &mLSError);
    LSERROR_CHECK_AND_PRINT(bRetVal);

    //Gmain attach
    bRetVal = LSGmainAttachPalmService(*msvcHandle, mainLoop, &mLSError);
    LSERROR_CHECK_AND_PRINT(bRetVal);

    // add root category
    bRetVal = LSPalmServiceRegisterCategory(*msvcHandle, "/", rootMethod, prvMethod, NULL, this, &mLSError);
    LSERROR_CHECK_AND_PRINT(bRetVal);

    // add geofence category
    bRetVal = LSPalmServiceRegisterCategory(*msvcHandle, "/geofence", geofenceMethod, NULL, NULL, this, &mLSError);
    LSERROR_CHECK_AND_PRINT(bRetVal);

    //Register cancel function cb to publicbus
    bRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPublicConnection(*msvcHandle),
                                              LocationService::_cancelSubscription,
                                              this,
                                              &mLSError);
    LSERROR_CHECK_AND_PRINT(bRetVal);

    //Register cancel function cb to privatebus
    bRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPrivateConnection(*msvcHandle),
                                              LocationService::_cancelSubscription,
                                              this,
                                              &mLSError);

    LSERROR_CHECK_AND_PRINT(bRetVal);

    return true;
}

LocationService::~LocationService()
{
    // Reviewed by Sathya & Abhishek
    // If only GetCurrentPosition is called and if service is killed then we need to free the memory allocated for handler.
    bool ret;

    if (handler_array[HANDLER_GPS])
       g_object_unref(handler_array[HANDLER_GPS]);

    if (handler_array[HANDLER_NW])
       g_object_unref(handler_array[HANDLER_NW]);

    if (handler_array[HANDLER_LBS])
       g_object_unref(handler_array[HANDLER_LBS]);

    LSMessageReleaseErrorReply();

    if (htPseudoGeofence) {
        g_hash_table_destroy(htPseudoGeofence);
        htPseudoGeofence = NULL;
    }

    if (pthread_mutex_destroy(&lbs_geocode_lock)|| pthread_mutex_destroy(&lbs_reverse_lock)
     || pthread_mutex_destroy(&nmea_lock) || pthread_mutex_destroy(&sat_lock)
     || pthread_mutex_destroy(&geofence_add_lock) || pthread_mutex_destroy(&geofence_pause_lock)
        || pthread_mutex_destroy(&geofence_remove_lock) || pthread_mutex_destroy(&geofence_resume_lock))
    {
        LS_LOG_DEBUG("Could not destroy mutex &map->lock");
    }

    LSError m_LSErr;
    LSErrorInit(&m_LSErr);
    ret = LSUnregisterPalmService(mServiceHandle, &m_LSErr);

    if (ret == false) {
        LSErrorPrint(&m_LSErr, stderr);
        LSErrorFree(&m_LSErr);
    }

    ret = LSUnregisterPalmService(mlgeServiceHandle, &m_LSErr);

    if (ret == false) {
        LSErrorPrint(&m_LSErr, stderr);
        LSErrorFree(&m_LSErr);
    }
}

/*****API's exposed to applications****/

/**
 * <Funciton >   getNmeaData
 * <Description>  API to get repeated nmea data if the subscribe is set to TRUE
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    successful return true else false
 */
bool LocationService::getNmeaData(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("======getNmeaData=====");
    int ret;
    LSError mLSError;

    LSErrorInit(&mLSError);
    jvalue_ref parsedObj = NULL;
    LocationErrorCode errorCode = LOCATION_SUCCESS;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_NMEA_DATA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getNmeaData");
        return true;
    }

    if (getHandlerStatus(GPS) == false) {
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    if (handler_array[HANDLER_GPS] != NULL)
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS, NULL);
    else {
        errorCode = LOCATION_UNKNOWN_ERROR;
        goto EXIT;
    }

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("Error HANDLER_GPS not started");
        errorCode = LOCATION_START_FAILURE;
        goto EXIT;
    }

    // Add to subsciption list with method name as key
    LS_LOG_DEBUG("isSubcriptionListEmpty = %d", isSubscListFilled(sh, message, SUBSC_GPS_GET_NMEA_KEY, false));
    bool mRetVal;
    mRetVal = LSSubscriptionAdd(sh, SUBSC_GPS_GET_NMEA_KEY, message, &mLSError);

    if (mRetVal == false) {
        LS_LOG_ERROR("Failed to add to subscription list");
        LSErrorPrintAndFree(&mLSError);
        errorCode = LOCATION_UNKNOWN_ERROR;
        goto EXIT;
    }

    LS_LOG_DEBUG("Call getNmeaData handler");

    LOCATION_ASSERT(pthread_mutex_lock(&nmea_lock) == 0);
    ret = handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], START, wrapper_getNmeaData_cb, NULL);
    LOCATION_ASSERT(pthread_mutex_unlock(&nmea_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        LS_LOG_ERROR("Error in getNmeaData");
        errorCode = LOCATION_UNKNOWN_ERROR;
    }

EXIT:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    return true;
}

bool LocationService::getCachedDatafromHandler(Handler *hdl, Position *pos, Accuracy *acc, int type)
{
    bool ret;

    ret = handler_get_last_position(hdl, pos, acc, type);
    LS_LOG_INFO("ret value getCachedDatafromHandler and Cached handler type=%d", ret);

    if (ret == ERROR_NONE)
       return true;
    else
       return false;
}

void LocationService::getAddressNominatiumData(jvalue_ref *serviceObject, Address *address) {

    address->street != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("street"), jstring_create(address->street))
                            : jobject_put(*serviceObject, J_CSTR_TO_JVAL("street"), jstring_create(""));

    address->countrycode != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("countrycode"), jstring_create(address->countrycode))
                                 : jobject_put(*serviceObject, J_CSTR_TO_JVAL("countrycode"), jstring_create(""));

    address->postcode != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("postcode"), jstring_create(address->postcode))
                              : jobject_put(*serviceObject, J_CSTR_TO_JVAL("postcode"), jstring_create(""));

    address->area != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("area"), jstring_create(address->area))
                          : jobject_put(*serviceObject, J_CSTR_TO_JVAL("area"), jstring_create(""));

    address->locality != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("locality"), jstring_create(address->locality))
                              : jobject_put(*serviceObject, J_CSTR_TO_JVAL("locality"), jstring_create(""));

    address->region != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("region"), jstring_create(address->region))
                            : jobject_put(*serviceObject, J_CSTR_TO_JVAL("region"), jstring_create(""));

    address->country != NULL ? jobject_put(*serviceObject, J_CSTR_TO_JVAL("country"), jstring_create(address->country))
                             : jobject_put(*serviceObject, J_CSTR_TO_JVAL("country"), jstring_create(""));
}

bool LocationService::getNominatiumReverseGeocode(LSHandle *sh, LSMessage *message, void *data) {

#ifdef NOMINATIUM_LBS
    int ret;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    Address address;
    jvalue_ref serviceObject = NULL;
    LSError mLSError;
    Position pos;
    LocationErrorCode errorCode = LOCATION_SUCCESS;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_REVERSE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getReverseLocation");
        return true;
    }

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        errorCode = LOCATION_DATA_CONNECTION_OFF;
        goto EXIT;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("latitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos.latitude);
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("longitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos.longitude);
    }

    LOCATION_ASSERT(pthread_mutex_lock(&lbs_reverse_lock) == 0);

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS, lbsGeocodeKey);

    if (ret == ERROR_NONE) {
        ret = handler_get_reverse_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &pos, &address);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }

        serviceObject = jobject_create();

        if (jis_null(serviceObject)) {
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));

        getAddressNominatiumData(&serviceObject, &address);

        bool bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);
    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
    }

EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    return true;
#endif
}

void LocationService::getReverseGeocodeData(jvalue_ref *parsedObj, GString **pos_data, Position *pos) {
    jvalue_ref jsonSubObject = NULL;
    jvalue_ref arrObject = NULL;
    std::string strlat;
    std::string strlng;

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("latitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos->latitude);
        strlat = boost::lexical_cast<string>(pos->latitude);
    }

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("longitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos->longitude);
        strlng = boost::lexical_cast<string>(pos->longitude);
    }

    *pos_data = g_string_new("latlng=");
    g_string_append(*pos_data, strlat.c_str());
    g_string_append(*pos_data, ",");
    g_string_append(*pos_data, strlng.c_str());

    raw_buffer nameBuf;

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("language"), &jsonSubObject)){
        nameBuf = jstring_get(jsonSubObject);
        g_string_append(*pos_data, "&language=");
        g_string_append(*pos_data, nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("result_type"), &jsonSubObject)) {
        int size = jarray_size(jsonSubObject);
        g_string_append(*pos_data,"&result_type=");
        LS_LOG_DEBUG("result_type size [%d]",size);
        for (int i = 0; i < size;i++) {
            arrObject = jarray_get(jsonSubObject, i);
            nameBuf = jstring_get(arrObject);

            if (i > 0)
                g_string_append(*pos_data, "|");

            g_string_append(*pos_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }
    }

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("location_type"), &jsonSubObject)) {
        int size = jarray_size(jsonSubObject);
        g_string_append(*pos_data,"&location_type=");
        LS_LOG_DEBUG("location_type size [%d]",size);

        for (int i = 0; i < size; i++) {
            arrObject = jarray_get(jsonSubObject, i);
            nameBuf = jstring_get(arrObject);

            if (i > 0)
                g_string_append(*pos_data, "|");

            g_string_append(*pos_data, nameBuf.m_str);

            jstring_free_buffer(nameBuf);
        }
    }
}

bool LocationService::getReverseLocation(LSHandle *sh, LSMessage *message, void *data)
{
#ifndef NOMINATIUM_LBS
    int ret;
    jvalue_ref parsedObj = NULL;
    char *response = NULL;
    GString *pos_data = NULL;
    LSError mLSError;
    Position pos;
    LocationErrorCode errorCode = LOCATION_SUCCESS;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GOOGLE_REVERSE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getReverseLocation");
        return true;
    }

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        errorCode = LOCATION_DATA_CONNECTION_OFF;
        goto EXIT;
    }

    getReverseGeocodeData(&parsedObj, &pos_data, &pos);
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_reverse_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS, lbsGeocodeKey);

    if (ret == ERROR_NONE) {
        ret = handler_get_reverse_google_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), (char *)pos_data->str, wrapper_rev_geocoding_cb);
        LS_LOG_DEBUG("handler_get_reverse_google_geo_code %s", response);
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }

        if (LSSubscriptionAdd(sh, SUBSC_GET_REVGEOCODE_KEY, message, &mLSError) == FALSE) {
            LS_LOG_ERROR("Failed to add to subscription list");
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }

    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
    }

EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (pos_data != NULL)
        g_string_free(pos_data, TRUE);

    return true;
#else
    return getNominatiumReverseGeocode(sh, message, data);
#endif
}

void geocodeFreeAddress(Address *addr) {

    if (addr->freeformaddr != NULL)
        g_free(addr->freeformaddr);

    if (addr->freeform == false) {
        if (addr->street)
            g_free(addr->street);

        if (addr->country)
            g_free(addr->country);

        if (addr->postcode)
            g_free(addr->postcode);

        if (addr->countrycode)
            g_free(addr->countrycode);

        if (addr->area)
            g_free(addr->area);

        if (addr->locality)
            g_free(addr->locality);

        if (addr->region)
            g_free(addr->region);
    }
}

bool LocationService::getNominatiumGeocode(LSHandle *sh, LSMessage *message, void *data) {
#ifdef NOMINATIUM_LBS
    int ret;
    Address addr = {0};
    LSError mLSError;
    Accuracy acc = {0};
    Position pos;
    jvalue_ref parsedObj = NULL;
    jvalue_ref serviceObject = NULL;
    jvalue_ref jsonSubObject = NULL;
    int errorCode = LOCATION_SUCCESS;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer(JSCHEMA_GET_GEOCODE_LOCATION_1), DOMOPT_NOOPT, NULL);

    if (!input_schema)
        return false;

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(parsedObj)) {
        if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GEOCODE_LOCATION_2, &parsedObj))
            return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("address"), &jsonSubObject)) {
        LS_LOG_DEBUG("value of address %s",jsonSubObject);
        raw_buffer nameBuf = jstring_get(jsonSubObject);
        addr.freeformaddr = g_strdup(nameBuf.m_str); // free the memory occupied
        jstring_free_buffer(nameBuf);
        addr.freeform = true;
    }
    else {
        LS_LOG_ERROR("Not free form");
        addr.freeform = false;

        if (location_util_parsejsonAddress(parsedObj, &addr) == false) {
            LS_LOG_ERROR ("value of no freeform failed");
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }
    }

    LOCATION_ASSERT(pthread_mutex_lock(&lbs_geocode_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS, lbsGeocodeKey);

    if (ret == ERROR_NONE) {
        LS_LOG_ERROR ("value of addr.freeformaddr %s", addr.freeformaddr);
        ret = handler_get_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &addr, &pos, &acc);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);

        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
        LS_LOG_INFO("value of lattitude %f longitude %f atitude %f", pos.latitude, pos.longitude, pos.altitude);
        serviceObject = jobject_create();

        if (jis_null(serviceObject)) {
            LS_LOG_INFO("value of lattitude..1");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        jobject_put(serviceObject, J_CSTR_TO_JVAL("latitude"), jnumber_create_f64(pos.latitude));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("longitude"), jnumber_create_f64(pos.longitude));

        LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
        j_release(&serviceObject);

    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
        LS_LOG_INFO("value of lattitude..2");
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    geocodeFreeAddress(&addr);
    jschema_release(&input_schema);
    return true;
#endif
}

void getAddressData(jvalue_ref *parsedObj, GString *address_data) {
    jvalue_ref jsonBoundSubObject = NULL;
    jvalue_ref jsonSubObject = NULL;
    jvalue_ref jsonComponetObject = NULL;
    raw_buffer nameBuf;

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("bounds"), &jsonBoundSubObject)) {
        double southwestLat;
        double southwestLon;
        double northeastLat;
        double northeastLon;

        jsonSubObject = jobject_get(jsonBoundSubObject, J_CSTR_TO_BUF("southwestLat"));
        jnumber_get_f64(jsonSubObject, &southwestLat);
        std::string strsouthwestLat = boost::lexical_cast<string>(southwestLat);
        jsonSubObject = jobject_get(jsonBoundSubObject, J_CSTR_TO_BUF("southwestLon"));
        jnumber_get_f64(jsonSubObject, &southwestLon);
        std::string strsouthwestLon = boost::lexical_cast<string>(southwestLon);
        jsonSubObject = jobject_get(jsonBoundSubObject, J_CSTR_TO_BUF("northeastLat"));
        jnumber_get_f64(jsonSubObject, &northeastLat);
        std::string strnortheastLat = boost::lexical_cast<string>(northeastLat);
        jsonSubObject = jobject_get(jsonBoundSubObject, J_CSTR_TO_BUF("northeastLon"));
        jnumber_get_f64(jsonSubObject, &northeastLon);
        std::string strnortheastLon = boost::lexical_cast<string>(northeastLon);

        LS_LOG_DEBUG("value of bounds %f,%f,%f",southwestLat, southwestLon, northeastLat);
        g_string_append(address_data, "&bounds=");
        g_string_append(address_data, strsouthwestLat.c_str());
        g_string_append(address_data, ",");
        g_string_append(address_data, strsouthwestLon.c_str());
        g_string_append(address_data, "|");
        g_string_append(address_data, strnortheastLat.c_str());
        g_string_append(address_data, ",");
        g_string_append(address_data, strnortheastLon.c_str());
    }

    if (jobject_get_exists(*parsedObj, J_CSTR_TO_BUF("components"), &jsonComponetObject)) {
        g_string_append(address_data, "components=");

        if (jobject_get_exists(jsonComponetObject, J_CSTR_TO_BUF("route"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);
            g_string_append(address_data, "route:");
            g_string_append(address_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }

        if (jobject_get_exists(jsonComponetObject, J_CSTR_TO_BUF("locality"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);
            g_string_append(address_data, "|locality:");
            g_string_append(address_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }

        if (jobject_get_exists(jsonComponetObject, J_CSTR_TO_BUF("administrative_area"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);
            g_string_append(address_data, "|administrative_area:");
            g_string_append(address_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }

        if (jobject_get_exists(jsonComponetObject, J_CSTR_TO_BUF("postal_code"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);
            g_string_append(address_data, "|postal_code:");
            g_string_append(address_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }

        if (jobject_get_exists(jsonComponetObject, J_CSTR_TO_BUF("country"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);
            g_string_append(address_data, "|country:");
            g_string_append(address_data, nameBuf.m_str);
            jstring_free_buffer(nameBuf);
        }
    }
}

bool LocationService::getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data)
{
#ifndef NOMINATIUM_LBS
    int ret;
    Address addr = {0};
    LSError mLSError;
    bool bRetVal;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    raw_buffer nameBuf;
    GString *address_data = NULL;
    int errorCode = LOCATION_SUCCESS;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GOOGLE_GEOCODE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("LSMessageValidateSchema");
        return true;
    }

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        errorCode = LOCATION_DATA_CONNECTION_OFF;
        goto EXIT;
    }

    if ((!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("address"), &jsonSubObject)) &&
        (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("components"),&jsonSubObject))) {
        LS_LOG_ERROR("Schema validation error");
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }

    address_data = g_string_new(NULL);
    bRetVal = jobject_get_exists(parsedObj, J_CSTR_TO_BUF("address"), &jsonSubObject);

    if (bRetVal == true) {
        LS_LOG_DEBUG("value of address %s",jsonSubObject);
        raw_buffer nameBuf = jstring_get(jsonSubObject);

        std::string str(nameBuf.m_str);
        boost::replace_all(str, " ","+");
        g_string_append(address_data,"address=");
        g_string_append(address_data, str.c_str());
        LS_LOG_DEBUG("value of address %s", address_data->str);
        jstring_free_buffer(nameBuf);
        addr.freeform = true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("language"), &jsonSubObject)) {
        nameBuf = jstring_get(jsonSubObject);
        g_string_append(address_data, "&language=");
        g_string_append(address_data, nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    getAddressData(&parsedObj, address_data);
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_geocode_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS, lbsGeocodeKey);

    if (ret == ERROR_NONE) {
        LS_LOG_ERROR ("value of addr.freeformaddr %s", addr.freeformaddr);
        ret = handler_get_google_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), address_data->str, wrapper_geocoding_cb);
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
        bRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GEOCODE_KEY, message, &mLSError);

        if (bRetVal == false) {
            LS_LOG_ERROR("Failed to add to subscription list");
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
        LS_LOG_INFO("value of lattitude..2");
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (address_data != NULL)
        g_string_free(address_data, TRUE);

    return true;
#else
    return getNominatiumGeocode(sh, message, data);
#endif
}

bool LocationService::getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data)
{
    jvalue_ref handlersArrayItem = NULL;
    jvalue_ref handlersArrayItem1 = NULL;
    jvalue_ref handlersArray = NULL;
    jvalue_ref serviceObject = NULL;
    jvalue_ref parsedObj = NULL;
    bool isSubscription = false;
    bool bRetVal;
    LSError mLSError;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_ALL_LOCATION_HANDLERS, &parsedObj)) {
        return true;
    }

    /*Handle subscription case*/
    if ((isSubscribeTypeValid(sh, message, false, &isSubscription)) && isSubscription) {

        if (LSSubscriptionAdd(sh, SUBSC_GETALLLOCATIONHANDLERS, message, &mLSError) == false) {
            LS_LOG_ERROR("Failed to add getAllLocationHandlers to subscription list");
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);
            return true;
        }
    }

    handlersArrayItem = jobject_create();

    if (jis_null(handlersArrayItem)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    handlersArrayItem1 = jobject_create();

    if (jis_null(handlersArrayItem1)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        j_release(&handlersArrayItem);
        j_release(&parsedObj);
        return true;
    }

    handlersArray = jarray_create(NULL);;

    if (jis_null(handlersArray)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        j_release(&handlersArrayItem1);
        j_release(&handlersArrayItem);
        j_release(&parsedObj);
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        j_release(&handlersArrayItem);
        j_release(&handlersArrayItem1);
        j_release(&handlersArray);
        j_release(&parsedObj);
        return true;
    }

    jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("name"), jstring_create(GPS));
    jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("state"), jboolean_create(getHandlerStatus(GPS)));
    jarray_append(handlersArray, handlersArrayItem);
    jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("name"), jstring_create(NETWORK));
    jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("state"), jboolean_create(getHandlerStatus(NETWORK)));
    jarray_append(handlersArray, handlersArrayItem1);
    /*Form the json object*/
    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("handlers"), handlersArray);

    LS_LOG_DEBUG("Inside LSMessageReply");
    bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    j_release(&serviceObject);
    j_release(&parsedObj);
    return true;
}

bool LocationService::getGpsStatus(LSHandle *sh, LSMessage *message, void *data)
{
    LSError mLSError;
    bool isSubscription = false;
    jvalue_ref serviceObject = NULL;
    jvalue_ref parsedObj = NULL;
    int state = 0;
    LSErrorInit(&mLSError);
    LS_LOG_INFO("=======Subscribe for getGpsStatus=======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GPS_STATUS, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getGpsStatus");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    if (handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS, NULL) != ERROR_NONE) {
        LS_LOG_ERROR("GPS handler start failed\n");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE);
        goto DONE_GET_GPS_STATUS;
    }

    state = handler_get_gps_status((Handler *)handler_array[HANDLER_GPS], wrapper_gpsStatus_cb);

    if (state == GPS_STATUS_AVAILABLE)
        mCachedGpsEngineStatus = true;
    else
        mCachedGpsEngineStatus = false;

    if ((isSubscribeTypeValid(sh, message, false, &isSubscription)) && isSubscription) {
        LS_LOG_ERROR("Subscription call\n");

        if (LSSubscriptionAdd(sh, SUBSC_GPS_ENGINE_STATUS, message, &mLSError) == false) {
            LSErrorPrint(&mLSError, stderr);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);
            goto DONE_GET_GPS_STATUS;
        }

        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        jobject_put(serviceObject, J_CSTR_TO_JVAL("state"),jboolean_create(mCachedGpsEngineStatus));

        if (!LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError))
            LSErrorPrint(&mLSError, stderr);

    } else {
        LS_LOG_DEBUG("Not subscription call\n");

        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        jobject_put(serviceObject, J_CSTR_TO_JVAL("state"), jboolean_create(mCachedGpsEngineStatus));

        if (!LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError))
            LSErrorPrint(&mLSError, stderr);
    }

DONE_GET_GPS_STATUS:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    return true;
}

bool LocationService::getState(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("=======getState=======");
    int state;
    LPAppHandle lpHandle = NULL;
    bool isSubscription = false;
    char *handler;
    LSError mLSError;
    jvalue_ref serviceObject = NULL;

    //Read Handler from json
    jvalue_ref handlerObj = NULL;
    jvalue_ref stateObj = NULL;
    LSErrorInit(&mLSError);
    jvalue_ref parsedObj = NULL;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_STATE, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getState");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &handlerObj)){
        raw_buffer handler_buf = jstring_get(handlerObj);
        handler = g_strdup(handler_buf.m_str);
        jstring_free_buffer(handler_buf);
    }

    //Read state from json
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("state"), &stateObj)){
        jnumber_get_i32(stateObj, &state);
    }

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        int lpret;
        lpret = LPAppCopyValueInt(lpHandle, handler, &state);
        LPAppFreeHandle(lpHandle, true);

        if ((isSubscribeTypeValid(sh, message, false, &isSubscription)) && isSubscription) {
            //Add to subscription list with handler+method name
            char subscription_key[MAX_GETSTATE_PARAM];
            strcpy(subscription_key, handler);
            LS_LOG_INFO("handler_key=%s len =%d", subscription_key, (strlen(SUBSC_GET_STATE_KEY) + strlen(handler)));

            if (LSSubscriptionAdd(sh, strcat(subscription_key, SUBSC_GET_STATE_KEY), message, &mLSError) == false) {
                LS_LOG_ERROR("Failed to add to subscription list");
                LSErrorPrintAndFree(&mLSError);
                LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);
                goto EXIT;
            }
            LS_LOG_DEBUG("handler_key=%s", subscription_key);
        }

        if (lpret != LP_ERR_NONE) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("state"), jnumber_create_i32(0));
            LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
            goto EXIT;
        }

        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        jobject_put(serviceObject, J_CSTR_TO_JVAL("state"),jnumber_create_i32(state));

        LS_LOG_DEBUG("state=%d", state);

        LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
    } else {
        LS_LOG_ERROR("LPAppGetHandle is not created ");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);
        goto EXIT;
    }

EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    g_free(handler);

    return true;
}

bool LocationService::setState(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("=======setState=======");
    char *handler = NULL;
    bool state;
    LSError mLSError;
    LSErrorInit(&mLSError);
    LPAppHandle lpHandle = 0;
    jvalue_ref parsedObj = NULL;
    jvalue_ref serviceObject = NULL;
    jvalue_ref getAllLocationHandlersReplyObject = NULL;
    jvalue_ref handlersArrayItem = NULL;
    jvalue_ref handlersArrayItem1 = NULL;
    jvalue_ref handlersArray = NULL;
    LocationErrorCode errorCode = LOCATION_SUCCESS;

    jvalue_ref handlerObj = NULL;
    jvalue_ref stateObj = NULL;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_SET_STATE, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in setState");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        errorCode = LOCATION_OUT_OF_MEM;
        goto EXIT;
    }

    getAllLocationHandlersReplyObject = jobject_create();

    if (jis_null(getAllLocationHandlersReplyObject)) {
        errorCode = LOCATION_OUT_OF_MEM;
        goto EXIT;
    }

    handlersArrayItem = jobject_create();

    if (jis_null(handlersArrayItem)) {
        errorCode = LOCATION_OUT_OF_MEM;
        goto EXIT;
    }

    handlersArrayItem1 = jobject_create();

    if (jis_null(handlersArrayItem1)) {
        errorCode = LOCATION_OUT_OF_MEM;
        j_release(&handlersArrayItem);
        goto EXIT;
    }

    handlersArray = jarray_create(NULL);

    if (jis_null(handlersArray)) {
        errorCode = LOCATION_OUT_OF_MEM;
        j_release(&handlersArrayItem1);
        j_release(&handlersArrayItem);
        goto EXIT;
    }

    //Read Handler from json
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &handlerObj)){
        raw_buffer handler_buf = jstring_get(handlerObj);
        handler = g_strdup(handler_buf.m_str);
        jstring_free_buffer(handler_buf);
    }

    //Read state from json
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("state"), &stateObj)){
        jboolean_get(stateObj, &state);
    }

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        LS_LOG_INFO("Writing handler state");

        LPAppSetValueInt(lpHandle, handler, state);
        LPAppFreeHandle(lpHandle, true);

        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        bool bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
        char subscription_key[MAX_GETSTATE_PARAM];

        LSERROR_CHECK_AND_PRINT(bRetVal);

        strcpy(subscription_key, handler);
        strcat(subscription_key, SUBSC_GET_STATE_KEY);

        if ((strcmp(handler, GPS) == 0) && mGpsStatus != state) {
            mGpsStatus = state;
            jobject_put(serviceObject, J_CSTR_TO_JVAL("state"), jnumber_create_i32(mGpsStatus));
            bRetVal = LSSubscriptionRespond(mServiceHandle, subscription_key, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle, subscription_key, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            //*getAllLocationHandlers subscription reply
            jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("name"), jstring_create(GPS));
            jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("state"), jboolean_create(mGpsStatus));
            jarray_append(handlersArray, handlersArrayItem);
            jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("name"), jstring_create(NETWORK));
            jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("state"), jboolean_create(mNwStatus));
            jarray_append(handlersArray, handlersArrayItem1);

            /*Form the json object*/
            location_util_form_json_reply(getAllLocationHandlersReplyObject, true, LOCATION_SUCCESS);
            jobject_put(getAllLocationHandlersReplyObject, J_CSTR_TO_JVAL("handlers"), handlersArray);

            bRetVal = LSSubscriptionRespond(mServiceHandle,
                                            SUBSC_GETALLLOCATIONHANDLERS,
                                            jvalue_tostring_simple(getAllLocationHandlersReplyObject),
                                            &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle,
                                            SUBSC_GETALLLOCATIONHANDLERS,
                                            jvalue_tostring_simple(getAllLocationHandlersReplyObject),
                                            &mLSError);
            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);


            if (state == false)
                handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, true);
        } else if ((strcmp(handler, NETWORK) == 0) && mNwStatus != state) {
            mNwStatus = state;
            jobject_put(serviceObject, J_CSTR_TO_JVAL("state"), jnumber_create_i32(mNwStatus));
            bRetVal = LSSubscriptionRespond(mServiceHandle, subscription_key, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle, subscription_key, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            //*getAllLocationHandlers subscription reply
            jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("name"), jstring_create(GPS));
            jobject_put(handlersArrayItem, J_CSTR_TO_JVAL("state"), jboolean_create(mGpsStatus));
            jarray_append(handlersArray, handlersArrayItem);
            jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("name"), jstring_create(NETWORK));
            jobject_put(handlersArrayItem1, J_CSTR_TO_JVAL("state"), jboolean_create(mNwStatus));
            jarray_append(handlersArray, handlersArrayItem1);

            /*Form the json object*/
            location_util_form_json_reply(getAllLocationHandlersReplyObject, true, LOCATION_SUCCESS);
            jobject_put(getAllLocationHandlersReplyObject, J_CSTR_TO_JVAL("handlers"), handlersArray);

            bRetVal = LSSubscriptionRespond(mServiceHandle,
                                            SUBSC_GETALLLOCATIONHANDLERS,
                                            jvalue_tostring_simple(getAllLocationHandlersReplyObject),
                                            &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle,
                                            SUBSC_GETALLLOCATIONHANDLERS,
                                            jvalue_tostring_simple(getAllLocationHandlersReplyObject),
                                            &mLSError);
            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            if (state == false) {
                handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, true);
                handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, true);
            }
        } else { //memleak
            j_release(&handlersArrayItem1);
            j_release(&handlersArrayItem);
            j_release(&handlersArray);
        }

    } else {
        LS_LOG_DEBUG("LPAppGetHandle is not created");
        j_release(&handlersArrayItem1);
        j_release(&handlersArrayItem);
        j_release(&handlersArray);
        errorCode = LOCATION_UNKNOWN_ERROR;
    }

EXIT:

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(getAllLocationHandlersReplyObject))
        j_release(&getAllLocationHandlersReplyObject);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    if (handler != NULL)
        g_free(handler);

    return true;
}

bool LocationService::setGPSParameters(LSHandle *sh, LSMessage *message, void *data) {
    LS_LOG_INFO("=======setGPSParameters=======");
    jvalue_ref parsedObj = NULL;
    jvalue_ref serviceObject = NULL;
    char *cmdStr = NULL;
    bool bRetVal;
    int ret;

    LSError mLSError;
    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_SET_GPS_PARAMETERS, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in setGPSParameters");
        return true;
    }

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS, NULL);

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("GPS Handler starting failed");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE);
        j_release(&parsedObj);
        return true;
    }

    int err = handler_set_gps_parameters(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), jvalue_tostring_simple(parsedObj));

    if (err == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("Error in %s", cmdStr);
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        j_release(&parsedObj);
        return true;
    }

    handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        j_release(&parsedObj);
        return true;
    }

    jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));

    bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    j_release(&serviceObject);
    j_release(&parsedObj);
    return true;
}

bool LocationService::exitLocation(LSHandle *sh, LSMessage *message, void *data) {
     g_main_loop_quit(mMainLoop);
     return true;
}

bool LocationService::stopGPS(LSHandle *sh, LSMessage *message, void *data) {
    LS_LOG_INFO("=======stopGPS=======");
    int ret;
    LSError mLSError;
    jvalue_ref serviceObject = NULL;
    jvalue_ref parsedObj = NULL;

    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_STOP_GPS, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in stopGPS");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        j_release(&parsedObj);
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    ret = handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, true);

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("GPS force stop failed");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);

    } else {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        bool bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);
    }

    j_release(&serviceObject);
    j_release(&parsedObj);
    return true;
}

bool LocationService::sendExtraCommand(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("=======sendExtraCommand=======");
    jvalue_ref serviceObject = NULL;
    char *command;
    bool bRetVal;
    int ret;
    LSError mLSError;
    LSErrorInit(&mLSError);
    jvalue_ref commandObj = NULL;
    jvalue_ref parsedObj = NULL;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_SEND_EXTRA_COMMAND, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in sendExtraCommand");
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("command"), &commandObj)){
        raw_buffer handler_buf = jstring_get(commandObj);
        command = g_strdup(handler_buf.m_str);
        jstring_free_buffer(handler_buf);
    }

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS, NULL);

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("GPS Handler starting failed");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }
    LS_LOG_INFO("command = %s", command);

    int err = handler_send_extra_command(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), command);

    if (err == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("Error in %s", command);
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }

    handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree (&mLSError);

    j_release(&parsedObj);
    j_release(&serviceObject);
    g_free(command);

    return true;
}

bool LocationService::getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data)
{
    char *handler = NULL;
    int powerRequirement = 0;
    bool bRetVal;
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    jvalue_ref serviceObject = NULL;

    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_LOCATION_HANDLER_DETAILS, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getLocationHandlerDetails");
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &jsonSubObject)) {
        raw_buffer nameBuf = jstring_get(jsonSubObject);
        handler = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }
    else {
        LS_LOG_ERROR("Invalid param:Handler");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
        j_release(&parsedObj);
        return true;
    }

    if (NULL == handler) {
        LS_LOG_ERROR("ParamInput is NULL");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
        j_release(&parsedObj);
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        g_free(handler);
        j_release(&parsedObj);
        return true;
    }

    if (strcmp(handler, "gps") == 0) {
        powerRequirement = 1;
        LS_LOG_INFO("getHandlerStatus(GPS)");

        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("accuracy"), jnumber_create_i32(ACCURACY_LEVEL_HIGH));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("powerRequirement"), jnumber_create_i32(powerRequirement));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresNetwork"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresCell"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("monetaryCost"), jboolean_create(false));

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else if (strcmp(handler, "network") == 0) {
        LS_LOG_INFO("getHandlerStatus(network)");
        powerRequirement = 3;

        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("accuracy"), jnumber_create_i32(ACCURACY_LEVEL_LOW));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("powerRequirement"), jnumber_create_i32(powerRequirement));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresNetwork"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresCell"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("monetaryCost"), jboolean_create(true));

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else {
        LS_LOG_ERROR("LPAppGetHandle is not created");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
    }

    j_release(&parsedObj);
    j_release(&serviceObject);
    g_free(handler);
    return true;
}

bool LocationService::getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    bool bRetVal;
    LSError mLSError;
    LocationErrorCode errorCode = LOCATION_SUCCESS;
    jvalue_ref parsedObj = NULL;

    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GPS_SATELLITE_DATA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getGpsSatelliteData");
        return true;
    }

    if (getHandlerStatus(GPS)) {
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS, NULL);

        if (ret != ERROR_NONE) {
            LS_LOG_ERROR("GPS Handler not started");
            errorCode = LOCATION_START_FAILURE;
            goto EXIT;
        }
    } else {
        LS_LOG_ERROR("getGpsSatelliteData GPS OFF");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    bRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GPS_SATELLITE_DATA, message, &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        errorCode = LOCATION_SUCCESS;
        goto EXIT;
    }

    LOCATION_ASSERT(pthread_mutex_lock(&sat_lock) == 0);
    ret = handler_get_gps_satellite_data(handler_array[HANDLER_GPS], START, wrapper_getGpsSatelliteData_cb);
    LOCATION_ASSERT(pthread_mutex_unlock(&sat_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        errorCode = LOCATION_UNKNOWN_ERROR;
    }

EXIT:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    return true;
}

bool LocationService::getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data)
{
    bool bRetVal ;
    long long mTTFF = 0;
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref serviceObject = NULL;
    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_TIME_TO_FIRST_FIX, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getTimeToFirstFix");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)){
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    mTTFF = handler_get_time_to_first_fix((Handler *) handler_array[HANDLER_GPS]);
    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("TTFF"),jnumber_create_i32(mTTFF));

    LS_LOG_INFO("get time to first fix %lld reply payload %s", mTTFF, jvalue_tostring_simple(serviceObject));

    bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
    }

    j_release(&parsedObj);
    j_release(&serviceObject);

    return true;
}

bool LocationService::addGeofenceArea(LSHandle *sh, LSMessage *message, void *data)
{
    gdouble latitude = 0.0;
    gdouble longitude = 0.0;
    gdouble radius = 0.0;
    int geofenceid = -1;
    int count = 0;
    int errorCode = LOCATION_SUCCESS;
    char str_geofenceid[32];
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    LS_LOG_DEBUG("=======addGeofenceArea=======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_ADD_GEOFENCE_AREA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in addGeofenceArea\n");
        return true;
    }

    //GPS  off
    if (getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) {
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    /* Extract latitude from message */
    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("latitude"), &jsonSubObject);
    jnumber_get_f64(jsonSubObject, &latitude);

     /* Extract longitude from message */
    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("longitude"), &jsonSubObject);
    jnumber_get_f64(jsonSubObject, &longitude);

     /* Extract radius from message */
    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("radius"), &jsonSubObject);
    jnumber_get_f64(jsonSubObject, &radius);

    if (handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                      HANDLER_GPS, NULL) != ERROR_NONE) {
        errorCode = LOCATION_START_FAILURE;
        goto EXIT;
    }

     /* Generate random Geofence Id which is not used by previous request */
    while (count < (MAX_GEOFENCE_RANGE - MIN_GEOFENCE_RANGE)) {
        geofenceid = rand() % (MAX_GEOFENCE_RANGE-MIN_GEOFENCE_RANGE) + MIN_GEOFENCE_RANGE;

        LS_LOG_DEBUG("=======addGeofenceArea ID genrated %d=======\n", geofenceid);

        if (!is_geofenceId_used[geofenceid - MIN_GEOFENCE_RANGE]) {
            is_geofenceId_used[geofenceid - MIN_GEOFENCE_RANGE] = true;
            break;
        }

        count++;
    }

    if (count == (MAX_GEOFENCE_ID - MIN_GEOFENCE_RANGE)) {
        LS_LOG_DEBUG("MAX_GEOFENCE_ID reached\n");
        errorCode = LOCATION_GEOFENCE_TOO_MANY_GEOFENCE;
        goto EXIT;
    }

    if (handler_add_geofence_area((Handler *) handler_array[HANDLER_GPS],
                                  TRUE,
                                  &geofenceid,
                                  &latitude,
                                  &longitude,
                                  &radius,
                                  wrapper_geofence_add_cb,
                                  wrapper_geofence_breach_cb,
                                  wrapper_geofence_status_cb) != ERROR_NONE) {
        errorCode = LOCATION_UNKNOWN_ERROR;
    } else {
        LSErrorInit(&mLSError);
        sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_ADD_AREA_KEY);

        if (LSSubscriptionAdd(sh, str_geofenceid, message, &mLSError)) {
            g_hash_table_insert(htPseudoGeofence,
                                LSMessageGetUniqueToken(message),
                                GINT_TO_POINTER(geofenceid));
        } else {
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
        }
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

bool LocationService::removeGeofenceArea(LSHandle *sh, LSMessage *message, void *data)
{
    int geofenceid = -1;
    int errorCode = LOCATION_SUCCESS;
    char str_geofenceid[32];
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    LS_LOG_DEBUG("=======removeGeofenceArea=======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_REMOVE_GEOFENCE_AREA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in removeGeofenceArea\n");
        return true;
    }

    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("geofenceid"), &jsonSubObject);
    jnumber_get_i32(jsonSubObject, &geofenceid);

    sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_ADD_AREA_KEY);

    if (isSubscListFilled(sh, message, str_geofenceid, false) == false) {
        errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
        goto EXIT;
    }

    if (handler_remove_geofence((Handler *) handler_array[HANDLER_GPS],
                                TRUE,
                                &geofenceid,
                                wrapper_geofence_remove_cb) != ERROR_NONE) {
        errorCode = LOCATION_UNKNOWN_ERROR;
    } else {
        LSErrorInit(&mLSError);
        sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_REMOVE_AREA_KEY);

        if (!LSSubscriptionAdd(sh, str_geofenceid, message, &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
        }
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

bool LocationService::pauseGeofence(LSHandle *sh, LSMessage *message, void *data)
{
    int geofence_id = -1;
    int errorReply = LOCATION_SUCCESS;
    jvalue_ref parsedObj = NULL;
    char str_geofence_id[32];
    LSError mLSError;
    jvalue_ref jsonObject = NULL;

    LS_LOG_DEBUG("=======pauseGeofence======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_PAUSE_GEOFENCE_AREA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in removeGeofenceArea\n");
        return true;
    }

    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("geofenceid"), &jsonObject);
    jnumber_get_i32(jsonObject, &geofence_id);

    sprintf(str_geofence_id, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);
    if (isSubscListFilled(sh, message, str_geofence_id, false) == false) {
        errorReply = LOCATION_GEOFENCE_ID_UNKNOWN;
        goto EXIT;
    }

    if (handler_pause_geofence((Handler *) handler_array[HANDLER_GPS],
                               TRUE,
                               &geofence_id,
                               wrapper_geofence_pause_cb) != ERROR_NONE) {
        errorReply = LOCATION_UNKNOWN_ERROR;
    } else {
        LSErrorInit(&mLSError);
        sprintf(str_geofence_id, "%d%s", geofence_id, SUBSC_GEOFENCE_PAUSE_AREA_KEY);

        if (!LSSubscriptionAdd(sh, str_geofence_id, message, &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
            errorReply = LOCATION_UNKNOWN_ERROR;
        }
    }

EXIT:

    if (errorReply != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorReply);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

bool LocationService::resumeGeofence(LSHandle *sh, LSMessage *message, void *data)
{
    int geofenceid = -1;
    int monitorTrans = 0;
    int errorCode = LOCATION_SUCCESS;
    char str_geofenceid[32];
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    LS_LOG_DEBUG("=======resumeGeofence======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_RESUME_GEOFENCE_AREA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in resumeGeofenceArea\n");
        return true;
    }

    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("geofenceid"), &jsonSubObject);
    jnumber_get_i32(jsonSubObject, &geofenceid);

    sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_ADD_AREA_KEY);
    if (isSubscListFilled(sh, message, str_geofenceid, false) == false) {
        errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
        goto EXIT;
    }

    monitorTrans = GEOFENCE_ENTERED | GEOFENCE_EXITED | GEOFENCE_UNCERTAIN;
    if (handler_resume_geofence((Handler *) handler_array[HANDLER_GPS],
                                TRUE,
                                &geofenceid,
                                &monitorTrans,
                                wrapper_geofence_resume_cb) != ERROR_NONE) {
        errorCode = LOCATION_UNKNOWN_ERROR;
    } else {
        LSErrorInit(&mLSError);
        sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_RESUME_AREA_KEY);

        if (!LSSubscriptionAdd(sh, str_geofenceid, message, &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
        }
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

int LocationService::getConnectionErrorCode() {
    int errorCode = LOCATION_SUCCESS;

    if ((getConnectionManagerState() == false) && (getWifiInternetState() == false))
        errorCode = LOCATION_DATA_CONNECTION_OFF;
    else if (getWifiState() == false)
        errorCode = LOCATION_WIFI_CONNECTION_OFF;
    else
        errorCode = LOCATION_UNKNOWN_ERROR;

    return errorCode;
}

bool LocationService::getLocationUpdates(LSHandle *sh, LSMessage *message, void *data) {

    LS_LOG_INFO("=======getLocationUpdates=======payload %s",LSMessageGetPayload(message));
    int errorCode = LOCATION_SUCCESS;
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref serviceObj = NULL;
    char key[KEY_MAX] = { 0x00 };
    char* handlerName = NULL;
    bool bRetVal = false;
    unsigned char startedHandlers = 0;
    guint responseTime = 0;
    int sel_handler = LocationService::GETLOC_UPDATE_INVALID;
    int minInterval = 0;
    int minDistance = 0;
    int handlertype = -1;

    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCEHMA_GET_LOCATION_UPDATES, &parsedObj)) {
        LS_LOG_ERROR("Schema error in getLocationUpdates");
        return true;
    }

    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) &&
        (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_ERROR("Both handler are OFF\n");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    /* Parse minimumInterval in milliseconds */
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumInterval"), &serviceObj)) {
        jnumber_get_i32(serviceObj, &minInterval);
        LS_LOG_DEBUG("minimumInterval %d", minInterval);
    }

    /* Parse minimumDistance in meters*/
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &serviceObj)) {
        jnumber_get_i32(serviceObj, &minDistance);
        LS_LOG_DEBUG("minimumDistance %d", minDistance);
    }
    /* Parse Handler name */
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &serviceObj)) {
        raw_buffer nameBuf = jstring_get(serviceObj);
        handlerName = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    } else {
        handlerName = g_strdup(HYBRID);
        handlertype = HANDLER_HYBRID;
    }

    LS_LOG_INFO("handlerName %s", handlerName);

    if(((strcmp(handlerName, GPS) == 0) && (getHandlerStatus(GPS) == false)) ||
       ((strcmp(handlerName, NETWORK) == 0) && (getHandlerStatus(NETWORK) == false))) {
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    sel_handler = getHandlerVal(handlerName);


    if (sel_handler == LocationService::GETLOC_UPDATE_INVALID) {
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }

    /*Parse responseTime in seconds*/
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("responseTimeout"), &serviceObj)) {
        jnumber_get_i32(serviceObj,&responseTime);
        LS_LOG_DEBUG("responseTime in seconds %d", responseTime);
    }

    if (enableHandlers(sel_handler, key, &startedHandlers) || (sel_handler == LocationService::GETLOC_UPDATE_PASSIVE)) {

        struct timeval tv;
        gettimeofday(&tv, (struct timezone *) NULL);
        long long reqTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
        TimerData *timerdata = NULL;
        guint timerID = 0;

        timerdata = new(std::nothrow) TimerData(message, sh, startedHandlers,key);
        if (timerdata == NULL) {
            LS_LOG_ERROR("timerdata null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        LS_LOG_INFO("timerdata %p", timerdata);
        if (responseTime != 0) {
            timerID = g_timeout_add_seconds(responseTime, &TimerCallbackLocationUpdate, timerdata);
            LS_LOG_INFO("timerID %d started for responseTime %d", timerID,responseTime);
        }

        LocationUpdateRequest *locUpdateReq = new(std::nothrow) LocationUpdateRequest(message,
                                                                                      reqTime,
                                                                                      timerID,
                                                                                      timerdata,
                                                                                      LocationService::INVALID_LAT,
                                                                                      LocationService::INVALID_LONG,
                                                                                      handlertype);

        if (locUpdateReq == NULL) {
            LS_LOG_ERROR("locUpdateReq null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        boost::shared_ptr<LocationUpdateRequest> req (locUpdateReq);
        m_locUpdate_req_list.push_back(req);

        /*Add to subsctiption list*/
        bRetVal = LSSubscriptionAdd(sh, key, message, &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }

        /********Request gps Handler*****************************/
        if (startedHandlers & HANDLER_GPS_BIT) {
            handler_get_location_updates((Handler *) handler_array[HANDLER_GPS],
                                          START,
                                          wrapper_getLocationUpdate_cb,
                                          NULL,
                                          HANDLER_GPS,
                                          LSPalmServiceGetPrivateConnection(mServiceHandle));
        }

        /********Request wifi Handler*****************************/
        if (startedHandlers & HANDLER_WIFI_BIT) {
            handler_get_location_updates((Handler *) handler_array[HANDLER_NW],
                                          START,
                                          wrapper_getLocationUpdate_cb,
                                          NULL,
                                          HANDLER_WIFI,
                                          LSPalmServiceGetPrivateConnection(mServiceHandle));
        }

        /********Request cellid Handler*****************************/
        if (startedHandlers & HANDLER_CELLID_BIT) {

            handler_get_location_updates((Handler *) handler_array[HANDLER_NW],
                                          START,
                                          wrapper_getLocationUpdate_cb,
                                          NULL,
                                          HANDLER_CELLID,
                                          LSPalmServiceGetPrivateConnection(mServiceHandle));

        }

    } else {
        LS_LOG_ERROR("Unable to start handlers");
        errorCode = getConnectionErrorCode();
        goto EXIT;
    }

EXIT:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS) {
        LSMessageReplyError(sh, message, errorCode);
    }

    if (handlerName != NULL)
        g_free(handlerName);

    return true;
}



bool LocationService::getCachedPosition(LSHandle *sh, LSMessage *message, void *data) {
    LSError lsError;
    int errorCode = LOCATION_SUCCESS;
    Position pos;
    Accuracy acc;
    Position gpspos;
    Accuracy gpsacc;
    Position wifipos;
    Accuracy wifiacc;
    Position cellpos;
    Accuracy cellacc;
    jvalue_ref parsedObj = NULL;
    jvalue_ref handlerObj = NULL;
    jvalue_ref serviceObj = NULL;
    jvalue_ref maximumAgeObj = NULL;
    char* handlerName = NULL;
    int maximumAge = 0;
    bool bRetVal;

    LSErrorInit(&lsError);
    LS_LOG_INFO("=======getCachedPosition======= payload %s",LSMessageGetPayload(message));

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_CACHED_POSITION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getCurrentPosition");
        return true;
    }

    serviceObj = jobject_create();

    if (jis_null(serviceObj)) {
        errorCode = LOCATION_OUT_OF_MEM;
        goto EXIT;
    }

    memset(&pos, 0x00, sizeof(Position));
    memset(&gpspos, 0x00, sizeof(Position));
    memset(&wifipos, 0x00, sizeof(Position));
    memset(&cellpos, 0x00, sizeof(Position));

    /*Parse Handler name*/
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &handlerObj)) {
        raw_buffer nameBuf = jstring_get(handlerObj);
        handlerName = g_strdup(nameBuf.m_str);
        LS_LOG_INFO("handlerName %s", handlerName);
        jstring_free_buffer(nameBuf);
    } else {
        handlerName = g_strdup(HYBRID);
    }
    //check in cache first if maximumAge is -1

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("maximumAge"), &maximumAgeObj)) {
        jnumber_get_i32(maximumAgeObj,&maximumAge);
    }

    LS_LOG_INFO("maximumAge= %d", maximumAge);

    if (strcmp(handlerName,GPS) == 0) {
        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                                 &pos,
                                 &acc,
                                 HANDLER_GPS);
    } else if (strcmp(handlerName,NETWORK) == 0) {
        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                 &wifipos,
                                 &wifiacc,
                                 HANDLER_WIFI);

        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                 &cellpos,
                                 &cellacc,
                                 HANDLER_CELLID);

        pos = comparePositionTimeStamps(gpspos, wifipos, cellpos, gpsacc, wifiacc, cellacc, &acc);

    } else if (strcmp(handlerName,HYBRID) == 0 || strcmp(handlerName,PASSIVE) == 0) {
        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                                 &gpspos,
                                 &gpsacc,
                                 HANDLER_GPS);

        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                 &wifipos,
                                 &wifiacc,
                                 HANDLER_WIFI);

        getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                 &cellpos,
                                 &cellacc,
                                 HANDLER_CELLID);

        pos = comparePositionTimeStamps(gpspos,
                                        wifipos,
                                        cellpos,
                                        gpsacc,
                                        wifiacc,
                                        cellacc,
                                        &acc);
    }

    if (pos.latitude == 0 || pos.longitude == 0) {
        errorCode = LOCATION_POS_NOT_AVAILABLE;
    } else {
        if (maximumAge > 0) {
            struct timeval tv;
            gettimeofday(&tv, (struct timezone *) NULL);
            long long currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

            if (maximumAge < (currentTime - pos.timestamp)) {
                errorCode = LOCATION_POS_NOT_AVAILABLE;
            }
        }
    }

EXIT:
    if (errorCode != LOCATION_SUCCESS ) {
        LS_LOG_INFO("getCachedPosition reply error %d",errorCode);
        LSMessageReplyError(sh, message, errorCode);
    } else {
        //Read according to accuracy value
        location_util_form_json_reply(serviceObj, true, LOCATION_SUCCESS);
        location_util_add_pos_json(serviceObj, &pos);
        location_util_add_acc_json(serviceObj, &acc);

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObj), &lsError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&lsError);
    }

    if (!jis_null(serviceObj)) {
        LS_LOG_INFO("getCachedPosition reply payload %s", jvalue_tostring_simple(serviceObj));
        j_release(&serviceObj);
    }

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (handlerName != NULL)
        g_free(handlerName);

    return true;
}



Position LocationService::comparePositionTimeStamps(Position pos1,
                                                    Position pos2,
                                                    Position pos3,
                                                    Accuracy acc1,
                                                    Accuracy acc2,
                                                    Accuracy acc3,
                                                    Accuracy *retAcc)
{
    Position pos;
    memset(&pos, 0x00, sizeof(Position));

    if (pos1.timestamp >= pos2.timestamp && pos1.timestamp >= pos3.timestamp) {
        /*timestamp are same then compare accuracy*/
        if (pos1.timestamp == pos2.timestamp) {
            (acc1.horizAccuracy < acc2.horizAccuracy) ? (*retAcc = acc1, pos = pos1) : (*retAcc = acc2, pos = pos2);
        } else if (pos1.timestamp == pos3.timestamp) {
            (acc1.horizAccuracy < acc3.horizAccuracy) ? (*retAcc = acc1, pos = pos1) : (*retAcc = acc3, pos = pos3);
        } else {
            *retAcc = acc1;
            pos = pos1;
        }
    }

    if (pos2.timestamp >= pos1.timestamp && pos2.timestamp >= pos3.timestamp) {
         /*timestamp are same then compare accuracy*/
        if (pos2.timestamp == pos1.timestamp) {
            (acc2.horizAccuracy < acc1.horizAccuracy) ? (*retAcc = acc2, pos = pos2) : (*retAcc = acc1, pos = pos1);
        } else if (pos2.timestamp == pos3.timestamp) {
            (acc2.horizAccuracy < acc3.horizAccuracy) ? (*retAcc = acc2, pos = pos2) : (*retAcc = acc3, pos = pos3);
        } else {
            *retAcc = acc2;
            pos = pos2;
        }
    }

    if (pos3.timestamp >= pos1.timestamp && pos3.timestamp >= pos2.timestamp) {
         /*timestamp are same then compare accuracy*/
        if (pos3.timestamp == pos1.timestamp) {
            (acc3.horizAccuracy < acc1.horizAccuracy) ? (*retAcc = acc3, pos = pos3) : (*retAcc = acc1, pos = pos1);
        } else if (pos3.timestamp == pos2.timestamp) {
            (acc3.horizAccuracy < acc2.horizAccuracy) ? (*retAcc = acc3, pos = pos3) : (*retAcc = acc2, pos = pos2);
        } else {
            *retAcc = acc3;
            pos = pos3;
        }
    }

    return pos;
}


int LocationService::enableHandlers(int sel_handler, char *key, unsigned char *startedHandlers)
{
    bool gpsHandlerStatus;
    bool wifiHandlerStatus;

    switch (sel_handler) {
        case LocationService::GETLOC_UPDATE_GPS:
            if (enableGpsHandler(startedHandlers)) {
                strcpy(key, SUBSC_GET_LOC_UPDATES_GPS_KEY);
            }

            break;

        case LocationService::GETLOC_UPDATE_NW:
            if (enableNwHandler(startedHandlers)) {
                strcpy(key, SUBSC_GET_LOC_UPDATES_NW_KEY);
            }

            break;

        case LocationService::GETLOC_UPDATE_GPS_NW:
            gpsHandlerStatus = enableGpsHandler(startedHandlers);
            wifiHandlerStatus = enableNwHandler(startedHandlers);

            if (gpsHandlerStatus || wifiHandlerStatus) {
                strcpy(key, SUBSC_GET_LOC_UPDATES_HYBRID_KEY);
            }

            break;

        case LocationService::GETLOC_UPDATE_PASSIVE:
            strcpy(key, SUBSC_GET_LOC_UPDATES_PASSIVE_KEY);
            break;

        default:
            LS_LOG_DEBUG("LocationService invalid case");
            break;
    }

    if (*startedHandlers == HANDLER_STATE_DISABLED) {
        LS_LOG_INFO("All handlers disabled");
        return false;
    }

    return true;
}


bool LocationService::enableNwHandler(unsigned char *startedHandlers)
{
    bool ret = false;

    if (getHandlerStatus(NETWORK)) {
        LS_LOG_DEBUG("network is on in Settings");

        if ((getWifiState() == true) && (getConnectionManagerState() || getWifiInternetState())) {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                HANDLER_WIFI, nwGeolocationKey);

            if (ret != ERROR_NONE) {
                LS_LOG_ERROR("WIFI handler not started");
            } else {
                *startedHandlers |= HANDLER_WIFI_BIT;
            }
        }

        LS_LOG_INFO("isInternetConnectionAvailable %d TelephonyState %d",
                     getConnectionManagerState(),
                     getTelephonyState());

        if (getTelephonyState() && (getConnectionManagerState() || getWifiInternetState())) {

            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                HANDLER_CELLID, nwGeolocationKey);

            if (ret != ERROR_NONE) {
                LS_LOG_ERROR("CELLID handler not started");
            } else {
                *startedHandlers |= HANDLER_CELLID_BIT;
            }
        }

        if (*startedHandlers == HANDLER_STATE_DISABLED)
            return false;

    }

    return true;
}



bool LocationService::enableGpsHandler(unsigned char *startedHandlers)
{
    LS_LOG_DEBUG("===enableGpsHandler====");

    if (getHandlerStatus(GPS)) {
        bool ret = false;
        LS_LOG_DEBUG("gps is on in Settings");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                            HANDLER_GPS, NULL);

        if (ret != ERROR_NONE) {
            LS_LOG_ERROR("GPS handler not started");
            return false;
        }

        *startedHandlers |= HANDLER_GPS_BIT;
    } else {
        return false;
    }

    return true;
}

int LocationService::getHandlerVal(char *handlerName)
{
    int sel_handler =LocationService::GETLOC_UPDATE_INVALID;

    if (handlerName == NULL)
        sel_handler = LocationService::GETLOC_UPDATE_INVALID;
    else if (strcmp(handlerName, GPS) == 0)
        sel_handler = LocationService::GETLOC_UPDATE_GPS;
    else if (strcmp(handlerName, NETWORK) == 0)
        sel_handler = LocationService::GETLOC_UPDATE_NW;
    else if (strcmp(handlerName, HYBRID) == 0)
        sel_handler = LocationService::GETLOC_UPDATE_GPS_NW;
    else if (strcmp(handlerName, PASSIVE) == 0)
        sel_handler = LocationService::GETLOC_UPDATE_PASSIVE;

    return sel_handler;

}

/********************* Callback functions**********************************************************************
 ********************   Called from handlers********************************************************************
 **************************************************************************************************************/

void LocationService::wrapper_geocoding_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type)
{
    LS_LOG_DEBUG("wrapper_geocoding_cb");
    getInstance()->geocoding_reply(response, error, type);
}

void LocationService::wrapper_rev_geocoding_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type)
{
    LS_LOG_DEBUG("wrapper_rev_geocoding_cb");
    getInstance()->rev_geocoding_reply(response, error, type);
}

/**
 * <Funciton >   wrapper_getCurrentPosition_cb
 * <Description>  Callback function called for startTracking from handler
 * @param     <enable_cb> <In>
 * @param     <pos> <In> <Position Structure>
 * @param     <accuracy> <In> <Accuracy Structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void LocationService::wrapper_getNmeaData_cb(gboolean enable_cb, int64_t timestamp, char *nmea, int length, gpointer privateIns)
{
    LS_LOG_DEBUG("wrapper_getNmeaData_cb called back ...");
    getInstance()->get_nmea_reply(timestamp, nmea, length);
}

void LocationService::wrapper_getGpsSatelliteData_cb(gboolean enable_cb, Satellite *satellite, gpointer privateIns)
{
    LS_LOG_DEBUG("wrapper_getGpsSatelliteData_cb called back ...");
    getInstance()->getGpsSatelliteData_reply(satellite);
}

void LocationService::wrapper_gpsStatus_cb(gboolean enable_cb, int state, gpointer data)
{
    LS_LOG_DEBUG("wrapper_gpsStatus_cb called back ...");
    getInstance()->getGpsStatus_reply(state);
}

void LocationService::wrapper_geofence_add_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    LS_LOG_INFO("gps_handler_geofence_add_cb\n");
    getInstance()->geofence_add_reply(geofence_id, status);
}

void LocationService::wrapper_geofence_remove_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    LS_LOG_DEBUG("wrapper_geofence_remove_cb");
    getInstance()->geofence_remove_reply(geofence_id, status);
}

void LocationService::wrapper_geofence_pause_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    LS_LOG_DEBUG("wrapper_geofence_pause_cb");
    getInstance()->geofence_pause_reply(geofence_id, status);
}

void LocationService::wrapper_geofence_resume_cb(int32_t geofence_id, int32_t status, gpointer user_data)
{
    LS_LOG_DEBUG("wrapper_geofence_resume_cb");
    getInstance()->geofence_resume_reply(geofence_id, status);
}

void LocationService::wrapper_geofence_breach_cb(int32_t geofence_id, int32_t status, int64_t timestamp, double latitude, double longitude, gpointer user_data)
{
    LS_LOG_DEBUG("wrapper_geofence_breach_cb");
    getInstance()->geofence_breach_reply(geofence_id, status, timestamp, latitude, longitude);
}

void LocationService::wrapper_geofence_status_cb (int32_t status, Position *last_position, Accuracy *accuracy, gpointer user_data)
{
    LS_LOG_DEBUG("wrapper_geofence_status_cb");
    // Not required as it is only intialization status
}

void LocationService::wrapper_getLocationUpdate_cb(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error,gpointer privateIns, int type)
{
   LS_LOG_DEBUG("wrapper_getLocationUpdate_cb");
   getInstance()->getLocationUpdate_reply(pos, accuracy, error, type);
}

/********************************Response to Application layer*********************************************************************/

void LocationService::get_nmea_reply(long long timestamp, char *data, int length)
{
    LSError mLSError;
    char *retString = NULL;
    jvalue_ref serviceObject = NULL;

    LS_LOG_DEBUG("[DEBUG] get_nmea_reply called\n");

    LSErrorInit(&mLSError);

    serviceObject = jobject_create();
    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Out of memory\n");
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    LS_LOG_DEBUG("timestamp=%lu\n", timestamp);
    LS_LOG_DEBUG("data=%s\n", data);

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("timestamp"), jnumber_create_i64(timestamp));

    if (data != NULL)
        jobject_put(serviceObject, J_CSTR_TO_JVAL("nmea"), jstring_create(data));

    retString = jvalue_tostring_simple(serviceObject);

EXIT:
    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              SUBSC_GPS_GET_NMEA_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                             SUBSC_GPS_GET_NMEA_KEY,
                                             retString,
                                             &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::getGpsSatelliteData_reply(Satellite *sat)
{
    int num_satellite_used_count = 0;
    char *retString = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    /*Creating a json object*/
    jvalue_ref serviceObject = NULL;
    jvalue_ref serviceArray = NULL;
    jvalue_ref visibleSatelliteItem = NULL;

    LS_LOG_DEBUG("[DEBUG] getGpsSatelliteData_reply called, reply to application\n");

    if (sat == NULL) {
        LS_LOG_ERROR("satellite data NULL");
        return;
    }

    serviceArray = jarray_create(NULL);
    if (jis_null(serviceArray)) {
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    serviceObject = jobject_create();
    if (jis_null(serviceObject)) {
        j_release(&serviceArray);
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("visibleSatellites"), jnumber_create_i32(sat->visible_satellites_count));

    while (num_satellite_used_count < sat->visible_satellites_count) {
        visibleSatelliteItem = jobject_create();

        if (!jis_null(visibleSatelliteItem)){
            LS_LOG_DEBUG(" Service Agent value of %llf num_satellite_used_count%d ", sat->sat_used[num_satellite_used_count].azimuth,num_satellite_used_count);
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("index"), jnumber_create_i32(num_satellite_used_count));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("azimuth"), jnumber_create_f64(sat->sat_used[num_satellite_used_count].azimuth));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("elevation"), jnumber_create_f64(sat->sat_used[num_satellite_used_count].elevation));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("prn"), jnumber_create_i32(sat->sat_used[num_satellite_used_count].prn));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("snr"), jnumber_create_f64(sat->sat_used[num_satellite_used_count].snr));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("hasAlmanac"), jboolean_create(sat->sat_used[num_satellite_used_count].hasalmanac));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("hasEphemeris"), jboolean_create(sat->sat_used[num_satellite_used_count].hasephemeris));
            jobject_put(visibleSatelliteItem, J_CSTR_TO_JVAL("usedInFix"), jboolean_create(sat->sat_used[num_satellite_used_count].used));

            jarray_append(serviceArray, visibleSatelliteItem);
            num_satellite_used_count++;
        }

    }

    jobject_put(serviceObject, J_CSTR_TO_JVAL("satellites"), serviceArray);
    retString = jvalue_tostring_simple(serviceObject);

EXIT:
    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              SUBSC_GET_GPS_SATELLITE_DATA,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              SUBSC_GET_GPS_SATELLITE_DATA,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

/**
 * <Funciton >   getGpsStatus_reply

 * <Description>  Callback function called to update gps status

 * @param     LunaService handle

 * @param     LunaService message

 * @param     user data

 * @return    bool if successful return true else false
 */
void LocationService::getGpsStatus_reply(int state)
{
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool isGpsEngineOn;

    if (state == GPS_STATUS_AVAILABLE)
        isGpsEngineOn = true;
    else
        isGpsEngineOn = false;

    mCachedGpsEngineStatus = isGpsEngineOn;
    jvalue_ref serviceObject = jobject_create();

    if (!jis_null(serviceObject)) {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        jobject_put(serviceObject, J_CSTR_TO_JVAL("state"), jboolean_create(isGpsEngineOn));

        bool bRetVal = LSSubscriptionRespond(mServiceHandle,
                                             SUBSC_GPS_ENGINE_STATUS,
                                             jvalue_tostring_simple(serviceObject),
                                             &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        bRetVal = LSSubscriptionRespond(mlgeServiceHandle,
                                        SUBSC_GPS_ENGINE_STATUS,
                                        jvalue_tostring_simple(serviceObject),
                                        &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        j_release(&serviceObject);
    }

}

void LocationService::geofence_breach_reply(int32_t geofence_id, int32_t status, int64_t timestamp, double latitude, double longitude)
{
    LSError mLSError;
    char str_geofenceid[32];
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("geofence_breach_reply: id=%d, status=%d, timestamp=%lld, latitude=%f, longitude=%f\n",
                geofence_id, status, timestamp, latitude, longitude);

    serviceObject = jobject_create();

    if (!jis_null(serviceObject)) {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);

        switch (status) {
            case GEOFENCE_ENTERED:
                retString = geofenceStateText[GEOFENCE_TRANSITION_ENTERED];
                break;

            case GEOFENCE_EXITED:
                retString = geofenceStateText[GEOFENCE_TRANSITION_EXITED];
                break;

            case GEOFENCE_UNCERTAIN:
                retString = geofenceStateText[GEOFENCE_TRANSITION_UNCERTAIN];
                break;

            default:
                retString = geofenceStateText[GEOFENCE_TRANSITION_UNCERTAIN];
                break;
        }

        jobject_put(serviceObject, J_CSTR_TO_JVAL("status"), jstring_create(retString));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("timestamp"), jnumber_create_i64(timestamp));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("latitude"), jnumber_create_f64(latitude));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("longitude"), jnumber_create_f64(longitude));

        retString = jvalue_tostring_simple(serviceObject);
    } else {
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
    }

    LSErrorInit(&mLSError);
    sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);

    if (!LSSubscriptionRespond(mServiceHandle,
                               str_geofenceid,
                               retString,
                               &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionRespond(mlgeServiceHandle,
                               str_geofenceid,
                               retString,
                               &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::geofence_add_reply(int32_t geofence_id, int32_t status)
{
    LSError mLSError;
    char str_geofenceid[32];
    int errorCode = LOCATION_UNKNOWN_ERROR;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("geofence_add_reply: id=%d, status=%d\n", geofence_id, status);

    switch (status) {
        case GEOFENCE_OPERATION_SUCCESS:
            errorCode = LOCATION_SUCCESS;
            break;

        case GEOFENCE_ERROR_TOO_MANY_GEOFENCES:
            errorCode = LOCATION_GEOFENCE_TOO_MANY_GEOFENCE;
            break;

        case GEOFENCE_ERROR_ID_EXISTS:
            errorCode = LOCATION_GEOFENCE_ID_EXIST;
            break;

        case GEOFENCE_ERROR_INVALID_TRANSITION:
            errorCode = LOCATION_GEOFENCE_INVALID_TRANSITION;
            break;

        case GEOFENCE_ERROR_GENERIC:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;

        default:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;
    }

    if (errorCode != LOCATION_SUCCESS) {
        is_geofenceId_used[geofence_id - MIN_GEOFENCE_RANGE] = false;

        retString = LSMessageGetErrorReply(errorCode);
    } else {
        serviceObject = jobject_create();

        if (!jis_null(serviceObject)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("status"), jstring_create(geofenceStateText[GEOFENCE_ADD_SUCCESS]));
            jobject_put(serviceObject, J_CSTR_TO_JVAL("geofenceid"), jnumber_create_i32(geofence_id));
            retString = jvalue_tostring_simple(serviceObject);
        } else {
            retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        }
    }

    LSErrorInit(&mLSError);
    sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);

    if (!LSSubscriptionRespond(mServiceHandle,
                               str_geofenceid,
                               retString,
                               &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionRespond(mlgeServiceHandle,
                               str_geofenceid,
                               retString,
                               &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::geofence_remove_reply(int32_t geofence_id, int32_t status)
{
    LSError mLSError;
    char str_geofenceid[32];
    int errorCode = LOCATION_UNKNOWN_ERROR;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("geofence_remove_reply: id=%d, status=%d\n", geofence_id, status);

    switch (status) {
        case GEOFENCE_OPERATION_SUCCESS:
            errorCode = LOCATION_SUCCESS;
            break;

        case GEOFENCE_ERROR_ID_UNKNOWN:
            errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
            break;

        case GEOFENCE_ERROR_GENERIC:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;

        default:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;
    }

    if (errorCode != LOCATION_SUCCESS) {
        retString = LSMessageGetErrorReply(errorCode);
    } else {
        serviceObject = jobject_create();

        if (!jis_null(serviceObject)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("status"), jstring_create(geofenceStateText[GEOFENCE_REMOVE_SUCCESS]));
            retString = jvalue_tostring_simple(serviceObject);
        } else {
            retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        }
    }

    LSErrorInit(&mLSError);
    sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_REMOVE_AREA_KEY);

    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (status == GEOFENCE_OPERATION_SUCCESS) {
        sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);

        /*Reply add luna api and remove from list*/
        if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                  str_geofenceid,
                                                  retString,
                                                  &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                  str_geofenceid,
                                                  retString,
                                                  &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        handler_remove_geofence((Handler *) handler_array[HANDLER_GPS], FALSE, 0, wrapper_geofence_remove_cb);
        is_geofenceId_used[geofence_id - MIN_GEOFENCE_RANGE] = false;

        if (htPseudoGeofence != NULL) {
            GHashTableIter iter;
            gpointer key, value;
            key = value = NULL;
            g_hash_table_iter_init(&iter, htPseudoGeofence);

            while (g_hash_table_iter_next(&iter, &key, &value)) {
                if (GPOINTER_TO_INT(value) == geofence_id) {
                    g_hash_table_remove(htPseudoGeofence, key);
                    break;
                }
            }
        }
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::geofence_pause_reply(int32_t geofence_id, int32_t status)
{
    LSError mLSError;
    char str_geofenceid[32];
    int errorCode = LOCATION_UNKNOWN_ERROR;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("geofence_pause_reply: id=%d, status=%d\n", geofence_id, status);

    switch (status) {
        case GEOFENCE_OPERATION_SUCCESS:
            errorCode = LOCATION_SUCCESS;
            break;

        case GEOFENCE_ERROR_ID_UNKNOWN:
            errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
            break;

        case GEOFENCE_ERROR_INVALID_TRANSITION:
            errorCode = LOCATION_GEOFENCE_INVALID_TRANSITION;
            break;

        case GEOFENCE_ERROR_GENERIC:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;

        default:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;
    }

    if (errorCode != LOCATION_SUCCESS) {
        retString = LSMessageGetErrorReply(errorCode);
    } else {
        serviceObject = jobject_create();

        if (!jis_null(serviceObject)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("status"), jstring_create(geofenceStateText[GEOFENCE_PAUSE_SUCCESS]));
            retString = jvalue_tostring_simple(serviceObject);
        } else {
            retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        }
    }

    LSErrorInit(&mLSError);
    sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_PAUSE_AREA_KEY);

    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (status == GEOFENCE_OPERATION_SUCCESS) {
        sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);

        if (!LSSubscriptionRespond(mServiceHandle,
                                   str_geofenceid,
                                   retString,
                                   &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        if (!LSSubscriptionRespond(mlgeServiceHandle,
                                   str_geofenceid,
                                   retString,
                                   &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::geofence_resume_reply(int32_t geofence_id, int32_t status)
{
    LSError mLSError;
    char str_geofenceid[32];
    int errorCode = LOCATION_UNKNOWN_ERROR;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("geofence_resume_reply: id=%d, status=%d\n", geofence_id, status);

    switch(status) {
        case GEOFENCE_OPERATION_SUCCESS:
            errorCode = LOCATION_SUCCESS;
            break;

        case GEOFENCE_ERROR_ID_UNKNOWN:
            errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
            break;

        case GEOFENCE_ERROR_GENERIC:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;

        default:
            errorCode = LOCATION_UNKNOWN_ERROR;
            break;
    }

    if (errorCode != LOCATION_SUCCESS) {
        retString = LSMessageGetErrorReply(errorCode);
    } else {
        serviceObject = jobject_create();

        if (!jis_null(serviceObject)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("status"), jstring_create(geofenceStateText[GEOFENCE_RESUME_SUCCESS]));
            retString = jvalue_tostring_simple(serviceObject);
        } else {
            retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        }
    }

    LSErrorInit(&mLSError);
    sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_RESUME_AREA_KEY);

    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              str_geofenceid,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (status == GEOFENCE_OPERATION_SUCCESS) {
        sprintf(str_geofenceid, "%d%s", geofence_id, SUBSC_GEOFENCE_ADD_AREA_KEY);

        if (!LSSubscriptionRespond(mServiceHandle,
                                   str_geofenceid,
                                   retString,
                                   &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        if (!LSSubscriptionRespond(mlgeServiceHandle,
                                   str_geofenceid,
                                   retString,
                                   &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}


void LocationService::getLocationUpdate_reply(Position *pos, Accuracy *accuracy, int error, int type)
{
    LS_LOG_INFO("getLocationUpdate_reply");
    char *retString = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool bRetVal;
    jvalue_ref serviceObject = NULL;
    char *key1 = NULL;
    char *key2 = NULL;

    if(pos)
        LS_LOG_INFO("latitude %f longitude %f altitude %f timestamp %lld", pos->latitude,
                                                                           pos->longitude,
                                                                           pos->altitude,
                                                                           pos->timestamp);
    if(accuracy)
        LS_LOG_INFO("horizAccuracy %f vertAccuracy %f", accuracy->horizAccuracy, accuracy->vertAccuracy);

    if (type == HANDLER_GPS) {
        key1 = SUBSC_GET_LOC_UPDATES_GPS_KEY;
        key2 = SUBSC_GET_LOC_UPDATES_HYBRID_KEY;
    } else {
        key1 = SUBSC_GET_LOC_UPDATES_NW_KEY;
        key2 = SUBSC_GET_LOC_UPDATES_HYBRID_KEY;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Out of memory");
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    if (error == ERROR_NONE) {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject,accuracy);
        retString = jvalue_tostring_simple(serviceObject);
    } else if (error == ERROR_TIMEOUT) {
        retString = LSMessageGetErrorReply(LOCATION_TIME_OUT);
    } else {
        retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
    }

EXIT:
    LS_LOG_DEBUG("key1 %s key2 %s", key1,key2);
    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mServiceHandle,
                                                 key1,
                                                 retString,
                                                 &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mlgeServiceHandle,
                                                 key1,
                                                 retString,
                                                 &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mServiceHandle,
                                                 key2,
                                                 retString,
                                                 &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mlgeServiceHandle,
                                                 key2,
                                                 retString,
                                                 &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    /*reply to passive provider*/

    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mServiceHandle,
                                                 SUBSC_GET_LOC_UPDATES_PASSIVE_KEY,
                                                 jvalue_tostring_simple(serviceObject),
                                                 &mLSError);
    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubNonSubRespondGetLocUpdateCase(pos,
                                                 accuracy,
                                                 mlgeServiceHandle,
                                                 SUBSC_GET_LOC_UPDATES_PASSIVE_KEY,
                                                 jvalue_tostring_simple(serviceObject),
                                                 &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    if (!jis_null(serviceObject)) {
        LS_LOG_INFO("reply payload %s", jvalue_tostring_simple(serviceObject));
        j_release(&serviceObject);
    }
}

void LocationService::geocoding_reply(char *response, int error, int type)
{
    LSError mLSError;
    char *retString = NULL;
    jvalue_ref jval = NULL;
    jdomparser_ref parser = NULL;
    jvalue_ref serviceObject = NULL;

    handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);

    if (error == ERROR_NOT_AVAILABLE || response == NULL) {
        retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
        goto EXIT;
    } else if (error == ERROR_NETWORK_ERROR) {
        retString = LSMessageGetErrorReply(LOCATION_DATA_CONNECTION_OFF);
        goto EXIT;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    //create parser
    parser = jdomparser_create(&schemaInfo, 0);

    if (!parser) {
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    if (!jdomparser_feed(parser, response, strlen(response))) {
        retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
        goto EXIT;
    }

    jval = jdomparser_get_result(parser);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("response"), jval);
    retString = jvalue_tostring_simple(serviceObject);

EXIT:
    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              SUBSC_GET_GEOCODE_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                             SUBSC_GET_GEOCODE_KEY,
                                             retString,
                                             &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    if (parser != NULL)
        jdomparser_release(&parser);
}

void LocationService::rev_geocoding_reply(char *response, int error, int type)
{
    LSError mLSError;
    char *retString = NULL;
    jvalue_ref jval = NULL;
    jdomparser_ref parser = NULL;
    jvalue_ref serviceObject = NULL;

    handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);

    LS_LOG_INFO("value of rev_geocoding_reply..%d", error);

    if (error == ERROR_NOT_AVAILABLE || response == NULL) {
        retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
        goto EXIT;
    } else if (error == ERROR_NETWORK_ERROR) {
        retString = LSMessageGetErrorReply(LOCATION_DATA_CONNECTION_OFF);
        goto EXIT;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_INFO("value of lattitude..1");
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    //create parser
    parser = jdomparser_create(&schemaInfo, 0);

    if (!parser) {
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    if (!jdomparser_feed(parser, response, strlen(response))) {
        retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
        goto EXIT;
    }

    jval = jdomparser_get_result(parser);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("response"), jval);
    retString = jvalue_tostring_simple(serviceObject);

EXIT:
    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              SUBSC_GET_REVGEOCODE_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              SUBSC_GET_REVGEOCODE_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);

    if (parser != NULL)
        jdomparser_release(&parser);
}



/**
 * <Funciton >   cancelSubscription

 * <Description>  Callback function called when application cancels the subscription or Application crashed

 * @param     LunaService handle

 * @param     LunaService message

 * @param     user data
 * @return    bool if successful return true else false
 */
bool LocationService::cancelSubscription(LSHandle *sh, LSMessage *message, void *data)
{
    LSError mLSError;
    bool bRetVal = false;
    char *key = LSMessageGetMethod(message);
    LSSubscriptionIter *iter = NULL;

    LS_LOG_DEBUG("LSMessageGetMethod(message) %s\n", key);

    LSErrorInit(&mLSError);

    if (key == NULL) {
        LS_LOG_ERROR("Not a valid key");
        return true;
    }

    if ((strcmp(key, SUBSC_GEOFENCE_ADD_AREA_KEY)) == 0) {
        int geofenceid = -1;
        gpointer geofenceid_ptr = NULL;
        char *uniquetoken = NULL;

        if (htPseudoGeofence) {
            uniquetoken = LSMessageGetUniqueToken(message);

            if (uniquetoken) {
                geofenceid_ptr = g_hash_table_lookup(htPseudoGeofence, uniquetoken);

                if (geofenceid_ptr) {
                    geofenceid = GPOINTER_TO_INT(geofenceid_ptr);

                    is_geofenceId_used[geofenceid - MIN_GEOFENCE_RANGE] = false;
                    handler_remove_geofence((Handler *) handler_array[HANDLER_GPS],
                                            TRUE,
                                            &geofenceid,
                                            wrapper_geofence_remove_cb);
                }
            }
        }

        double latitude,longitude,radius;

        handler_add_geofence_area((Handler *) handler_array[HANDLER_GPS],
                                  FALSE,
                                  &geofenceid,
                                  &latitude,
                                  &longitude,
                                  &radius,
                                  wrapper_geofence_add_cb,
                                  wrapper_geofence_breach_cb,
                                  wrapper_geofence_status_cb);
    }else if (key != NULL && (strcmp(key, GET_LOC_UPDATE_KEY) == 0)) {
        getLocRequestStopSubscription(sh, message);
    } else  {
        LS_LOG_DEBUG("NOT CRITERIA_KEY");
        bRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
            return true;
        }

        if (LSSubscriptionHasNext(iter)) {
            LSMessage *msg = LSSubscriptionNext(iter);

        LS_LOG_DEBUG("LSSubscriptionHasNext = %d %p", LSSubscriptionHasNext(iter), msg);

        if (LSSubscriptionHasNext(iter) == false)
            stopSubcription(sh, key);
        }
        LSSubscriptionRelease(iter);


    }
    return true;
}

/**
 * <Funciton >   getLocRequestStopSubscription

 * <Description>  Stop gps engine handling for getLocationUpdate when all gps request are empty

 * @param     LunaService handle

 * @param     LunaService message

 * @return    void
 */
void LocationService::getLocRequestStopSubscription(LSHandle *sh, LSMessage *message) {

    LS_LOG_DEBUG("getLocRequestStopSubscription");

    if (!isSubscListFilled(sh, message, SUBSC_GET_LOC_UPDATES_NW_KEY, true) &&
        !isSubscListFilled(sh, message, SUBSC_GET_LOC_UPDATES_HYBRID_KEY, true)) {
        LS_LOG_INFO("getLocRequestStopSubscription Stopping NW handler");
        stopSubcription(sh, SUBSC_GET_LOC_UPDATES_NW_KEY);
    }

    if (!isSubscListFilled(sh, message, SUBSC_GET_LOC_UPDATES_GPS_KEY, true) &&
        !isSubscListFilled(sh, message, SUBSC_GET_LOC_UPDATES_HYBRID_KEY, true)) {
        LS_LOG_INFO("getLocRequestStopSubscription Stopping GPS Engine");
        stopSubcription(sh, SUBSC_GET_LOC_UPDATES_GPS_KEY);
    }

    LSMessageRemoveReqList(message);
}



bool LocationService::getHandlerStatus(const char *handler)
{
    LS_LOG_INFO("getHandlerStatus handler %s", handler);

    if (strcmp(handler, GPS) == 0)
        return mGpsStatus;

    if (strcmp(handler, NETWORK) == 0)
        return mNwStatus;

    return false;
}

bool LocationService::loadHandlerStatus(const char *handler)
{
    LPAppHandle lpHandle = 0;
    int state = HANDLER_STATE_DISABLED;
    LS_LOG_INFO("getHandlerStatus ");

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        int lpret;
        lpret = LPAppCopyValueInt(lpHandle, handler, &state);
        LS_LOG_INFO("state=%d lpret = %d\n", state, lpret);
        LPAppFreeHandle(lpHandle, true);
    } else {
        LS_LOG_DEBUG("[DEBUG] handle initialization failed\n");
    }

    if (state != HANDLER_STATE_DISABLED)
        return true;
    else
        return false;
}

void LocationService::stopSubcription(LSHandle *sh,const char *key)
{
    if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {

        handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], STOP,
                               wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetNmeaSubProgress = false;

    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {

        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP,
                                       wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetSatSubProgress = false;

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_GPS_KEY) == 0) {

        //STOP GPS TODO change to proper Handler API after handler implementation
        handler_get_location_updates(handler_array[HANDLER_GPS], STOP, wrapper_getLocationUpdate_cb,
                                     NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_NW_KEY) == 0) {

        //STOP CELLID TODO change to proper Handler API after handler implementation
        handler_get_location_updates(handler_array[HANDLER_NW], STOP,
                                     wrapper_getLocationUpdate_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

        //STOP WIFI
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_getLocationUpdate_cb,
                                     NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

    }

}

void LocationService::stopNonSubcription(const char *key) {
    if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {
        handler_get_nmea_data(handler_array[HANDLER_GPS], STOP,
                               wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {
        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP,
                                       wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_GPS_KEY) == 0) {
        //STOP GPS
        handler_get_location_updates(handler_array[HANDLER_GPS], STOP, wrapper_getLocationUpdate_cb,
                                     NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_NW_KEY) == 0) {
        //STOP CELLID
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_getLocationUpdate_cb,
                                     NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

        //STOP WIFI
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_getLocationUpdate_cb,
                                     NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

    }
}

gboolean LocationService::_TimerCallbackLocationUpdate(void *data)
{
    LS_LOG_INFO("======_TimerCallbackLocationUpdate==========");
    char *retString = NULL;
    LSError lserror;
    TimerData *timerdata = (TimerData *) data;
    LSSubscriptionIter *iter = NULL;

    LSErrorInit(&lserror);
    retString = LSMessageGetErrorReply(LOCATION_TIME_OUT);

    if (timerdata == NULL) {
        LS_LOG_ERROR("TimerCallback timerdata is null");
        return false;
    }

    LSMessage *message = timerdata->GetMessage();
    LSHandle *sh = timerdata->GetHandle();
    const char *key = timerdata->getKey();

    if (message == NULL || sh == NULL || key == NULL) {
        LS_LOG_ERROR("Timerdata member is NULL");
        return false;
    } else {
        /*reply timeout and remove from list */
        bool bRetVal = LSMessageReply(sh, message, retString, &lserror);
        if (bRetVal == false) {
            LSErrorPrintAndFree(&lserror);
        }

        bRetVal = LSSubscriptionAcquire(sh, key, &iter, &lserror);
        if (bRetVal == true && LSSubscriptionHasNext(iter)) {
            do {
                LSMessage *msg = LSSubscriptionNext(iter);
                if (msg == message)
                {
                    LSSubscriptionRemove(iter);
                    if(!LSMessageRemoveReqList(msg))
                        LS_LOG_ERROR("Message is not found in the LocationUpdateRequestPtr in timeout");
                 }
             } while (LSSubscriptionHasNext(iter));
        }
        LSSubscriptionRelease(iter);
        getLocRequestStopSubscription(sh,message);
    }

    return false;
}

/*Timer Implementation END */
/*Returns true if the list is filled
 *false if it empty*/
bool LocationService::isSubscListFilled(LSHandle *sh,
                                        LSMessage *message,
                                        const char *key,
                                        bool cancelCase)
{
    LSError mLSError;
    LSSubscriptionIter *iter = NULL;
    bool bRetVal;
    LSMessage *msg;
    int i = 0;

    LSErrorInit(&mLSError);
    bRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

    if (bRetVal == false) {
        LSErrorPrint(&mLSError, stderr);
        LSErrorFree(&mLSError);
        return bRetVal;
    }

    if (LSSubscriptionHasNext(iter) && cancelCase) {
        msg = LSSubscriptionNext(iter);
        if (message == msg) {
            bRetVal = LSSubscriptionHasNext(iter); // checking list of own message
        } else if (msg != NULL) {
            bRetVal = true;
        }
    } else {
        bRetVal = LSSubscriptionHasNext(iter);
    }

    LSSubscriptionRelease(iter);

    if (bRetVal == false)
       LS_LOG_DEBUG("List is empty for key %s i = %d",key,i++);

    if (bRetVal == true)
        LS_LOG_DEBUG("List is not empty for key %s i = %d",key,i);

    return bRetVal;
}

bool LocationService::LSSubscriptionNonSubscriptionRespond(LSPalmService *psh, const char *key, const char *payload, LSError *lserror)
{
    LSHandle *public_bus = LSPalmServiceGetPublicConnection(psh);
    LSHandle *private_bus = LSPalmServiceGetPrivateConnection(psh);
    bool retVal = LSSubscriptionNonSubscriptionReply(public_bus, key, payload, lserror);

    if (retVal == false)
        return retVal;

    retVal = LSSubscriptionNonSubscriptionReply(private_bus, key, payload, lserror);

    if (retVal == false)
        return retVal;
}

bool LocationService::LSSubNonSubRespondGetLocUpdateCase(Position *pos,
                                                         Accuracy *acc,
                                                         LSPalmService *psh,
                                                         const char *key,
                                                         const char *payload,
                                                         LSError *lserror)
{
    LSHandle *public_bus = LSPalmServiceGetPublicConnection(psh);
    LSHandle *private_bus = LSPalmServiceGetPrivateConnection(psh);

    bool retVal = LSSubNonSubReplyLocUpdateCase(pos, acc, public_bus, key, payload, lserror);
    if (retVal == false)
        return retVal;

    retVal = LSSubNonSubReplyLocUpdateCase(pos, acc, private_bus, key, payload, lserror);
    if (retVal == false)
        return retVal;
}

bool LocationService::LSSubscriptionNonSubscriptionReply(LSHandle *sh,
                                                         const char *key,
                                                         const char *payload,
                                                         LSError *lserror)
{
    LSSubscriptionIter *iter = NULL;
    bool retVal;
    bool isNonSubscibePresent = false;

    retVal = LSSubscriptionAcquire(sh, key, &iter, lserror);

    if (retVal == true && LSSubscriptionHasNext(iter)) {
        do {
            LSMessage *msg = LSSubscriptionNext(iter);

            if (LSMessageIsSubscription(msg)) {
                LS_LOG_DEBUG("LSSubscriptionNonSubscriptionReply replying susbcribed call LSMessageGetPayload(message) = %s",
                              LSMessageGetPayload(msg));
                retVal = LSMessageReply(sh, msg, payload, lserror);
                if (retVal == false) {
                    return retVal;
                }
            } else {
                retVal = LSMessageReply(sh, msg, payload, lserror);
                if (retVal == false) {
                    return retVal;
                }
                LSSubscriptionRemove(iter);
                LS_LOG_DEBUG("Removed Non Subscription message from list");
                isNonSubscibePresent = true;
            }

        } while (LSSubscriptionHasNext(iter));

    }

    LSSubscriptionRelease(iter);

    if (isNonSubscibePresent && (isSubscListFilled(sh, NULL,key,false) == false))
        stopNonSubcription(key);

    return retVal;
}

bool LocationService::LSSubNonSubReplyLocUpdateCase(Position *pos,
                                                    Accuracy *acc,
                                                    LSHandle *sh,
                                                    const char *key,
                                                    const char *payload,
                                                    LSError *lserror)
{
    LSSubscriptionIter *iter = NULL;
    LSMessage *msg = NULL;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    jschema_ref input_schema = NULL ;
    JSchemaInfo schemaInfo;
    int minDist, minInterval;
    bool isNonSubscibePresent = false;

    LS_LOG_DEBUG("key = %s\n", key);

    if (!LSSubscriptionAcquire(sh, key, &iter, lserror))
        return false;

    input_schema = jschema_parse (j_cstr_to_buffer(SCHEMA_ANY),
                                  DOMOPT_NOOPT, NULL);
    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);

    while (LSSubscriptionHasNext(iter)) {
        msg = LSSubscriptionNext(iter);

        //parse message to get minDistance
        if (!LSMessageIsSubscription(msg))
            isNonSubscibePresent = true;

        if (pos == NULL || acc == NULL) {
            // means error string will be returned
            LSMessageReplyLocUpdateCase(msg, sh, key, payload, iter, lserror);
        } else {
            minInterval = minDist = 0;

            parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)),
                                   DOMOPT_NOOPT, &schemaInfo);

            if (!jis_null(parsedObj)) {
                if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumInterval"), &jsonSubObject))
                    jnumber_get_i32(jsonSubObject, &minInterval);

                if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &jsonSubObject))
                    jnumber_get_i32(jsonSubObject, &minDist);

                j_release(&parsedObj);
            }

            if (meetsCriteria(msg, pos, acc, minInterval, minDist))
                LSMessageReplyLocUpdateCase(msg, sh, key, payload, iter, lserror);
        }

        // check for key sub list and gps_nw sub list*/
        if (isNonSubscibePresent) {
            if (strcmp(key, SUBSC_GET_LOC_UPDATES_HYBRID_KEY) == 0) {

                LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY and GPS_CRITERIA_KEY empty");

                //check gps list empty
                if ((isSubscListFilled(sh, msg, SUBSC_GET_LOC_UPDATES_HYBRID_KEY, false) ==  false) &&
                    (isSubscListFilled(sh, msg, SUBSC_GET_LOC_UPDATES_GPS_KEY, false) == false)) {
                    LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY and GPS_CRITERIA_KEY empty");
                    stopNonSubcription(SUBSC_GET_LOC_UPDATES_GPS_KEY);
                }

                //check nw list empty and stop nw handler
                if ((isSubscListFilled(sh, msg, SUBSC_GET_LOC_UPDATES_HYBRID_KEY, false) == false) &&
                    (isSubscListFilled(sh, msg, SUBSC_GET_LOC_UPDATES_NW_KEY, false) == false)) {
                    LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY and NW_CRITERIA_KEY empty");
                    stopNonSubcription(SUBSC_GET_LOC_UPDATES_NW_KEY);
                }
            } else if ((isSubscListFilled(sh, msg, key, false) == false) &&
                       (isSubscListFilled(sh, msg, SUBSC_GET_LOC_UPDATES_HYBRID_KEY, false) == false)) {
                LS_LOG_DEBUG("key %s empty",key);
                stopNonSubcription(key);
            }
        }
    }

    LSSubscriptionRelease(iter);
    jschema_release(&input_schema);

    return true;
}



bool LocationService::LSMessageReplyLocUpdateCase(LSMessage *msg,
                                                  LSHandle *sh,
                                                  const char *key,
                                                  const char *payload,
                                                  LSSubscriptionIter *iter,
                                                  LSError *lserror)
{
    bool retVal = false;

    retVal = LSMessageReply(sh, msg, payload, lserror);

    if (!LSMessageIsSubscription(msg)) {
        /*remove from subscription list*/
        LSSubscriptionRemove(iter);

        /*remove from request list*/
        if(!LSMessageRemoveReqList(msg))
            LS_LOG_ERROR("Message is not found in the LocationUpdateRequestPtr");
    } else {
        removeTimer(msg);
    }

    return retVal;
}


bool LocationService::removeTimer(LSMessage *message)
{
    std::vector<LocationUpdateRequestPtr>::iterator it;
    int size = m_locUpdate_req_list.size();
    int count = 0;
    bool found = false;
    TimerData *timerdata;
    guint timerID;
    bool timerRemoved;

    LS_LOG_INFO("size = %d",size);
    if (size <= 0) {
        LS_LOG_ERROR("m_locUpdate_req_list is empty");
        return false;
    }

    for (it = m_locUpdate_req_list.begin(); count < size; ++it, count++) {
        if (it->get()->getMessage() == message) {
            timerID = it->get()->getTimerID();

            if(timerID != 0)
               timerRemoved = g_source_remove (timerID);

            timerdata = it->get()->getTimerData();
            if (timerdata != NULL) {
                delete timerdata;
                timerdata = NULL;
                it->get()->setTimerData(timerdata);
            }
            LS_LOG_INFO("timerRemoved %d timerID %d", timerRemoved, timerID);
            found = true;
            break;
        }

    }
    return found;
}


bool LocationService::LSMessageRemoveReqList(LSMessage *message)
{
    std::vector<LocationUpdateRequestPtr>::iterator it;
    int size = m_locUpdate_req_list.size();
    int count = 0;
    bool found = false;
    TimerData *timerdata;
    guint timerID;
    bool timerRemoved;

    LS_LOG_INFO("m_locUpdate_req_list size %d",size);

    if (size <= 0) {
        LS_LOG_ERROR("m_locUpdate_req_list is empty");
        return false;
    }

    for (it = m_locUpdate_req_list.begin(); count < size; ++it, count++) {
        if (it->get()->getMessage() == message) {
            timerID = it->get()->getTimerID();
            timerRemoved = g_source_remove (timerID);
            timerdata = it->get()->getTimerData();
            LS_LOG_INFO("timerRemoved %d timerID %d", timerRemoved, timerID);
            if (timerdata != NULL) {
                delete timerdata;
                timerdata = NULL;
                it->get()->setTimerData(timerdata);
            }
            m_locUpdate_req_list.erase(it);
            found = true;
            break;
        }

    }
    return found;
}



bool LocationService::meetsCriteria(LSMessage *msg,
                                    Position *pos,
                                    Accuracy *acc,
                                    int minInterval,
                                    int minDist)
{
    LSMessage *listmsg = NULL;
    std::vector<LocationUpdateRequestPtr>::iterator it;
    bool bMeetsInterval, bMeetsDistance, bMeetsCriteria;
    struct timeval tv;
    long long currentTime, elapsedTime;

    gettimeofday(&tv, (struct timezone *) NULL);
    currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    bMeetsCriteria = false;

    for (it = m_locUpdate_req_list.begin(); it != m_locUpdate_req_list.end(); ++it) {
        if (*it == NULL)
            continue;

        listmsg = it->get()->getMessage();
        if (msg == listmsg) {
            if (it->get()->getFirstReply()) {
                it->get()->updateRequestTime(currentTime);
                it->get()->updateLatAndLong(pos->latitude, pos->longitude);
                it->get()->updateFirstReply(false);
                bMeetsCriteria = true;
            } else {
                // check if it's cached one
                if (pos->timestamp != 0) {
                    bMeetsInterval = bMeetsDistance = true;

                    elapsedTime = currentTime - it->get()->getRequestTime();

                    if (minInterval > 0) {
                        if (elapsedTime <= minInterval)
                            bMeetsInterval = false;
                    }

                    if (minDist > 0) {
                        if (!loc_geometry_calc_distance(pos->latitude,
                                                        pos->longitude,
                                                        it->get()->getLatitude(),
                                                        it->get()->getLongitude()) >= minDist)
                            bMeetsDistance = false;
                    }

                    if (bMeetsInterval && bMeetsDistance) {
                        if (it->get()->getHandlerType() == HANDLER_HYBRID) {
                            if (elapsedTime > 60000 || acc->horizAccuracy < MINIMAL_ACCURACY)
                                bMeetsCriteria = true;
                        } else {
                            bMeetsCriteria = true;
                        }
                    }

                    if (bMeetsCriteria) {
                        it->get()->updateRequestTime(currentTime);
                        it->get()->updateLatAndLong(pos->latitude, pos->longitude);
                    }
                }
            }

            break;
        }
    }

    return bMeetsCriteria;
}

bool LocationService::isSubscribeTypeValid(LSHandle *sh, LSMessage *message, bool isMandatory, bool *isSubscription)
{
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubArg = NULL;

    if (!LSMessageValidateSchema(sh, message, SCHEMA_ANY, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in isSubscribeTypeValid\n");
        return false;
    }


    if (jobject_get_exists(parsedObj,J_CSTR_TO_BUF("subscribe"), &jsonSubArg)) {

        if (isSubscription)
            jboolean_get(jsonSubArg , isSubscription);
    } else {

        if (isMandatory) {
            LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
            j_release(&parsedObj);
            return false;
        }

        if (isSubscription)
            *isSubscription = false;
    }

    j_release(&parsedObj);
    return true;
}
