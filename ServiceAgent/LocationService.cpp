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
#include <lunaprefs.h>

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
    { 0, 0 }
};

LSMethod LocationService::prvMethod[] = {
    { "sendExtraCommand", LocationService::_sendExtraCommand },
    { "stopGPS",LocationService::_stopGPS },
    { "setGPSParameters", LocationService::_setGPSParameters },
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
        || pthread_mutex_init(&pos_nw_lock, NULL)) {
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

    if (pthread_mutex_destroy(&lbs_geocode_lock) || pthread_mutex_destroy(&lbs_reverse_lock)
        || pthread_mutex_destroy(&track_lock)
        || pthread_mutex_destroy(&pos_lock) || pthread_mutex_destroy(&nmea_lock)
        || pthread_mutex_destroy(&sat_lock) || pthread_mutex_destroy(&track_wifi_lock)
        || pthread_mutex_destroy(&track_cell_lock) || pthread_mutex_destroy(&pos_nw_lock))
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
    LSMessagePrint(message, stdout);

    if (isSubscribeTypeValid(sh, message, false, NULL) == false)
        return true;

    if (getHandlerStatus(GPS) == false) {
        LSMessageReplyError(sh, message, LOCATION_LOCATION_OFF, locationErrorText[LOCATION_LOCATION_OFF]);
        return true;
    }

    if (handler_array[HANDLER_GPS] != NULL)
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);
    else {
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
        return true;
    }

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("Error HANDLER_GPS not started");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE, locationErrorText[LOCATION_START_FAILURE]);
        return true;
    }

    // Add to subsciption list with method name as key
    LS_LOG_DEBUG("isSubcriptionListEmpty = %d", isSubscListFilled(sh, SUBSC_GPS_GET_NMEA_KEY, false));
    bool mRetVal;
    mRetVal = LSSubscriptionAdd(sh, SUBSC_GPS_GET_NMEA_KEY, message, &mLSError);

    if (mRetVal == false) {
        LS_LOG_ERROR("Failed to add to subscription list");
        LSErrorPrintAndFree(&mLSError);
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
        return true;
    }

    LS_LOG_DEBUG("Call getNmeaData handler");

    LOCATION_ASSERT(pthread_mutex_lock(&nmea_lock) == 0);
    ret = handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], START, wrapper_getNmeaData_cb, NULL);
    LOCATION_ASSERT(pthread_mutex_unlock(&nmea_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        LS_LOG_ERROR("Error in getNmeaData");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

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
    struct json_object *serviceObject = NULL;
    struct json_object *m_JsonArgument = NULL;
    struct json_object *m_JsonSubArgument = NULL;
    int accuracyLevel = 0;
    int responseTimeLevel = 0;
    unsigned char reqHandlerType = 0;
    TimerData *timerdata;
    Request *request = NULL;
    LSError mLSError;
    bool isGpsReqSucess = false, isCellIdReqSucess = false, isWifiReqSucess = false;
    bool bRetVal;
    //HandlerStatus handlerStatus;
    unsigned char handlerStatus = 0;

    LSErrorInit(&mLSError);
    /*Initialize all handlers status*/
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_ERROR("serviceObject NULL Out of memory");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        LSMessageUnref(message);
        return true;
    }

    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_ERROR("parsing error");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        return true;
    }

    //TODO check posistion data with maximum age in cache
    if(handler_Selection(sh, message, data, &accuracyLevel, &responseTimeLevel, &handlerStatus) == false) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        return true;
     }


    //check in cache first if maximumAge is -1
    bRetVal = json_object_object_get_ex(m_JsonArgument, "maximumAge", &m_JsonSubArgument);

    if (bRetVal == true) {
        int maximumAge;
      if(m_JsonSubArgument == NULL || !json_object_is_type(m_JsonSubArgument,json_type_int)) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        return true;
      }

        maximumAge = json_object_get_int(m_JsonSubArgument);

        if (maximumAge != READ_FROM_CACHE) {
            LS_LOG_DEBUG("maximumAge= %d", maximumAge);
            bRetVal = readLocationfromCache(sh, message, serviceObject, maximumAge, accuracyLevel, handlerStatus);

            if (bRetVal == true) {
                json_object_put(serviceObject);
                json_object_put(m_JsonArgument);
                LSMessageUnref(message);
                return true;
            }

            //failed to get data from cache continue
        }
    }

    //Before selecting Handler check all handlers disabled from settings then throw LocationServiceOFF error
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_ERROR("Both handler are OFF\n");
        LSMessageReplyError(sh, message, LOCATION_LOCATION_OFF, locationErrorText[LOCATION_LOCATION_OFF]);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        LSMessageUnref(message);
        return true;
    }

    //Only wifi request, check wifistate
    if (!(handlerStatus & HANDLER_GPS_BIT) && (handlerStatus & HANDLER_WIFI_BIT) && !(handlerStatus & HANDLER_CELLID_BIT) && wifistate == false) {
        LS_LOG_ERROR("wifi disabled");

        LSMessageReplyError(sh, message, LOCATION_WIFI_CONNECTION_OFF, locationErrorText[LOCATION_WIFI_CONNECTION_OFF]); // data connection off throw error.

        if (serviceObject != NULL)
            json_object_put(serviceObject);

        json_object_put(m_JsonArgument);
        LSMessageUnref(message);
        return true;
    }

    //Only wifi or cellid request, check dataconnection state
    if (!(handlerStatus & HANDLER_GPS_BIT) && ((handlerStatus & HANDLER_WIFI_BIT) || (handlerStatus & HANDLER_CELLID_BIT))
            && (isInternetConnectionAvailable == false && isWifiInternetAvailable == false)) {
        LS_LOG_ERROR("No Internet Connection");

        LSMessageReplyError(sh, message, LOCATION_DATA_CONNECTION_OFF, locationErrorText[LOCATION_DATA_CONNECTION_OFF]); // data connection off throw error.

        if (serviceObject != NULL)
            json_object_put(serviceObject);

        json_object_put(m_JsonArgument);
        LSMessageUnref(message);
        return true;
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

        LSMessageReplyError(sh, message, LOCATION_LOCATION_OFF, locationErrorText[LOCATION_LOCATION_OFF]);

        if (serviceObject != NULL)
            json_object_put(serviceObject);

        json_object_put(m_JsonArgument);
        LSMessageUnref(message);
        return true;
    } else {
        int timeinSec;
        int responseTime;
        guint timerID = 0;
        //Start Timer
        responseTime = convertResponseTimeFromLevel(accuracyLevel, responseTimeLevel);
        timeinSec = responseTime / TIME_SCALE_SEC;
        timerdata = new(std::nothrow) TimerData(message, sh, reqHandlerType);

        if (timerdata == NULL) {
            LS_LOG_ERROR("timerdata null Out of memory");
            LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);

            if (serviceObject != NULL)
               json_object_put(serviceObject);

            json_object_put(m_JsonArgument);
            LSMessageUnref(message);
            return true;
        }

        timerID = g_timeout_add_seconds(timeinSec, &TimerCallback, timerdata);
        LS_LOG_DEBUG("Add to Requestlist handlerType=%x timerID=%d", reqHandlerType, timerID);
        //Add to Requestlist
        request = new(std::nothrow) Request(message, sh, timerID, reqHandlerType, timerdata);

        if (request == NULL) {
            LS_LOG_ERROR("req null Out of memory");
            LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);

            if (serviceObject != NULL)
               json_object_put(serviceObject);

            json_object_put(m_JsonArgument);
            LSMessageUnref(message);
            return true;
        }

        boost::shared_ptr<Request> req(request);
        m_reqlist.push_back(req);
        LS_LOG_DEBUG("message=%p sh=%p", message, sh);
        LS_LOG_INFO("Handler Started are gpsHandlerStarted=%d,wifiHandlerStarted=%d,cellidHandlerStarted=%d", isGpsReqSucess, isWifiReqSucess,
                    isCellIdReqSucess);

        if (serviceObject != NULL)
           json_object_put(serviceObject);

        json_object_put(m_JsonArgument);
        return true;
    }
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
                                            json_object *serviceObject,
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
            location_util_add_returnValue_json(serviceObject, false);
            location_util_add_errorCode_json(serviceObject, LOCATION_POS_NOT_AVAILABLE);
            location_util_add_errorText_json(serviceObject, locationErrorText[LOCATION_POS_NOT_AVAILABLE]);
            bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            return true;
        }

        //Read according to accuracy value
        location_util_add_pos_json(serviceObject, &pos);
        location_util_add_acc_json(serviceObject, &acc);

        if (pos.latitude != 0 && pos.longitude != 0) {
            bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            return true;
        } else
            return FALSE;
    } else {
        long long currentTime = ::time(0);
        currentTime *= TIME_SCALE_SEC; // milli sec
        long long longmaxAge = maxAge * TIME_SCALE_SEC;

        //More accurate data return first
        if (gpsCacheSuccess && (longmaxAge >= currentTime - gpspos.timestamp) && (gpsacc.horizAccuracy <= accuracy)) {
            location_util_add_returnValue_json(serviceObject, true);
            location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &gpspos);
            location_util_add_acc_json(serviceObject, &gpsacc);
            bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            } else
                return true;
        } else if (wifiCacheSuccess && (longmaxAge >= currentTime - wifipos.timestamp) && (wifiacc.horizAccuracy <= accuracy)) {
            LS_LOG_DEBUG("currentTime %lld wifipos.timestamp %lld longmaxAge %lld", currentTime, wifipos.timestamp, longmaxAge);
            location_util_add_returnValue_json(serviceObject, true);
            location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &wifipos);
            location_util_add_acc_json(serviceObject, &wifiacc);
            bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            }
            else
                return true;
        } else if (cellCacheSucess && (longmaxAge >= currentTime - cellidpos.timestamp) && (cellidacc.horizAccuracy <= accuracy)) {
            location_util_add_returnValue_json(serviceObject, true);
            location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
            location_util_add_pos_json(serviceObject, &cellidpos);
            location_util_add_acc_json(serviceObject, &cellidacc);
            bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            } else
                return true;
        }

        return false;
    }
}

bool LocationService::getCachedDatafromHandler(Handler *hdl, Position *pos, Accuracy *acc, int type)
{
    bool ret;
    LSError mLSError;
    LSErrorInit(&mLSError);
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
                                        unsigned char *handlerStatus)
{
    LS_LOG_INFO("handler_Selection");
    bool mboolRespose = false;
    bool mboolAcc = false;
    struct json_object *m_JsonArgument = NULL;
    struct json_object *m_JsonSubArgument = NULL;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_ERROR("m_JsonArgument Parsing Error");
        return false;
    }

    mboolAcc = json_object_object_get_ex(m_JsonArgument, "accuracy", &m_JsonSubArgument);

    if(m_JsonSubArgument == NULL || (mboolAcc && !json_object_is_type(m_JsonSubArgument, json_type_int))) {
    json_object_put(m_JsonArgument);
    return false;
    }

    if (mboolAcc)
       *accuracy = json_object_get_int(m_JsonSubArgument);

    m_JsonSubArgument = NULL;
    mboolRespose = json_object_object_get_ex(m_JsonArgument, "responseTime", &m_JsonSubArgument);

    if(m_JsonSubArgument == NULL || (mboolRespose && !json_object_is_type(m_JsonSubArgument, json_type_int))) {
    json_object_put(m_JsonArgument);
    return false;
    }

    if (mboolRespose)
       *responseTime = json_object_get_int(m_JsonSubArgument);

    if(mboolAcc && (*accuracy < ACCURACY_LEVEL_HIGH || *accuracy > ACCURACY_LEVEL_LOW)) {
       json_object_put(m_JsonArgument);
       return false;
    }
    if(mboolRespose && (*responseTime < RESPONSE_LEVEL_LOW || *responseTime  > RESPONSE_LEVEL_HIGH)) {
       json_object_put(m_JsonArgument);
       return false;
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

    json_object_put(m_JsonArgument);
    return true;
}

int LocationService::convertResponseTimeFromLevel(int accLevel, int responseTimeLevel)
{
    int responseTime;

    if (responseTimeLevel == RESPONSE_LEVEL_LOW) {
        //High 10 sec
        responseTime = SHORT_RESPONSE_TIME; //proper value 10000
    } else if (responseTimeLevel == RESPONSE_LEVEL_MEDIUM) {
        // mediuRemoveCellWifiInfoListnerm range 30 sec to 60 sec
        if (accLevel == ACCURACY_LEVEL_HIGH) {
            // if need greater accuracy without any time limit then  go for the big response time as
            // GPS fix may take that long
            responseTime = MEDIUM_RESPONSE_TIME;
        } else
            responseTime = MEDIUM_RESPONSE_TIME;
    } else {
        //Low 100 sec to 60 sec
        if (accLevel == ACCURACY_LEVEL_HIGH) {
            // if need greater accuracy without any time limit then  go for the big response time as
            // GPS fix may take that long
            responseTime = LONG_RESPONSE_TIME;
        } else
            responseTime = MEDIUM_RESPONSE_TIME;
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
    LSMessagePrint(message, stdout);
    int ret;
    LSError mLSError;
    bool bRetVal;
    if (isSubscribeTypeValid(sh, message, false, NULL) == false)
        return true;

    //GPS and NW both are off
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_ERROR("GPS & NW status is OFF");
        LSMessageReplyError(sh, message, LOCATION_LOCATION_OFF, locationErrorText[LOCATION_LOCATION_OFF]);
        return true;
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

        if (wifistate == true) { //wifistate == true)
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
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
        return true;
    }

    bRetVal = LSSubscriptionAdd(sh, SUBSC_START_TRACK_KEY, message, &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
        return true;
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

    return true;
}

bool LocationService::getReverseLocation(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    bool bRetVal;
    Position pos;
    Address address;
    LSError mLSError;
    TrakingErrorCode errorCode = LOCATION_SUCCESS;
    jvalue_ref serviceObject = NULL;

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        LSMessageReplyError(sh, message, LOCATION_DATA_CONNECTION_OFF, locationErrorText[LOCATION_DATA_CONNECTION_OFF]);
        return true;
    }

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_REVERSE_LOCATION, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in getReverseLocation");
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("latitude"), &jsonSubObject)){
        jnumber_get_f64(jsonSubObject, &pos.latitude);
    } else {
        LS_LOG_DEBUG("Input parameter is invalid, no latitude in request");
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("longitude"), &jsonSubObject)){
        jnumber_get_f64(jsonSubObject, &pos.longitude);
    } else {
        LS_LOG_ERROR("Input parameter is invalid, no latitude in request");
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }

    if (pos.latitude < -90.0 || pos.latitude > 90.0 || pos.longitude < -180.0 || pos.longitude > 180.0)
    {
        LS_LOG_ERROR("Lat and long out of range");
        errorCode = LOCATION_INVALID_INPUT;
        goto EXIT;
    }
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_reverse_lock) == 0);

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);

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

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
        errorCode = LOCATION_START_FAILURE;
    }

EXIT:

     if (!jis_null(serviceObject))
         j_release(&serviceObject);

     if (!jis_null(parsedObj))
         j_release(&parsedObj);

     if (errorCode != LOCATION_SUCCESS)
         LSMessageReplyError(sh, message, errorCode, locationErrorText[errorCode]);

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
    int errorCode = LOCATION_SUCCESS;

    if (!isInternetConnectionAvailable && !isWifiInternetAvailable) {
        LSMessageReplyError(sh, message, LOCATION_DATA_CONNECTION_OFF, locationErrorText[LOCATION_DATA_CONNECTION_OFF]);
        return true;
    }

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

    bRetVal = jobject_get_exists(parsedObj, J_CSTR_TO_BUF("address"), &jsonSubObject);

    if (bRetVal == true) {
        raw_buffer nameBuf = jstring_get(jsonSubObject);
        addr.freeformaddr = g_strdup(nameBuf.m_str); // free the memory occupied
        jstring_free_buffer(nameBuf);
        addr.freeform = true;
    } else {
        LS_LOG_ERROR("Not free form");
        addr.freeform = false;

        if (location_util_parsejsonAddress(&parsedObj, &addr) == false) {
            LS_LOG_ERROR ("value of no freeform failed");
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }

    }

    LOCATION_ASSERT(pthread_mutex_lock(&lbs_geocode_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);

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

        location_util_form_json_reply(&serviceObject, true, LOCATION_SUCCESS);
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

    if (addr.freeformaddr != NULL)
        g_free(addr.freeformaddr);


    if (errorCode != LOCATION_SUCCESS)
        LSMessageReplyError(sh, message, errorCode, locationErrorText[errorCode]);

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    if (addr.freeform = false) {

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
    return true;
}

bool LocationService::getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data)
{
    jvalue_ref handlersArrayItem = NULL;
    jvalue_ref handlersArrayItem1 = NULL;
    jvalue_ref handlersArray = NULL;
    jvalue_ref serviceObject = NULL;
    jvalue_ref parsedObj = NULL;

    LSError mLSError;
    bool mhandler = false;
    bool bRetVal;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_ALL_LOCATION_HANDLERS, &parsedObj)) {
        return true;
    }

    handlersArrayItem = jobject_create();

    if (jis_null(handlersArrayItem)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    handlersArrayItem1 = jobject_create();

    if (jis_null(handlersArrayItem1)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        j_release(&handlersArrayItem);
        j_release(&parsedObj);
        return true;
    }

    handlersArray = jarray_create(NULL);;

    if (jis_null(handlersArray)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        j_release(&handlersArrayItem1);
        j_release(&handlersArrayItem);
        j_release(&parsedObj);
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
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
    location_util_form_json_reply(&serviceObject, true, LOCATION_SUCCESS);
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
    struct json_object *serviceObject = NULL;

    LS_LOG_INFO("=======Subscribe for getGpsStatus=======\n");

    if (isSubscribeTypeValid(sh, message, false, &isSubscription) == false)
        return true;

    LSErrorInit(&mLSError);

    if (handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS) != ERROR_NONE) {
        LS_LOG_ERROR("GPS handler start failed\n");
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE, locationErrorText[LOCATION_START_FAILURE]);

        goto DONE_GET_GPS_STATUS;
    }

    serviceObject = json_object_new_object();

    if (!serviceObject) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        goto DONE_GET_GPS_STATUS;
    }

    if (isSubscription) {
        LS_LOG_ERROR("Subscription call\n");

        if (LSSubscriptionAdd(sh, SUBSC_GPS_ENGINE_STATUS, message, &mLSError) == false) {
            LSErrorPrint(&mLSError, stderr);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);

            goto DONE_GET_GPS_STATUS;
        }

        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_boolean(mCachedGpsEngineStatus));

        if (!LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError))
            LSErrorPrint(&mLSError, stderr);

        handler_get_gps_status((Handler *)handler_array[HANDLER_GPS], wrapper_gpsStatus_cb);
    } else {
        LS_LOG_DEBUG("Not subscription call\n");

        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_boolean(mCachedGpsEngineStatus));
        if (!LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError))
           LSErrorPrint(&mLSError, stderr);
    }

DONE_GET_GPS_STATUS:
    LSErrorFree(&mLSError);
    if (serviceObject)
        json_object_put(serviceObject);

    return true;
}

bool LocationService::getState(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_INFO("=======getState=======");
    int state;
    bool bRetVal;
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
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
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
                LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
                goto EXIT;
            }
            LS_LOG_DEBUG("handler_key=%s", subscription_key);
        }

        if (lpret != LP_ERR_NONE) {
            location_util_form_json_reply(&serviceObject, true, SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("state"),jnumber_create_i32(NULL));
            LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
            goto EXIT;
        }

            location_util_form_json_reply(&serviceObject, true, SUCCESS);
            jobject_put(serviceObject, J_CSTR_TO_JVAL("state"),jnumber_create_i32(state));

        LS_LOG_DEBUG("state=%d", state);

        LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);
    } else {
        LS_LOG_ERROR("LPAppGetHandle is not created ");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
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
    char *handler;
    bool state;
    bool bRetVal;
    LSError mLSError;
    LSErrorInit(&mLSError);
    struct json_object *m_JsonArgument = NULL;
    struct json_object *m_JsonSubArgument = NULL;
    struct json_object *serviceObject = NULL;
    LPAppHandle lpHandle = 0;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        return true;
    }

    //Read Handler from json
    bRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);

    if (m_JsonSubArgument != NULL && bRetVal && json_object_is_type(m_JsonSubArgument, json_type_string))
        handler = json_object_get_string(m_JsonSubArgument);
    else {
        LS_LOG_ERROR("Invalid param:handler");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    //Read state from json
    m_JsonSubArgument = NULL;
    bRetVal = json_object_object_get_ex(m_JsonArgument, "state", &m_JsonSubArgument);

    if (m_JsonSubArgument != NULL && bRetVal && json_object_is_type(m_JsonSubArgument, json_type_boolean)
            && ((strcmp(handler, GPS) == 0) || (strcmp(handler, NETWORK)) == 0))
        state = json_object_get_boolean(m_JsonSubArgument);
    else {
        LS_LOG_ERROR("Invalid param:state");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        LS_LOG_INFO("Writing handler state");
        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
            json_object_put(m_JsonArgument);
            return true;
        }

        LPAppSetValueInt(lpHandle, handler, state);
        LPAppFreeHandle(lpHandle, true);

        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
        json_object_object_add(serviceObject, "state", json_object_new_int(state));

        char subscription_key[MAX_GETSTATE_PARAM];
        strcpy(subscription_key, handler);
        strcat(subscription_key, SUBSC_GET_STATE_KEY);

        if ((strcmp(handler, GPS) == 0) && mGpsStatus != state) {
            mGpsStatus = state;

            bRetVal = LSSubscriptionRespond(mServiceHandle, subscription_key, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle, subscription_key, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            if (state == false)
                handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, true);
        }

        if ((strcmp(handler, NETWORK) == 0) && mNwStatus != state) {
            mNwStatus = state;

            bRetVal = LSSubscriptionRespond(mServiceHandle, subscription_key, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionRespond(mlgeServiceHandle, subscription_key, json_object_to_json_string(serviceObject), &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            if (state == false) {
                handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, true);
                handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, true);
            }
        }

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        json_object_put(serviceObject);
    } else {
        LS_LOG_DEBUG("LPAppGetHandle is not created");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

    json_object_put(m_JsonArgument);

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
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE, locationErrorText[LOCATION_START_FAILURE]);
        j_release(&parsedObj);
        return true;
    }

    int err = handler_set_gps_parameters(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), (gpointer) jvalue_tostring_simple(parsedObj));

    if (err == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("Error in %s", cmdStr);
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        j_release(&parsedObj);
        return true;
    }

    handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
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

bool LocationService::stopGPS(LSHandle *sh, LSMessage *message, void *data) {
    LS_LOG_INFO("=======stopGPS=======");
    bool bRetVal;
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
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    ret = handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, true);

    if (ret != ERROR_NONE) {
        LS_LOG_ERROR("GPS force stop failed");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);

    } else {
        location_util_form_json_reply(&serviceObject, true, SUCCESS);
        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

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
        LSMessageReplyError(sh, message, LOCATION_START_FAILURE, locationErrorText[LOCATION_START_FAILURE]);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }
    LS_LOG_INFO("command = %s", command);

    int err = handler_send_extra_command(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), command);

    if (err == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("Error in %s", command);
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }

    handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        j_release(&parsedObj);
        g_free(command);
        return true;
    }

    location_util_form_json_reply(&serviceObject, true, SUCCESS);
    jobject_put(serviceObject, J_CSTR_TO_JVAL("state"),jnumber_create_i32(NULL));

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
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        j_release(&parsedObj);
        return true;
    }

    if (NULL == handler) {
        LS_LOG_ERROR("ParamInput is NULL");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        j_release(&parsedObj);
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
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
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
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
    LSErrorInit(&mLSError);

    if (isSubscribeTypeValid(sh, message, false, NULL) == false)
        return true;

    if (getHandlerStatus(GPS)) {
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

        if (ret != ERROR_NONE) {
            LS_LOG_ERROR("GPS Handler not started");
            LSMessageReplyError(sh, message, LOCATION_START_FAILURE, locationErrorText[LOCATION_START_FAILURE]);
            return true;
        }
    } else {
        LS_LOG_ERROR("getGpsSatelliteData GPS OFF");
        LSMessageReplyError(sh, message, LOCATION_LOCATION_OFF, locationErrorText[LOCATION_LOCATION_OFF]);
        return true;
    }

    bRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GPS_SATELLITE_DATA, message, &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        return true;
    }

    LOCATION_ASSERT(pthread_mutex_lock(&sat_lock) == 0);
    ret = handler_get_gps_satellite_data(handler_array[HANDLER_GPS], START, wrapper_getGpsSatelliteData_cb);
    LOCATION_ASSERT(pthread_mutex_unlock(&sat_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR, locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

    return true;
}
bool LocationService::getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data)
{
    bool bRetVal ;
    long long mTTFF = handler_get_time_to_first_fix((Handler *) handler_array[HANDLER_GPS]);
    LSError mLSError;
    char str[MAX_LIMIT];
    struct json_object *serviceObject = NULL;

    LSErrorInit(&mLSError);
    sprintf(str, "%lld", mTTFF);
    LS_LOG_INFO("get time to first fix %lld\n", mTTFF);

    serviceObject = json_object_new_object();

    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
    json_object_object_add(serviceObject, "TTFF", json_object_new_string(str));

    bRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
    }
    json_object_put(serviceObject);
    return true;
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

/********************************Response to Application layer*********************************************************************/

void LocationService::startTracking_reply(Position *pos, Accuracy *accuracy, int error, int type)
{
    LS_LOG_DEBUG("==========startTracking_reply called============");
    struct json_object *serviceObject = NULL;
    LSError mLSError;
    bool bRetVal;

    LSErrorInit(&mLSError);

    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_ERROR("No memory");
        return;
    }

    if (error == ERROR_NONE) {
        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject, SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);

        if (mIsStartFirstReply || accuracy->horizAccuracy < MINIMAL_ACCURACY) {
            mlastTrackingReplyTime = time(0);
            mIsStartFirstReply = false;
            bRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                           SUBSC_START_TRACK_KEY,
                                                           json_object_to_json_string(serviceObject),
                                                           &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                           SUBSC_START_TRACK_KEY,
                                                           json_object_to_json_string(serviceObject),
                                                           &mLSError);
            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);
        } else {

            long long current_time = time(0);
            LS_LOG_DEBUG("Accuracy is not 100m current_time = %lld mlastTrackingReplyTime = %lld",current_time,mlastTrackingReplyTime);

            if (current_time - mlastTrackingReplyTime > 60) {
                bRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                               SUBSC_START_TRACK_KEY,
                                                               json_object_to_json_string(serviceObject),
                                                               &mLSError);

                if (bRetVal == false)
                    LSErrorPrintAndFree(&mLSError);

                bRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                               SUBSC_START_TRACK_KEY,
                                                               json_object_to_json_string(serviceObject),
                                                               &mLSError);
                if (bRetVal == false)
                    LSErrorPrintAndFree(&mLSError);

                mlastTrackingReplyTime = time(0);

            }
        }

    } else {
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
            location_util_add_returnValue_json(serviceObject, false);
            location_util_add_errorCode_json(serviceObject, LOCATION_TIME_OUT);
            location_util_add_errorText_json(serviceObject, locationErrorText[LOCATION_TIME_OUT]);

            bRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                           SUBSC_START_TRACK_KEY,
                                                           json_object_to_json_string(serviceObject),
                                                           &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                           SUBSC_START_TRACK_KEY,
                                                           json_object_to_json_string(serviceObject),
                                                           &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);
        }
    }

    json_object_put(serviceObject);
}

void LocationService::get_nmea_reply(long long timestamp, char *data, int length)
{
    LS_LOG_DEBUG("[DEBUG] get_nmea_reply called");
    bool bRetVal;
    struct json_object *serviceObject = NULL;
    LSError mLSError;

    LSErrorInit(&mLSError);
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_ERROR("Out of memory");
        return;
    }

    LS_LOG_DEBUG(" timestamp=%d\n", timestamp);
    LS_LOG_DEBUG("data=%s\n", data);
    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, SUCCESS);
    json_object_object_add(serviceObject, "timestamp", json_object_new_double(timestamp));

    if (data != NULL)
        json_object_object_add(serviceObject, "nmea", json_object_new_string(data));

    LS_LOG_DEBUG("mServiceHandle=%x\n", mServiceHandle);
    bRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle,
                                                   SUBSC_GPS_GET_NMEA_KEY,
                                                   json_object_to_json_string(serviceObject),
                                                   &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle,
                                                   SUBSC_GPS_GET_NMEA_KEY,
                                                   json_object_to_json_string(serviceObject),
                                                   &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);
    json_object_put(serviceObject);    //freeing the memory
}

void LocationService::getCurrentPosition_reply(Position *pos, Accuracy *accuracy, int error, int handlerType)
{
    LS_LOG_INFO("======getCurrentPosition_reply called=======");
    int count = 0;
    struct json_object *serviceObject = NULL;
    LSError mLSError;
    TimerData *timerdata;

    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_ERROR("Out of memory");
        return;
    }

    LSErrorInit(&mLSError);

    if (error == ERROR_NONE) {
        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject, SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);
    } else {
        LS_LOG_ERROR("error = %d ERROR_NONE =%d", error, ERROR_NONE);
        location_util_add_returnValue_json(serviceObject, false);
        location_util_add_errorCode_json(serviceObject, LOCATION_POS_NOT_AVAILABLE);
        location_util_add_errorText_json(serviceObject, locationErrorText[LOCATION_POS_NOT_AVAILABLE]);
    }

    /*List Implementation*/
    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();

    if (size <= 0) {
        LS_LOG_ERROR("List is empty");
        json_object_put(serviceObject);
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
                    LSMessageReply(it->get()->GetHandle(), it->get()->GetMessage(), json_object_to_json_string(serviceObject), &mLSError);
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

        //Multiple handler started, one failed then wait for other reply [START]
    }

    //Multiple handler started, one failed then wait for other reply [END]
    json_object_put(serviceObject);
}

void LocationService::getGpsSatelliteData_reply(Satellite *sat)
{
    LS_LOG_DEBUG("[DEBUG] getGpsSatelliteData_reply called, reply to application\n");
    int num_satellite_used_count = 0;
    bool bRetVal;
    char *resString;
    LSError mLSError;
    struct json_object *serviceObject = NULL;
    struct json_object *serviceArray = NULL;
    struct json_object *visibleSatelliteItem = NULL;
    LSErrorInit(&mLSError);
    /*Creating a json object*/
    serviceObject = json_object_new_object();

    if (sat == NULL) {

        if(serviceObject != NULL)
           json_object_put(serviceObject);

        LS_LOG_ERROR("satallite data NULL");
        return;
    }

    if (serviceObject == NULL) {
        LS_LOG_ERROR("serviceObject NULL Out of memory");
        return;
    }

    serviceArray = json_object_new_array();

    if (serviceArray == NULL) {
        json_object_put(serviceObject);
        LS_LOG_ERROR("serviceArray NULL Out of memory");
        return;
    }

    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject,LOCATION_SUCCESS);
    json_object_object_add(serviceObject, "visibleSatellites", json_object_new_int(sat->visible_satellites_count));

    while (num_satellite_used_count < sat->visible_satellites_count) {

        visibleSatelliteItem = json_object_new_object();

        if(visibleSatelliteItem == NULL) {
           json_object_put(serviceObject);
           json_object_put(serviceArray);
           return;
        }

        LS_LOG_DEBUG(" Service Agent value of %llf num_satellite_used_count%d ", sat->sat_used[num_satellite_used_count].azimuth,num_satellite_used_count);
        json_object_object_add(visibleSatelliteItem, "index", json_object_new_int(num_satellite_used_count));
        json_object_object_add(visibleSatelliteItem, "azimuth", json_object_new_double(sat->sat_used[num_satellite_used_count].azimuth));
        json_object_object_add(visibleSatelliteItem, "elevation", json_object_new_double(sat->sat_used[num_satellite_used_count].elevation));
        json_object_object_add(visibleSatelliteItem, "prn", json_object_new_int(sat->sat_used[num_satellite_used_count].prn));
        json_object_object_add(visibleSatelliteItem, "snr", json_object_new_double(sat->sat_used[num_satellite_used_count].snr));
        json_object_object_add(visibleSatelliteItem, "hasAlmanac", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasalmanac));
        json_object_object_add(visibleSatelliteItem, "hasEphemeris", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasephemeris));
        json_object_object_add(visibleSatelliteItem, "usedInFix", json_object_new_boolean(sat->sat_used[num_satellite_used_count].used));

        json_object_array_add(serviceArray, visibleSatelliteItem);
        num_satellite_used_count++;

    }
    json_object_object_add(serviceObject, "satellites", serviceArray);
    resString = json_object_to_json_string(serviceObject);

    bRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    bRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);

    if (bRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    json_object_put(serviceObject);
    json_object_put(visibleSatelliteItem);
    json_object_put(serviceArray);
    LS_LOG_DEBUG("[DEBUG] bRetVal=%d\n", bRetVal);
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
    struct json_object *serviceObject = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool isGpsEngineOn;
    int count = 0;

    serviceObject = json_object_new_object();

    if (state == GPS_STATUS_AVAILABLE)
        isGpsEngineOn = true;
    else
        isGpsEngineOn = false;

    mCachedGpsEngineStatus = isGpsEngineOn;

    if (serviceObject != NULL) {
        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_boolean(isGpsEngineOn));

        bool bRetVal = LSSubscriptionRespond(mServiceHandle,
                                             SUBSC_GPS_ENGINE_STATUS,
                                             json_object_to_json_string(serviceObject),
                                             &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        bRetVal = LSSubscriptionRespond(mlgeServiceHandle,
                                        SUBSC_GPS_ENGINE_STATUS,
                                        json_object_to_json_string(serviceObject),
                                        &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);
    }

    if (serviceObject != NULL)
        json_object_put(serviceObject);
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
    bool bRetVal;
    char *key = LSMessageGetMethod(message);
    LSSubscriptionIter *iter = NULL;

    LS_LOG_INFO("LSMessageGetMethod(message) %s", key);
    LSErrorInit(&mLSError);

    if (key == NULL) {
        LS_LOG_ERROR("Not a valid key");
        return true;
    }

    bRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

    if (bRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        return true;
    }

    if (LSSubscriptionHasNext(iter)) {
        LSMessage *msg = LSSubscriptionNext(iter);
        LS_LOG_DEBUG("LSSubscriptionHasNext= %d ", LSSubscriptionHasNext(iter));

        if (LSSubscriptionHasNext(iter) == false)
            stopSubcription(sh, key);
    }

    LSSubscriptionRelease(iter);
    //check for criteria key list
    if(key != NULL && (strcmp(key, CRITERIA_KEY) == 0))
       criteriaStopSubscription(sh, message);

    return true;
}

void LocationService::criteriaStopSubscription(LSHandle *sh, LSMessage *message) {
    LS_LOG_DEBUG("criteriaStopSubscription");

    if(!isSubscListFilled(sh, GPS_CRITERIA_KEY, true)){
        LS_LOG_INFO("criteriaStopSubscription Stopping GPS Engine");
        stopSubcription(sh, GPS_CRITERIA_KEY);
    }

    if(!isSubscListFilled(sh, NW_CRITERIA_KEY, true)){
        LS_LOG_INFO("criteriaStopSubscription Stopping NW handler");
        stopSubcription(sh, NW_CRITERIA_KEY);
    }

    criteriaCatSvrc->removeMessageFromCriteriaReqList(message);
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
    } else
        LS_LOG_DEBUG("[DEBUG] handle initialization failed\n");

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
            handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        }
        //STOP CELLID
        if (!(getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
            //STOP WIFI
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        }
        this->mIsStartFirstReply = true;
        mIsStartTrackProgress = false;
    } else if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {
        handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], STOP, wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetNmeaSubProgress = false;
    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {
        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP, wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetSatSubProgress = false;
    }
    else if(strcmp(key,GPS_CRITERIA_KEY) == 0) {
        //STOP GPS
        handler_start_tracking_criteria(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
    }
    else if(strcmp(key,NW_CRITERIA_KEY) == 0){
        //STOP CELLID
        handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
        //STOP WIFI
        handler_start_tracking_criteria(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
    }

}

void LocationService::stopNonSubcription(const char *key) {
    if (strcmp(key, SUBSC_START_TRACK_KEY) == 0) {
        //STOP GPS
        if (!(getHandlerStatus(GPS) == HANDLER_STATE_DISABLED)) {
            handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        }

        if (!(getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
            //STOP CELLID
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);

            //STOP WIFI
            handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
            handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        }

    } else if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {
        handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], STOP, wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {
        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP, wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);

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

            LSMessageReplyError(sh, message, LOCATION_TIME_OUT,locationErrorText[LOCATION_TIME_OUT]);
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
bool LocationService::isSubscListFilled(LSHandle *sh, const char *key,bool cancelCase)
{
    LSError mLSError;
    LSSubscriptionIter *iter = NULL;
    bool bRetVal;

    LSErrorInit(&mLSError);
    bRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

    if (bRetVal == false) {
        LSErrorPrint(&mLSError, stderr);
        LSErrorFree(&mLSError);
        return bRetVal;
    }
    if(LSSubscriptionHasNext(iter) && cancelCase) {
        LSMessage *msg = LSSubscriptionNext(iter);
    }
    bRetVal = LSSubscriptionHasNext(iter);

    LSSubscriptionRelease(iter);

    if(bRetVal == false)
       LS_LOG_ERROR("List is empty");

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

bool LocationService::LSSubscriptionNonSubscriptionReply(LSHandle *sh, const char *key, const char *payload, LSError *lserror)
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

    if (isNonSubscibePresent && (isSubscListFilled(sh, key,false) == false))
        stopNonSubcription(key);

    return retVal;
}

bool LocationService::isSubscribeTypeValid(LSHandle *sh, LSMessage *message, bool isMandatory, bool *isSubscription)
{
    struct json_object *jsonArg = NULL;
    struct json_object *jsonSubArg = NULL;
    bool bRetVal = false;

    jsonArg = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (!jsonArg || is_error(jsonArg)) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
        goto exit;
    }

    if (json_object_object_get_ex(jsonArg, "subscribe", &jsonSubArg)) {
        if (!jsonSubArg || is_error(jsonSubArg) || !json_object_is_type(jsonSubArg, json_type_boolean)) {
            LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
            goto exit;
        }

        if (isSubscription)
            *isSubscription = json_object_get_boolean(jsonSubArg);
    } else {
        if (isMandatory) {
            LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, locationErrorText[LOCATION_INVALID_INPUT]);
            goto exit;
        }

        if (isSubscription)
            *isSubscription = false;
    }

    bRetVal = true;

exit:
    if (jsonArg && !is_error(jsonArg))
        json_object_put(jsonArg);

    return bRetVal;
}

