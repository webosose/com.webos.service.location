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
using namespace std;
/**
 @var LSMethod LocationService::rootMethod[]
 methods belonging to root category
 */
LSMethod LocationService::rootMethod[] = {
    { "getNmeaData", LocationService::_getNmeaData },
    { "getCurrentPosition",LocationService::_getCurrentPosition },
    { "startTracking", LocationService::_startTracking },
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
        || pthread_mutex_init(&track_lock, NULL) || pthread_mutex_init(&pos_lock, NULL)
        || pthread_mutex_init(&nmea_lock, NULL) || pthread_mutex_init(&sat_lock, NULL)
        || pthread_mutex_init(&track_wifi_lock, NULL) || pthread_mutex_init(&track_cell_lock, NULL)
        || pthread_mutex_init(&pos_nw_lock, NULL) || pthread_mutex_init(&geofence_add_lock, NULL)
        || pthread_mutex_init(&geofence_pause_lock, NULL) || pthread_mutex_init(&geofence_remove_lock, NULL)
        || pthread_mutex_init(&geofence_resume_lock, NULL)){
        return false;
    }

    criteriaCatSvrc = NULL;
    criteriaCatSvrc = LunaCriteriaCategoryHandler::getInstance();

    if (criteriaCatSvrc == NULL) {
        LS_LOG_ERROR("criteriaCatSvrc is NULL");
        return false;
    }

    LS_LOG_DEBUG("handler_array[HANDLER_GPS] = %u",handler_array[HANDLER_GPS]);

    criteriaCatSvrc->init(mServiceHandle, mlgeServiceHandle, handler_array);
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
    bRetVal = LSPalmServiceRegisterCategory(*msvcHandle, "/geofence", geofenceMethod, prvMethod, NULL, this, &mLSError);
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

    if (m_reqlist.empty() == false) {
        m_reqlist.clear();
    }

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

    if (pthread_mutex_destroy(&lbs_geocode_lock)
        || pthread_mutex_destroy(&lbs_reverse_lock)|| pthread_mutex_destroy(&track_lock)
        || pthread_mutex_destroy(&pos_lock) || pthread_mutex_destroy(&nmea_lock)
        || pthread_mutex_destroy(&sat_lock) || pthread_mutex_destroy(&track_wifi_lock)
        || pthread_mutex_destroy(&track_cell_lock) || pthread_mutex_destroy(&pos_nw_lock)
        || pthread_mutex_destroy(&geofence_add_lock) ||  pthread_mutex_destroy(&geofence_pause_lock)
        ||  pthread_mutex_destroy(&geofence_remove_lock) ||  pthread_mutex_destroy(&geofence_resume_lock))
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
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);
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

/**
 * <Funciton >   getCurrentPosition
 * <Description>  gets the location from handler
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    bool, if successful return true else false
 */
bool LocationService::getCurrentPosition(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("======getCurrentPosition=======");
    LSMessageRef(message);
    int accuracyLevel = 0;
    int responseTimeLevel = 0;
    int maximumAge = 0;
    unsigned char reqHandlerType = 0;
    TimerData *timerdata = NULL;
    Request *request = NULL;
    LSError mLSError;
    bool isGpsReqSucess = false, isCellIdReqSucess = false, isWifiReqSucess = false;
    bool bRetVal = false;
    bool mboolmaximumAge = false;
    //HandlerStatus handlerStatus;
    unsigned char handlerStatus = 0;
    LocationErrorCode errorCode = LOCATION_SUCCESS;
    jvalue_ref serviceObject = NULL;
    jvalue_ref maximumAgeObj = NULL;
    jvalue_ref parsedObj = NULL;

    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_CURRENT_POSITION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getCurrentPosition");
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM);
        return true;
    }

    //TODO check posistion data with maximum age in cache
    if (handler_Selection(sh, message, data, &accuracyLevel, &responseTimeLevel, &handlerStatus, parsedObj) == false) {
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }

    //check in cache first if maximumAge is -1

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("maximumAge"), &maximumAgeObj)) {
        mboolmaximumAge=true;
        jnumber_get_i32(maximumAgeObj,&maximumAge);
    }

    if (mboolmaximumAge && maximumAge != READ_FROM_CACHE) {
        LS_LOG_DEBUG("maximumAge= %d", maximumAge);
        bRetVal = readLocationfromCache(sh, message, serviceObject, maximumAge, accuracyLevel, handlerStatus);

        if (bRetVal == true) {
            return true;
        }
    }

    //Before selecting Handler check all handlers disabled from settings then throw LocationServiceOFF error
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_ERROR("Both handler are OFF\n");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    //Only wifi request, check wifistate
    if (!(handlerStatus & HANDLER_GPS_BIT) && (handlerStatus & HANDLER_WIFI_BIT) && !(handlerStatus & HANDLER_CELLID_BIT) && wifistate == false) {
        LS_LOG_ERROR("wifi disabled");
        errorCode = LOCATION_WIFI_CONNECTION_OFF;
        // data connection off throw error.
        goto EXIT;
    }

    //Only wifi or cellid request, check dataconnection state
    if (!(handlerStatus & HANDLER_GPS_BIT) && ((handlerStatus & HANDLER_WIFI_BIT) || (handlerStatus & HANDLER_CELLID_BIT))
        && (isInternetConnectionAvailable == false && isWifiInternetAvailable == false)) {
        LS_LOG_ERROR("No Internet Connection");
        errorCode = LOCATION_DATA_CONNECTION_OFF;
        // data connection off throw error.
        goto EXIT;
    }

    /*Handler GPS*/
    if ((handlerStatus & HANDLER_GPS_BIT) && getHandlerStatus(GPS)) {
        LOCATION_ASSERT(pthread_mutex_lock(&pos_lock) == 0);
        isGpsReqSucess = reqLocationToHandler(HANDLER_GPS, &reqHandlerType, HANDLER_GPS, sh);
        LOCATION_ASSERT(pthread_mutex_unlock(&pos_lock) == 0);
    }

    /*Handler WIFI*/
    if ((handlerStatus & HANDLER_WIFI_BIT) && getHandlerStatus(NETWORK) && (wifistate) && (isInternetConnectionAvailable || isWifiInternetAvailable)) {
        LOCATION_ASSERT(pthread_mutex_lock(&pos_nw_lock) == 0);
        isWifiReqSucess = reqLocationToHandler(HANDLER_NW, &reqHandlerType, HANDLER_WIFI, sh);
        LOCATION_ASSERT(pthread_mutex_unlock(&pos_nw_lock) == 0);
    }

    //NETWORK HANDLER start and make request of type cellid

    /*Handler CELL-ID*/
    if ((handlerStatus & HANDLER_CELLID_BIT) && getHandlerStatus(NETWORK) && (isTelephonyAvailable && (isInternetConnectionAvailable || isWifiInternetAvailable))) {
        LOCATION_ASSERT(pthread_mutex_lock(&pos_nw_lock) == 0);
        isCellIdReqSucess = reqLocationToHandler(HANDLER_NW, &reqHandlerType, HANDLER_CELLID, sh);
        LOCATION_ASSERT(pthread_mutex_unlock(&pos_nw_lock) == 0);
    }

    if ((isGpsReqSucess == false) && (isWifiReqSucess == false) && (isCellIdReqSucess == false)) {
        LS_LOG_ERROR("No handlers started");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    } else {
        //Start Timer
        int timeinSec = 0;
        int responseTime = 0;
        guint timerID = 0;

        responseTime = convertResponseTimeFromLevel(accuracyLevel, responseTimeLevel);
        timeinSec = responseTime / TIME_SCALE_SEC;
        timerdata = new(std::nothrow) TimerData(message, sh, reqHandlerType, NULL);

        if (timerdata == NULL) {
            LS_LOG_ERROR("timerdata null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        timerID = g_timeout_add_seconds(timeinSec, &TimerCallback, timerdata);
        LS_LOG_DEBUG("Add to Requestlist handlerType=%x timerID=%d", reqHandlerType, timerID);
        //Add to Requestlist
        request = new(std::nothrow) Request(message, sh, timerID, reqHandlerType, timerdata);

        if (request == NULL) {
            LS_LOG_ERROR("req null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        boost::shared_ptr<Request> req(request);
        m_reqlist.push_back(req);
        LS_LOG_DEBUG("message=%p sh=%p", message, sh);
        LS_LOG_INFO("Handler Started are gpsHandlerStarted=%d,wifiHandlerStarted=%d,cellidHandlerStarted=%d", isGpsReqSucess, isWifiReqSucess,
                    isCellIdReqSucess);
    }

EXIT:

        if (!jis_null(parsedObj))
            j_release(&parsedObj);

        if (!jis_null(serviceObject))
            j_release(&serviceObject);

        if (errorCode != LOCATION_SUCCESS) {
            LSMessageReplyError(sh, message, errorCode);
            LSMessageUnref(message);
        }

        return true;
}

bool LocationService::reqLocationToHandler(int handler_type,
                                           unsigned char *reqHandlerType,
                                           int subHandlerType,
                                           LSHandle *sh)
{
    int ret;
    LS_LOG_DEBUG("reqLocationToHandler handler_type %d subHandlerType %d", handler_type, subHandlerType);
    ret = handler_start(HANDLER_INTERFACE(handler_array[handler_type]), subHandlerType);

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("Failed to start handler errorcode %d", ret);
        return false;
    } else {
        //LS_LOG_DEBUG("%s handler started successfully",handlerNames[subHandlerType]);
        ret = handler_get_position(HANDLER_INTERFACE(handler_array[handler_type]), START, wrapper_getCurrentPosition_cb, NULL, subHandlerType, sh);

        if (ret == ERROR_NOT_AVAILABLE) {
            LS_LOG_ERROR("getCurrentPosition failed errorcode %d", ret);
            handler_get_position(HANDLER_INTERFACE(handler_array[handler_type]), STOP, wrapper_getCurrentPosition_cb, NULL, subHandlerType, sh);
            handler_stop(HANDLER_INTERFACE(handler_array[handler_type]), subHandlerType, false);
            return false;
        }

        *reqHandlerType |= REQ_HANDLER_TYPE(subHandlerType);
        return true;
    }
}

int convertAccuracyLeveltoAccuracy(int accuracylevel)
{
    int accuracy;

    switch (accuracylevel) {
        case ACCURACY_LEVEL_LOW :     //read from cellid
            accuracy = 2000;
            break;

        case ACCURACY_LEVEL_MEDIUM:   //read from wifi
            accuracy = 500;
            break;

        case ACCURACY_LEVEL_HIGH :    //read from gps
            accuracy = 100;
            break;

        default:
            accuracy = 2000;
    }

    return accuracy;
}

/**
 * <Funciton >   readLocationfromCache
 * <Description>  gets the location from cache
 * @param     LunaService handle
 * @param     LunaService message
 * @param     json_object serviceObject
 * @param     int maxAge
 * @param     HandlerStatus handlerstatus
 * @return    bool, if successful return true else false
 */
bool LocationService::readLocationfromCache(LSHandle *sh,
                                            LSMessage *message,
                                            jvalue_ref serviceObject,
                                            int maxAge,
                                            int accuracylevel,
                                            unsigned char handlerstatus)
{
    LS_LOG_INFO("=======readLocationfromCache=======");
    bool gpsCacheSuccess = false;
    bool wifiCacheSuccess = false;
    bool cellCacheSucess = false;
    bool bRetVal;
    Position pos;
    Accuracy acc;
    Position gpspos;
    Accuracy gpsacc;
    Position wifipos;
    Accuracy wifiacc;
    Position cellidpos;
    Accuracy cellidacc;
    LSError mLSError;

    LSErrorInit(&mLSError);

    memset(&gpspos, 0, sizeof(Position));
    memset(&wifipos, 0, sizeof(Position));
    memset(&cellidpos, 0, sizeof(Position));
    memset(&gpsacc, 0, sizeof(Accuracy));
    memset(&wifiacc, 0, sizeof(Accuracy));
    memset(&cellidacc, 0, sizeof(Accuracy));
    memset(is_geofenceId_used,0,sizeof(is_geofenceId_used));

    if (getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), &gpspos, &gpsacc, HANDLER_GPS))
        gpsCacheSuccess = true;

    if (getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]), &wifipos, &wifiacc, HANDLER_WIFI))
        wifiCacheSuccess = true;

    if (getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]), &cellidpos, &cellidacc, HANDLER_CELLID))
        cellCacheSucess = true;

    int accuracy = convertAccuracyLeveltoAccuracy(accuracylevel);

    if (maxAge == ALWAYS_READ_FROM_CACHE) {
        //Match accuracy in all cached data
        if (gpsCacheSuccess && (gpsacc.horizAccuracy <= accuracy)) {
            pos = gpspos;
            acc = gpsacc;
        } else if (wifiCacheSuccess && (wifiacc.horizAccuracy <= accuracy)) {
            pos = wifipos;
            acc = wifiacc;
        } else if (cellCacheSucess && (cellidacc.horizAccuracy <= accuracy)) {
            pos = cellidpos;
            acc = cellidacc;
        } else {
            LS_LOG_INFO("No Cache data present");
            LSMessageReplyError(sh, message, LOCATION_POS_NOT_AVAILABLE);

            return true;
        }

        //Read according to accuracy value
        location_util_add_pos_json(serviceObject, &pos);
        location_util_add_acc_json(serviceObject, &acc);

        if (pos.latitude != 0 && pos.longitude != 0) {
            bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            return true;
        } else
            return FALSE;
    } else {
        struct timeval tv;
        gettimeofday(&tv, (struct timezone *) NULL);
        long long currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
        long long longmaxAge = maxAge * TIME_SCALE_SEC;

        //More accurate data return first
        if (gpsCacheSuccess && (longmaxAge >= currentTime - gpspos.timestamp) && (gpsacc.horizAccuracy <= accuracy)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &gpspos);
            location_util_add_acc_json(serviceObject, &gpsacc);

            bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            } else
                return true;
        } else if (wifiCacheSuccess && (longmaxAge >= currentTime - wifipos.timestamp) && (wifiacc.horizAccuracy <= accuracy)) {
            LS_LOG_DEBUG("currentTime %lld wifipos.timestamp %lld longmaxAge %lld", currentTime, wifipos.timestamp, longmaxAge);
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &wifipos);
            location_util_add_acc_json(serviceObject, &wifiacc);

            bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            }
            else
              return true;
        } else if (cellCacheSucess && (longmaxAge >= currentTime - cellidpos.timestamp) && (cellidacc.horizAccuracy <= accuracy)) {
            location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &cellidpos);
            location_util_add_acc_json(serviceObject, &cellidacc);

            bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            }
            else
                return true;
        }

        return false;
    }
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

/**
 * <Funciton >  handler_Selection
 * <Description>  Getting respective handler based on accuracy and power requirement
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    handler if successful else NULL
 */
bool LocationService::handler_Selection(LSHandle *sh,
                                        LSMessage *message,
                                        void *data,
                                        int *accuracy,
                                        int *responseTime,
                                        unsigned char *handlerStatus,
                                        jvalue_ref  parsedObj)
{
    LS_LOG_INFO("handler_Selection");
    bool mboolRespose = false;
    bool mboolAcc = false;
    jvalue_ref accuracyObj = NULL;
    jvalue_ref responseTimeObj = NULL;

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("accuracy"), &accuracyObj)){
        jnumber_get_i32(accuracyObj, accuracy);
        LS_LOG_DEBUG("accuracy=%d", *accuracy);
        mboolAcc = true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("responseTime"), &responseTimeObj)){
        jnumber_get_i32(responseTimeObj, responseTime);
        LS_LOG_DEBUG("responseTime=%d", *responseTime );
        mboolRespose=true;
    }

    LS_LOG_INFO("accuracy=%d responseTime=%d", *accuracy, *responseTime);

    if (mboolAcc || mboolRespose) {
        if (ACCURACY_LEVEL_LOW == *accuracy && RESPONSE_LEVEL_LOW == *responseTime) {
            *handlerStatus |= HANDLER_CELLID_BIT;  //CELLID
            //NEW REQUIREMENT to give position from any source quickly 25/10/2013[START]
            *handlerStatus |= HANDLER_WIFI_BIT;    //WIFI
            //NEW REQUIREMENT to give position from any source quickly 25/10/2013[END]
        } else if (*accuracy != ACCURACY_LEVEL_HIGH)
            *handlerStatus |= HANDLER_WIFI_BIT; //WIFI;

        if (*accuracy != ACCURACY_LEVEL_LOW)
            *handlerStatus |= HANDLER_GPS_BIT; //GPS;
    } else {
        LS_LOG_INFO("Default gps handler assigned\n");
        *handlerStatus |= HANDLER_GPS_BIT;   //GPS;
        //NEW REQUIREMENT to give position from any source quickly 25/10/2013[START]
        *handlerStatus |= HANDLER_CELLID_BIT;  //CELLID
        *handlerStatus |= HANDLER_WIFI_BIT;    //WIFI
        //NEW REQUIREMENT to give position from any source quickly 25/10/2013[END]
    }

    return true;
}

int LocationService::convertResponseTimeFromLevel(int accLevel, int responseTimeLevel)
{
    int responseTime;

    switch(responseTimeLevel) {

        case RESPONSE_LEVEL_LOW:
            responseTime = SHORT_RESPONSE_TIME;
        break;

        case RESPONSE_LEVEL_MEDIUM:
            responseTime = MEDIUM_RESPONSE_TIME;
        break;

        default:
            responseTime = LONG_RESPONSE_TIME;
        break;
    }

    LS_LOG_DEBUG("responseTime=%d", responseTime);
    return responseTime;
}

//only for GPS Handler
/**
 * <Funciton >   startTracking
 * <Description>  API to get repeated updates about location if subscribe is set to TRUE
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    bool if successful return true else false
 */
bool LocationService::startTracking(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("=======startTracking=======");
    //LSMessageRef(message); //Not required as the message is not stored
    unsigned char startedHandlers = 0;
    int ret;
    LSError mLSError;
    bool bRetVal;
    jvalue_ref parsedObj = NULL;
    LocationErrorCode errorCode = LOCATION_SUCCESS;
    LSErrorInit(&mLSError);

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_START_TRACKING, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in startTracking");
        return true;
    }

    //GPS and NW both are off
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_ERROR("GPS & NW status is OFF");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    if (getHandlerStatus(GPS)) {
        LS_LOG_DEBUG("gps is on in Settings");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS);

        if (ret == ERROR_NONE)
            startedHandlers |= HANDLER_GPS_BIT;
        else
            LS_LOG_DEBUG("GPS handler not started");
    }

    if (getHandlerStatus(NETWORK)) {
        LS_LOG_DEBUG("network is on in Settings");

        if (wifistate == true) {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_WIFI);

            if (ret == ERROR_NONE)
                startedHandlers |= HANDLER_WIFI_BIT;
            else
                LS_LOG_DEBUG("WIFI handler not started");
        }

        LS_LOG_INFO("Location Service isInternetConnectionAvailable %d isTelephonyAvailable %d", isInternetConnectionAvailable, isTelephonyAvailable);

        if (isTelephonyAvailable && (isInternetConnectionAvailable || wifistate)) {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_CELLID);

            if (ret == ERROR_NONE)
                startedHandlers |= HANDLER_CELLID_BIT;
            else
                LS_LOG_DEBUG("CELLID handler not started");
        }

        // Add to subsciption list with method+handler as key
    }

    if (startedHandlers == HANDLER_STATE_DISABLED) {
        LS_LOG_ERROR("None of the handlers started");
        errorCode = LOCATION_UNKNOWN_ERROR;
        goto EXIT;
    }

    bRetVal = LSSubscriptionAdd(sh, SUBSC_START_TRACK_KEY, message, &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        errorCode = LOCATION_UNKNOWN_ERROR;
        goto EXIT;
    }

    //is it only for GPS?
    if (startedHandlers & HANDLER_GPS_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_GPS], START, wrapper_startTracking_cb, NULL, HANDLER_GPS, LSPalmServiceGetPrivateConnection(mServiceHandle));
        trackhandlerstate |= HANDLER_GPS_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_lock) == 0);
    }

    if (startedHandlers & HANDLER_WIFI_BIT) {
        //TODO get all AP List and pass to handler.

        LOCATION_ASSERT(pthread_mutex_lock(&track_wifi_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_WIFI, LSPalmServiceGetPrivateConnection(mServiceHandle));
        trackhandlerstate |= HANDLER_WIFI_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_wifi_lock) == 0);
    }

    if (startedHandlers & HANDLER_CELLID_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_cell_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_CELLID, LSPalmServiceGetPrivateConnection(mServiceHandle));
        trackhandlerstate |= HANDLER_CELLID_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_cell_lock) == 0);
    }

EXIT :

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

    return true;
}

bool LocationService::getReverseLocation(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
#ifdef NOMINATIUM_LBS
    Address address;
    jvalue_ref serviceObject = NULL;
#else
    jvalue_ref arrObject = NULL;
    char *response = NULL;
    char *language = NULL;
    char *result_type = NULL;
    char *location_type = NULL;
    char *longitude = NULL;
    char *latitude = NULL;
    GString *pos_data = NULL;
    std::string strlng;
    std::string strlat;
#endif
    LSError mLSError;
    Position pos;
    LocationErrorCode errorCode = LOCATION_SUCCESS;


#ifdef NOMINATIUM_LBS
    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_REVERSE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getReverseLocation");
        return true;
    }
#else
    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_GOOGLE_REVERSE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getReverseLocation");
        return true;
    }
#endif

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        errorCode = LOCATION_DATA_CONNECTION_OFF;
        goto EXIT;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("latitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos.latitude);
#ifndef NOMINATIUM_LBS
        strlat = boost::lexical_cast<string>(pos.latitude);
#endif
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("longitude"), &jsonSubObject)) {
        jnumber_get_f64(jsonSubObject, &pos.longitude);
#ifndef NOMINATIUM_LBS
        strlng = boost::lexical_cast<string>(pos.longitude);
#endif
    }

#ifndef NOMINATIUM_LBS
    pos_data = g_string_new("latlng=");
    g_string_append(pos_data, strlat.c_str());
    g_string_append(pos_data, ",");
    g_string_append(pos_data, strlng.c_str());

    raw_buffer nameBuf;

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("language"), &jsonSubObject)){
        nameBuf = jstring_get(jsonSubObject);
        g_string_append(pos_data, "&language=");
        g_string_append(pos_data, nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("result_type"), &jsonSubObject)) {
        int size = jarray_size(jsonSubObject);
        g_string_append(pos_data,"&result_type=");
        LS_LOG_DEBUG("result_type size [%d]",size);
        for (int i = 0; i < size;i++) {
            arrObject = jarray_get(jsonSubObject, i);
            nameBuf = jstring_get(arrObject);

            if (i > 0)
                g_string_append(pos_data, "|");

            g_string_append(pos_data, nameBuf.m_str);
            LS_LOG_DEBUG("pos_data size [%s]",pos_data->str);
            jstring_free_buffer(nameBuf);
        }
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("location_type"), &jsonSubObject)) {
        int size = jarray_size(jsonSubObject);
        g_string_append(pos_data,"&location_type=");
        LS_LOG_DEBUG("location_type size [%d]",size);

        for (int i = 0; i < size; i++) {
            arrObject = jarray_get(jsonSubObject, i);
            nameBuf = jstring_get(arrObject);

            if (i > 0)
                g_string_append(pos_data, "|");

            g_string_append(pos_data, nameBuf.m_str);
            LS_LOG_DEBUG("location_type string [%s]",pos_data->str);

            jstring_free_buffer(nameBuf);
        }
    }
#endif
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_reverse_lock) == 0);

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);

    if (ret == ERROR_NONE) {
#ifdef NOMINATIUM_LBS
        ret = handler_get_reverse_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &pos, &address);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);
#else
        ret = handler_get_reverse_google_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), (char *)pos_data->str, wrapper_rev_geocoding_cb);
        LS_LOG_DEBUG("handler_get_reverse_google_geo_code %s", response);
#endif
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }

#ifndef NOMINATIUM_LBS
        if (LSSubscriptionAdd(sh, SUBSC_GET_REVGEOCODE_KEY, message, &mLSError) == FALSE) {
            LS_LOG_ERROR("Failed to add to subscription list");
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
#else
        serviceObject = jobject_create();

        if (jis_null(serviceObject)) {
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));

        if (address.street != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("street"), jstring_create(address.street));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("street"), jstring_create(""));

        if (address.countrycode != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("countrycode"), jstring_create(address.countrycode));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("countrycode"), jstring_create(""));

        if (address.postcode != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("postcode"), jstring_create(address.postcode));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("postcode"), jstring_create(""));

        if (address.area != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("area"), jstring_create(address.area));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("area"), jstring_create(""));

        if (address.locality != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("locality"), jstring_create(address.locality));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("locality"), jstring_create(""));

        if (address.region != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("region"), jstring_create(address.region));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("region"), jstring_create(""));

        if (address.country != NULL)
            jobject_put(serviceObject, J_CSTR_TO_JVAL("country"), jstring_create(address.country));
        else
            jobject_put(serviceObject, J_CSTR_TO_JVAL("country"), jstring_create(""));

        bool bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);
#endif
    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
    }

EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode);

#ifndef NOMINATIUM_LBS
    if (pos_data != NULL)
        g_string_free(pos_data, TRUE);
#else
    if (!jis_null(serviceObject))
        j_release(&serviceObject);
#endif

    return true;
}

bool LocationService::getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    Address addr = {0};
    Accuracy acc = {0};
    Position pos;
    LSError mLSError;
    jvalue_ref serviceObject = NULL;
    bool bRetVal;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
#ifndef NOMINATIUM_LBS
    jvalue_ref jsonComponetObject = NULL;
    jvalue_ref jsonBoundSubObject = NULL;
    char *response = NULL;
    raw_buffer nameBuf;
    GString *address_data = NULL;
#endif
    int errorCode = LOCATION_SUCCESS;

#ifdef NOMINATIUM_LBS
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
#else
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
#endif
    bRetVal = jobject_get_exists(parsedObj, J_CSTR_TO_BUF("address"), &jsonSubObject);

    if (bRetVal == true) {
        LS_LOG_DEBUG("value of address %s",jsonSubObject);
        raw_buffer nameBuf = jstring_get(jsonSubObject);
#ifdef NOMINATIUM_LBS
        addr.freeformaddr = g_strdup(nameBuf.m_str); // free the memory occupied
#else
        std::string str(nameBuf.m_str);
        boost::replace_all(str, " ","+");
        g_string_append(address_data,"address=");
        g_string_append(address_data, str.c_str());
        LS_LOG_DEBUG("value of address %s", address_data->str);
#endif
        jstring_free_buffer(nameBuf);
        addr.freeform = true;
    }
#ifdef NOMINATIUM_LBS
    else {
        LS_LOG_ERROR("Not free form");
        addr.freeform = false;

        if (location_util_parsejsonAddress(parsedObj, &addr) == false) {
            LS_LOG_ERROR ("value of no freeform failed");
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }

    }
#else
    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("language"), &jsonSubObject)) {
        nameBuf = jstring_get(jsonSubObject);
        g_string_append(address_data, "&language=");
        g_string_append(address_data, nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("bounds"), &jsonBoundSubObject)) {
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

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("components"), &jsonComponetObject)) {
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
#endif
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_geocode_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);

    if (ret == ERROR_NONE) {
        LS_LOG_ERROR ("value of addr.freeformaddr %s", addr.freeformaddr);
#ifdef NOMINATIUM_LBS
        ret = handler_get_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &addr, &pos, &acc);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);
#else
        ret = handler_get_google_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), address_data->str, wrapper_geocoding_cb);
#endif

        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);

        if (ret != ERROR_NONE) {
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
#ifdef NOMINATIUM_LBS
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
#else
        bRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GEOCODE_KEY, message, &mLSError);

        if (bRetVal == false) {
            LS_LOG_ERROR("Failed to add to subscription list");
            LSErrorPrintAndFree(&mLSError);
            errorCode = LOCATION_UNKNOWN_ERROR;
            goto EXIT;
        }
#endif
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

#ifdef NOMINATIUM_LBS
    if (addr.freeformaddr != NULL)
        g_free(addr.freeformaddr);

    if (addr.freeform == false) {
        if (addr.street)
            g_free(addr.street);

        if (addr.country)
            g_free(addr.country);

        if (addr.postcode)
            g_free(addr.postcode);

        if (addr.countrycode)
            g_free(addr.countrycode);

        if (addr.area)
            g_free(addr.area);

        if (addr.locality)
            g_free(addr.locality);

        if (addr.region)
            g_free(addr.region);
    }

    jschema_release(&input_schema);
 #else
    if (address_data != NULL)
        g_string_free(address_data, TRUE);
 #endif

    return true;
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

    if (handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS) != ERROR_NONE) {
        LS_LOG_ERROR("GPS handler start failed\n");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE);
        goto DONE_GET_GPS_STATUS;
    }

    if ((isSubscribeTypeValid(sh, message, false, &isSubscription)) && isSubscription) {
        LS_LOG_ERROR("Subscription call\n");

        if (LSSubscriptionAdd(sh, SUBSC_GPS_ENGINE_STATUS, message, &mLSError) == false) {
            LSErrorPrint(&mLSError, stderr);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR);
            goto DONE_GET_GPS_STATUS;
        }

        int state = handler_get_gps_status((Handler *)handler_array[HANDLER_GPS], wrapper_gpsStatus_cb);

        if (state == GPS_STATUS_AVAILABLE)
            mCachedGpsEngineStatus = true;
        else
            mCachedGpsEngineStatus = false;

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

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

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
        }

        if ((strcmp(handler, NETWORK) == 0) && mNwStatus != state) {
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

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

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

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

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
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

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

    LS_LOG_INFO("get time to first fix %lld\n", mTTFF);

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
                      HANDLER_GPS) != ERROR_NONE) {
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
    int geofenceid = -1;
    int errorCode = LOCATION_SUCCESS;
    char str_geofenceid[32];
    LSError mLSError;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    LS_LOG_DEBUG("=======pauseGeofence======\n");

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_PAUSE_GEOFENCE_AREA, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in removeGeofenceArea\n");
        return true;
    }

    jobject_get_exists(parsedObj, J_CSTR_TO_BUF("geofenceid"), &jsonSubObject);
    jnumber_get_i32(jsonSubObject, &geofenceid);

    sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_ADD_AREA_KEY);
    if (isSubscListFilled(sh, message,str_geofenceid, false) == false) {
        errorCode = LOCATION_GEOFENCE_ID_UNKNOWN;
        goto EXIT;
    }

    if (handler_pause_geofence((Handler *) handler_array[HANDLER_GPS],
                               TRUE,
                               &geofenceid,
                               wrapper_geofence_pause_cb) != ERROR_NONE) {
        errorCode = LOCATION_UNKNOWN_ERROR;
    } else {
        LSErrorInit(&mLSError);
        sprintf(str_geofenceid, "%d%s", geofenceid, SUBSC_GEOFENCE_PAUSE_AREA_KEY);

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

bool LocationService::getLocationUpdates(LSHandle *sh, LSMessage *message, void *data) {

    LS_LOG_INFO("=======getLocationUpdates=======");
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
        LS_LOG_DEBUG("minimumDistance %d", minInterval);
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

    if((strcmp(handlerName, GPS) == 0) && (getHandlerStatus(GPS) == false)) {
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }
    if((strcmp(handlerName, NETWORK) == 0) && (getHandlerStatus(NETWORK) == false)) {
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
        int timeinSec = 0;
        guint timerID = 0;

        timerdata = new(std::nothrow) TimerData(message, sh, startedHandlers,key);
        if (timerdata == NULL) {
            LS_LOG_ERROR("timerdata null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        LS_LOG_INFO("timerdata %p", timerdata);
        if (responseTime != 0)
            timerID = g_timeout_add_seconds(responseTime, &TimerCallbackLocationUpdate, timerdata);

        LocationUpdateRequest *locUpdateReq = new(std::nothrow) LocationUpdateRequest(message,
                                                                                      reqTime,
                                                                                      timerID,
                                                                                      timerdata,
                                                                                      LunaCriteriaCategoryHandler::INVALID_LAT,
                                                                                      LunaCriteriaCategoryHandler::INVALID_LONG,
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

        if ((getConnectionManagerState() == false) && (getWifiInternetState() == false))
            errorCode = LOCATION_DATA_CONNECTION_OFF;
        else if (getWifiState() == false)
            errorCode = LOCATION_WIFI_CONNECTION_OFF;
        else
            errorCode = LOCATION_UNKNOWN_ERROR;

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

    LS_LOG_INFO("=======getCachedPosition=======");


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

    LS_LOG_DEBUG("maximumAge= %d", maximumAge);

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

    if (!jis_null(serviceObj))
        j_release(&serviceObj);

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
        LS_LOG_DEBUG("All handlers disabled");
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
                                HANDLER_WIFI);

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
                                HANDLER_CELLID);

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
                            HANDLER_GPS);

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

/**
 * <Funciton >   wrapper_startTracking_cb
 * <Description>  Callback function called for startTracking from handler
 * @param     <enable_cb> <In>
 * @param     <pos> <In> <Position Structure>
 * @param     <accuracy> <In> <Accuracy Structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void LocationService::wrapper_startTracking_cb(gboolean enable_cb,
                                               Position *pos,
                                               Accuracy *accuracy,
                                               int error,
                                               gpointer privateIns,
                                               int type)
{
    LS_LOG_DEBUG("wrapper_startTracking_cb");
    getInstance()->startTracking_reply(pos, accuracy, error, type);
}


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
void LocationService::wrapper_getCurrentPosition_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int handlerType)
{
    LS_LOG_DEBUG("wrapper_getCurrentPosition_cb called back from handlerType=%d", handlerType);
    getInstance()->getCurrentPosition_reply(position, accuracy, error, handlerType);
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

void LocationService::startTracking_reply(Position *pos, Accuracy *accuracy, int error, int type)
{
    LSError mLSError;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;
    long long current_time = 0;

    LS_LOG_DEBUG("==========startTracking_reply called============");

    LSErrorInit(&mLSError);

    serviceObject = jobject_create();
    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Out of memory\n");
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);
        goto EXIT;
    }

    if (error != ERROR_NONE) {
        switch (type) {
        case HANDLER_GPS:
            trackhandlerstate &= ~HANDLER_GPS_BIT;
            break;

        case HANDLER_WIFI:
            trackhandlerstate &= ~HANDLER_WIFI_BIT;
            break;

        case HANDLER_CELLID:
            trackhandlerstate &= ~HANDLER_CELLID_BIT;
            break;
        }

        if (trackhandlerstate == 0) {
            retString = LSMessageGetErrorReply(LOCATION_TIME_OUT);
        } else {
            retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
        }

        goto EXIT;
    }

    location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
    location_util_add_pos_json(serviceObject, pos);
    location_util_add_acc_json(serviceObject, accuracy);
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *) NULL);
    current_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    if (mIsStartFirstReply || accuracy->horizAccuracy < MINIMAL_ACCURACY) {
        mlastTrackingReplyTime = current_time;
        mIsStartFirstReply = false;
    } else {
        LS_LOG_DEBUG("Accuracy is not 100m current_time = %lld mlastTrackingReplyTime = %lld",current_time,mlastTrackingReplyTime);

        if (current_time - mlastTrackingReplyTime > 60000) {
            mlastTrackingReplyTime = current_time;
        } else {

            if (!jis_null(serviceObject))
                j_release(&serviceObject);

            return;
        }
    }

    retString = jvalue_tostring_simple(serviceObject);

EXIT:
    if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                              SUBSC_START_TRACK_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                              SUBSC_START_TRACK_KEY,
                                              retString,
                                              &mLSError)) {
        LSErrorPrintAndFree(&mLSError);
    }

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

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

void LocationService::getCurrentPosition_reply(Position *pos, Accuracy *accuracy, int error, int handlerType)
{
    int count = 0;
    LSError mLSError;
    TimerData *timerdata = NULL;
    jvalue_ref serviceObject = NULL;
    char *retString = NULL;

    LS_LOG_INFO("======getCurrentPosition_reply called=======\n");

    LSErrorInit(&mLSError);
    serviceObject = jobject_create();
    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Out of memory\n");
        retString = LSMessageGetErrorReply(LOCATION_OUT_OF_MEM);

        if (!LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                  SUBSC_GPS_GET_CURR_POSITION_KEY,
                                                  retString,
                                                  &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        if (!LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                  SUBSC_GPS_GET_CURR_POSITION_KEY,
                                                  retString,
                                                  &mLSError)) {
            LSErrorPrintAndFree(&mLSError);
        }

        return;
    }

    if (error == ERROR_NONE) {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);

        retString = jvalue_tostring_simple(serviceObject);
    } else {
        retString = LSMessageGetErrorReply(LOCATION_POS_NOT_AVAILABLE);
    }

    /*List Implementation*/
    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();

    if (size <= 0) {
        LS_LOG_ERROR("List is empty");
        j_release(&serviceObject);
        return;
    }

    for (it = m_reqlist.begin(); count < size; ++it, count++) {
         LS_LOG_DEBUG("count = %d size= %d", count, size);

        //Match Handlertype and GetReqhandlerType
         if (*it != NULL) {
             LS_LOG_DEBUG("it->get()->GetReqHandlerType()=%d", it->get()->GetReqHandlerType());

             if ((1u << handlerType) & it->get()->GetReqHandlerType()) {
                 LS_LOG_DEBUG("handlerType = %d bit is set in the Request", handlerType);
                 LS_LOG_DEBUG("it->get()->GetHandle() = %p it->get()->GetMessage() = %p", it->get()->GetHandle(), it->get()->GetMessage());
                 LS_LOG_DEBUG("Payload=%s", LSMessageGetPayload(it->get()->GetMessage()));
                 LS_LOG_DEBUG("App ID = %s", LSMessageGetApplicationID(it->get()->GetMessage()));

                 if (handlerType == HANDLER_WIFI || handlerType == HANDLER_CELLID)
                     handler_stop(handler_array[HANDLER_NW], handlerType, false);
                 else {
                     handler_get_position(handler_array[HANDLER_GPS], STOP, wrapper_getCurrentPosition_cb, NULL, HANDLER_GPS, NULL);
                     handler_stop(handler_array[handlerType], handlerType, false);
                 }

                //Multiple handler started, one failed then wait for other reply [START]
                 it->get()->UpdateReqHandlerType((it->get()->GetReqHandlerType()) ^ (1u << handlerType));
                 LS_LOG_INFO("Resetting bit handlerType %d after reset it->get()->GetReqHandlerType() = %d", handlerType,
                             it->get()->GetReqHandlerType());

                 if (it->get()->GetReqHandlerType() == 0 || error == ERROR_NONE) {
                     LSMessageReply(it->get()->GetHandle(), it->get()->GetMessage(), retString, &mLSError);
                     LSMessageUnref(it->get()->GetMessage());
                     //Remove timer
                     g_source_remove(it->get()->GetTimerID());
                     timerdata = it->get()->GetTimerData();

                     if (timerdata != NULL) {
                         delete timerdata;
                         timerdata = NULL;
                     }

                     //Remove from list
                     m_reqlist.erase(it);
                 }
            }
        }
    }

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

    if (!jis_null(serviceObject))
        j_release(&serviceObject);
}

void LocationService::geocoding_reply(char *response, int error, int type)
{
    LSError mLSError;
    char *retString = NULL;
    const char *json_end = NULL;
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

    // Call jdomparser_feed for every part of incoming json string. It will be done byte by byte.
    json_end = response + strlen(response);

    for (const char *i = response ; i != json_end ; ++i) {
        if (!jdomparser_feed(parser, i, 1)) {
            retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
            goto EXIT;
        }
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

    if (response != NULL)
        g_free(response);
}

void LocationService::rev_geocoding_reply(char *response, int error, int type)
{
    LSError mLSError;
    char *retString = NULL;
    const char *json_end = NULL;
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

    // Call jdomparser_feed for every part of incoming json string. It will be done byte by byte.
    json_end = response + strlen(response);

    for (const char *i = response ; i != json_end ; ++i) {
        if (!jdomparser_feed(parser, i, 1)) {
            retString = LSMessageGetErrorReply(LOCATION_UNKNOWN_ERROR);
            goto EXIT;
        }
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

    if (response != NULL)
        g_free(response);
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
    } else if (key != NULL && (strcmp(key, CRITERIA_KEY) == 0)) {
        criteriaStopSubscription(sh, message);

    //check for getLocUpdatelist key list
    } else if (key != NULL && (strcmp(key, GET_LOC_UPDATE_KEY) == 0)) {
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

        LS_LOG_DEBUG("LSSubscriptionHasNext = %d ", LSSubscriptionHasNext(iter));

        if (LSSubscriptionHasNext(iter) == false)
            stopSubcription(sh, key);
        }
        LSSubscriptionRelease(iter);


    }
    return true;
}


/**
 * <Funciton >   criteriaStopSubscription

 * <Description>  Stop gps engine handling for criteria request when all gps request are empty

 * @param     LunaService handle

 * @param     LunaService message

 * @return    void
 */
void LocationService::criteriaStopSubscription(LSHandle *sh, LSMessage *message) {

    LS_LOG_DEBUG("criteriaStopSubscription");

    if (!isSubscListFilled(sh, message, NW_CRITERIA_KEY, true) &&
        !isSubscListFilled(sh, message, GPS_NW_CRITERIA_KEY, true)) {
        LS_LOG_INFO("criteriaStopSubscription Stopping NW handler");
        stopSubcription(sh, NW_CRITERIA_KEY);
    }

    if (!isSubscListFilled(sh, message, GPS_CRITERIA_KEY, true) &&
        !isSubscListFilled(sh, message, GPS_NW_CRITERIA_KEY, true)) {
        LS_LOG_INFO("criteriaStopSubscription Stopping GPS Engine");
        stopSubcription(sh, GPS_CRITERIA_KEY);
    }

    if (!isSubscListFilled(sh, message,NW_CRITERIA_KEY, true) &&
        !isSubscListFilled(sh, message,GPS_NW_CRITERIA_KEY, true)) {
        LS_LOG_INFO("criteriaStopSubscription Stopping NW handler");
        stopSubcription(sh, NW_CRITERIA_KEY);
    }


    criteriaCatSvrc->removeMessageFromCriteriaReqList(message);
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
    if (strcmp(key, SUBSC_START_TRACK_KEY) == 0) {

        //STOP GPS
        if (!(getHandlerStatus(GPS) == HANDLER_STATE_DISABLED)) {

            handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
                                   NULL, HANDLER_GPS, NULL);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

        }

        //STOP CELLID
        if (!(getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {

            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                   NULL, HANDLER_CELLID, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

            //STOP WIFI
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                   NULL, HANDLER_WIFI, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

        }
        this->mIsStartFirstReply = true;
        mIsStartTrackProgress = false;

    } else if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {

        handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], STOP,
                               wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetNmeaSubProgress = false;

    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {

        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP,
                                       wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetSatSubProgress = false;

    } else if (strcmp(key,GPS_CRITERIA_KEY) == 0) {

        //STOP GPS
        handler_start_tracking_criteria(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
                                        NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,NW_CRITERIA_KEY) == 0) {

        //STOP CELLID
        handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                        NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

        //STOP WIFI
        handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                        NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_GPS_KEY) == 0) {

        //STOP GPS TODO change to proper Handler API after handler implementation
        handler_get_location_updates(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
                                     NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_NW_KEY) == 0) {

        //STOP CELLID TODO change to proper Handler API after handler implementation
        handler_get_location_updates(handler_array[HANDLER_NW], STOP,
                                     wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

        //STOP WIFI
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                     NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

    }

}

void LocationService::stopNonSubcription(const char *key) {
    if (strcmp(key, SUBSC_START_TRACK_KEY) == 0) {
        //STOP GPS
        if (!(getHandlerStatus(GPS) == HANDLER_STATE_DISABLED)) {
            handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
            NULL, HANDLER_GPS, NULL);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        }

        if (!(getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
            //STOP CELLID
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
            NULL, HANDLER_CELLID, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

            //STOP WIFI
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
            NULL, HANDLER_WIFI, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        }

    } else if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {
        handler_get_nmea_data(handler_array[HANDLER_GPS], STOP,
                               wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {
        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP,
                                       wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    }  else if ((strcmp(key, GPS_CRITERIA_KEY) == 0) ) {
        //STOP GPS
        if (!(getHandlerStatus(GPS) == HANDLER_STATE_DISABLED)) {
            handler_start_tracking_criteria(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
                                            NULL, HANDLER_GPS, NULL);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        }
    } else if ((strcmp(key, NW_CRITERIA_KEY) == 0)) {

        if (!(getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
            //STOP CELLID
            handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                            NULL, HANDLER_CELLID, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

            //STOP WIFI
            handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                            NULL, HANDLER_WIFI, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        }

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_GPS_KEY) == 0) {
        //STOP GPS
        handler_get_location_updates(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb,
                                     NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key,SUBSC_GET_LOC_UPDATES_NW_KEY) == 0) {
        //STOP CELLID
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                     NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

        //STOP WIFI
        handler_get_location_updates(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb,
                                     NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);

    }
}

/*Timer Implementation START */
gboolean LocationService::_TimerCallback(void *data)
{
    TimerData *timerdata = (TimerData *) data;

    if (timerdata == NULL) {
        LS_LOG_ERROR("TimerCallback timerdata is null");
        return false;
    }

    LSMessage *message = timerdata->GetMessage();
    LSHandle *sh = timerdata->GetHandle();
    unsigned char handlerType = timerdata->GetHandlerType();

    if (message == NULL || sh == NULL) {
        LS_LOG_ERROR("message or Handle is null");
        return false;
    } else {
        //Remove from list
        if (replyAndRemoveFromRequestList(sh, message, handlerType) == false)
            LS_LOG_DEBUG("Failed in replyAndRemoveFromRequestList");
    }

    // return false otherwise it will be called repeatedly until it returns false
    return false;
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
    unsigned char handlerType = timerdata->GetHandlerType();
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
bool LocationService::replyAndRemoveFromRequestList(LSHandle *sh, LSMessage *message, unsigned char handlerType)
{
    bool found = false;
    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();
    int count = 0;
    LSError mLSError;
    TimerData *timerdata;

    LSErrorInit(&mLSError);

    if (size <= 0) {
        LS_LOG_ERROR("List is empty");
        return false;
    }

    for (it = m_reqlist.begin(); count < size; ++it, count++) {
        if (it->get()->GetMessage() == message) {
            LS_LOG_INFO("Timeout Message is found in the list");

            if (HANDLER_GPS_BIT & handlerType) {
                handler_get_position(handler_array[HANDLER_GPS], STOP, wrapper_getCurrentPosition_cb, NULL, HANDLER_GPS, NULL);
                handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
            }

            if (HANDLER_WIFI_BIT & handlerType) {
                handler_get_position(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb, NULL, HANDLER_WIFI, NULL);
                handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
            }

            if (HANDLER_CELLID_BIT & handlerType) {
                handler_get_position(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb, NULL, HANDLER_CELLID, NULL);
                handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
            }

            LSMessageReplyError(sh, message, LOCATION_TIME_OUT);
            LSMessageUnref(message);
            timerdata = it->get()->GetTimerData();

            if (timerdata != NULL) {
                delete timerdata;
                timerdata = NULL;
            }

            m_reqlist.erase(it);
            found = true;
            break;
        }
    }

    /*message not found in the list*/
    return found;
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
                        if (!minDistance(minDist,
                                         pos->latitude,
                                         pos->longitude,
                                         it->get()->getLatitude(),
                                         it->get()->getLongitude()))
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


/* Vincenty formula. WGS-84 */
//minDistance(minDistance,pos->latitude,pos->latitude,mlastLat,mlastLong)
bool LocationService::minDistance(int minimumDistance, double latitude1, double longitude1, double latitude2 , double longitude2)
{
    double lambdaP, iter_limit = 100.0;
    double sin_sigma, sin_alpha, cos_sigma, sigma,  sq_cos_alpha, cos_2sigma, C;
    double sq_u, cal1, cal2, delta_sigma, cal_dist;
    double sin_lambda, cos_lambda;

    const double a = 6378137.0, b = 6356752.314245,  f = 1 / 298.257223563;
    double delta_lon = ((longitude2 - longitude1) * MATH_PI / 180);
    double u_1 = atan((1 - f) * tan((latitude1) * MATH_PI / 180));
    double u_2 = atan((1 - f) * tan((latitude2) * MATH_PI / 180));

    double lambda = delta_lon;
    double sin_u1 = sin(u_1);
    double cos_u1 = cos(u_1);
    double sin_u2 = sin(u_2);
    double cos_u2 = cos(u_2);

    LS_LOG_DEBUG("latitude1 = %f longitude1 = %f latitude2 = %f longitude2 = %f",latitude1,longitude1,latitude2,longitude2);
    do {
        sin_lambda = sin(lambda);
        cos_lambda = cos(lambda);
        sin_sigma = sqrt((cos_u2 * sin_lambda) * (cos_u2 * sin_lambda) + \
                         (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda) * \
                         (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda));

        if (sin_sigma == 0) {
            LS_LOG_DEBUG("co-incident points");
            return false;  // co-incident points
        }

        cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos_lambda;
        sigma = atan2(sin_sigma, cos_sigma);
        sin_alpha = cos_u1 * cos_u2 * sin_lambda / sin_sigma;
        sq_cos_alpha = 1.0 - sin_alpha * sin_alpha;
        cos_2sigma = cos_sigma - 2.0 * sin_u1 * sin_u2 / sq_cos_alpha;

        if (isnan(cos_2sigma))
            cos_2sigma = 0;

        C = f / 16.0 * sq_cos_alpha * (4.0 + f * (4.0 - 3.0 * sq_cos_alpha));
        lambdaP = lambda;
        lambda = delta_lon + (1.0 - C) * f * sin_alpha * \
                 (sigma + C * sin_sigma * (cos_2sigma + C * cos_sigma * (-1.0 + 2.0 * cos_2sigma * cos_2sigma)));
    } while (abs(lambda - lambdaP) > 1e-12 && --iter_limit > 0);

    if (iter_limit == 0)
    {
       LS_LOG_DEBUG("iter_limit");
       return false;
    }

    sq_u = sq_cos_alpha * (a * a - b * b) / (b * b);
    cal1 = 1.0 + sq_u / 16384.0 * (4096.0 + sq_u * (-768.0 + sq_u * (320.0 - 175.0 * sq_u)));
    cal2 = sq_u / 1024.0 * (256.0 + sq_u * (-128.0 + sq_u * (74.0 - 47.0 * sq_u)));
    delta_sigma = cal2 * sin_sigma * (cos_2sigma + cal2 / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos_2sigma * cos_2sigma) - \
                                      cal2 / 6.0 * cos_2sigma * (-3.0 + 4.0 * sin_sigma * sin_sigma) * (-3.0 + 4.0 * cos_2sigma * cos_2sigma)));
    cal_dist = b * cal1 * (sigma - delta_sigma);

    LS_LOG_DEBUG("Cal_distance = %f minimumDistance = %d",cal_dist, minimumDistance);

    if (cal_dist >= minimumDistance) {
        LS_LOG_DEBUG("Meets Minimum Distance creteria");
        return true;
    }
    LS_LOG_DEBUG("Fails to meet Minimum Distance creteria");
    return false;
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

