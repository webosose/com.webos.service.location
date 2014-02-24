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
#include <Conf.h>
#include <LocationManagerService.h>
#include <string.h>
#include <Handler_Interface.h>
#include <Gps_Handler.h>
#include <Lbs_Handler.h>
#include <Network_Handler.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>
#include <JsonUtility.h>
#include <db_util.h>
#include <libxml/parser.h>
#include <Address.h>
#include <libxml/xmlmemory.h>
#include <unistd.h>
#include <LunaLocationService_Util.h>
#include <LocationService_Log.h>
#include <lunaprefs.h>
FILE *fp;
/**
 @var LSMethod LocationService::rootMethod[]
 methods belonging to root category
 */
LSMethod LocationService::rootMethod[] = {
    { "getNmeaData", LocationService::_getNmeaData },
    { "getCurrentPosition",LocationService::_getCurrentPosition },
    { "startTracking", LocationService::_startTracking },
    { "getReverseLocation",LocationService::_getReverseLocation },
    { "startTrackingBestPosition", LocationService::_startTrackingBestPosition },
    { "getGeoCodeLocation",LocationService::_getGeoCodeLocation },
    { "getAllLocationHandlers", LocationService::_getAllLocationHandlers },
    { "SetGpsStatus",LocationService::_SetGpsStatus },
    { "getGpsStatus", LocationService::_GetGpsStatus },
    { "setState", LocationService::_setState },
    { "getState", LocationService::_getState },
    { "sendExtraCommand", LocationService::_sendExtraCommand },
    { "getLocationHandlerDetails",LocationService::_getLocationHandlerDetails },
    { "getGpsSatelliteData", LocationService::_getGpsSatelliteData },
    { "getTimeToFirstFix",LocationService::_getTimeToFirstFix },
    { "test", LocationService::_test },
    { 0, 0 }
};

LocationService *LocationService::locService = NULL;
int LocationService::method = METHOD_NONE;

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
        mServiceHandle(0), mlgeServiceHandle(0), mGpsStatus(false), mNwStatus(false), mIsStartFirstReply(true), mIsGetNmeaSubProgress(false), mIsGetSatSubProgress(
                false), mIsStartTrackProgress(false)
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
        LS_LOG_DEBUG("com.palm.location service registration failed");
        return false;
    }

    if (locationServiceRegister(LOCATION_SERVICE_ALIAS_LGE_NAME, mainLoop, &mlgeServiceHandle) == false) {
        LS_LOG_DEBUG("com.lge.location service registration failed");
        return false;
    }

    LS_LOG_DEBUG("mServiceHandle =%x mlgeServiceHandle = %x", mServiceHandle, mlgeServiceHandle);
    g_type_init();
    LS_LOG_DEBUG("[DEBUG] %s::%s( %d ) init ,g_type_init called \n", __FILE__, __PRETTY_FUNCTION__, __LINE__);
    //load all handlers
    handler_array[HANDLER_GPS] = (Handler *)static_cast<GObject *>(g_object_new(HANDLER_TYPE_GPS, NULL));
    handler_array[HANDLER_NW] = (Handler *)static_cast<GObject *>(g_object_new(HANDLER_TYPE_NW, NULL));
    handler_array[HANDLER_LBS] = (Handler *)static_cast<GObject *>(g_object_new(HANDLER_TYPE_LBS, NULL));

    if ((handler_array[HANDLER_GPS] == NULL) || (handler_array[HANDLER_NW] == NULL) || (handler_array[HANDLER_LBS] == NULL))
        return false;

    //Load initial settings from DB
    mGpsStatus = loadHandlerStatus(GPS);
    mNwStatus = loadHandlerStatus(NETWORK);

    if (pthread_mutex_init(&lbs_geocode_lock, NULL) ||  pthread_mutex_init(&lbs_reverse_lock, NULL) ||
            pthread_mutex_init(&track_lock, NULL) || pthread_mutex_init(&pos_lock, NULL)
            || pthread_mutex_init(&nmea_lock, NULL) || pthread_mutex_init(&sat_lock, NULL)
            || pthread_mutex_init(&track_wifi_lock, NULL) || pthread_mutex_init(&track_cell_lock, NULL)
            || pthread_mutex_init(&pos_nw_lock, NULL)) {
        return false;
    }

    LS_LOG_DEBUG("Successfully LocationService object initialized");
    return true;
}
bool LocationService::locationServiceRegister(char *srvcname, GMainLoop *mainLoop, LSPalmService **msvcHandle)
{
    bool mRetVal; // changed to local variable from class variable
    LSError mLSError;
    LSErrorInit(&mLSError);
    // Register palm service
    mRetVal = LSRegisterPalmService(srvcname, msvcHandle, &mLSError);
    LSERROR_CHECK_AND_PRINT(mRetVal);
    //Gmain attach
    mRetVal = LSGmainAttachPalmService(*msvcHandle, mainLoop, &mLSError);
    LSERROR_CHECK_AND_PRINT(mRetVal);
    // add root category
    mRetVal = LSPalmServiceRegisterCategory(*msvcHandle, "/", rootMethod, NULL, NULL, this, &mLSError);
    LSERROR_CHECK_AND_PRINT(mRetVal);
    //Register cancel function cb to publicbus
    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPublicConnection(*msvcHandle), LocationService::_cancelSubscription, this, &mLSError);
    LSERROR_CHECK_AND_PRINT(mRetVal);
    //Register cancel function cb to privatebus
    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPrivateConnection(*msvcHandle), LocationService::_cancelSubscription, this, &mLSError);
    LSERROR_CHECK_AND_PRINT(mRetVal);
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

    if (pthread_mutex_destroy(&lbs_geocode_lock) || pthread_mutex_destroy(&lbs_reverse_lock) || pthread_mutex_destroy(&track_lock)
            || pthread_mutex_destroy(&pos_lock) || pthread_mutex_destroy(&nmea_lock) || pthread_mutex_destroy(&sat_lock)
            || pthread_mutex_destroy(&track_wifi_lock) || pthread_mutex_destroy(&track_cell_lock) || pthread_mutex_destroy(&pos_nw_lock)) {
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
    LS_LOG_DEBUG("======getNmeaData=====");
    int ret;
    LSError mLSError;
    LSErrorInit(&mLSError);
    //LSMessageRef(message); //Not required as the message is not stored
    LSMessagePrint(message, stdout);

    if (getHandlerStatus(GPS) == false) {
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);
        return true;
    }

    if (handler_array[HANDLER_GPS] != NULL)
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);
    else {
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    if (ret != ERROR_NONE) {
        LS_LOG_DEBUG("Error HANDLER_GPS not started");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    // Add to subsciption list with method name as key
    LS_LOG_DEBUG("isSubcriptionListEmpty = %d", isSubscListEmpty(sh, SUBSC_GPS_GET_NMEA_KEY));
    bool mRetVal;
    mRetVal = LSSubscriptionAdd(sh, SUBSC_GPS_GET_NMEA_KEY, message, &mLSError);

    if (mRetVal == false) {
        LS_LOG_DEBUG("Failed to add to subscription list");
        LSErrorPrintAndFree(&mLSError);
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR, trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    LS_LOG_DEBUG("Call getNmeaData handler");
    LOCATION_ASSERT(pthread_mutex_lock(&nmea_lock) == 0);
    ret = handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], START, wrapper_getNmeaData_cb, NULL);
    LOCATION_ASSERT(pthread_mutex_unlock(&nmea_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        LS_LOG_DEBUG("Error in getNmeaData");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
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
    LS_LOG_DEBUG("======getCurrentPosition=======");
    LSMessageRef(message);
    struct json_object *serviceObject;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    int accuracyLevel = 0;
    int responseTimeLevel = 0;
    unsigned char reqHandlerType = 0;
    TimerData *timerdata;
    Request *request = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool isGpsReqSucess = false, isCellIdReqSucess = false, isWifiReqSucess = false;
    bool mRetVal;
    //HandlerStatus handlerStatus;
    unsigned char handlerStatus = 0;
    /*Initialize all handlers status*/
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("serviceObject NULL Out of memory");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        LSMessageUnref(message);
        return true;
    }

    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_DEBUG("parsing error");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        return true;
    }

    //TODO check posistion data with maximum age in cache
    if(handler_Selection(sh, message, data, &accuracyLevel, &responseTimeLevel, &handlerStatus) == false) {
        LSMessageReplyError(sh, message, TRACKING_INVALID_INPUT,trakingErrorText[TRACKING_INVALID_INPUT]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        return true;
     }


    //check in cache first if maximumAge is -1
    mRetVal = json_object_object_get_ex(m_JsonArgument, "maximumAge", &m_JsonSubArgument);

    if (mRetVal == true) {
        int maximumAge;
      if(!json_object_is_type(m_JsonSubArgument,json_type_int)) {
        LSMessageReplyError(sh, message, TRACKING_INVALID_INPUT,trakingErrorText[TRACKING_INVALID_INPUT]);
        LSMessageUnref(message);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        return true;
      }

        maximumAge = json_object_get_int(m_JsonSubArgument);

        if (maximumAge != READ_FROM_CACHE) {
            LS_LOG_DEBUG("maximumAge= %d", maximumAge);
            mRetVal = readLocationfromCache(sh, message, serviceObject, maximumAge, accuracyLevel, handlerStatus);

            if (mRetVal == true) {
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
        LS_LOG_DEBUG("Both handler are OFF\n");
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);
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
    if ((handlerStatus & HANDLER_WIFI_BIT) && getHandlerStatus(NETWORK)) {
        LOCATION_ASSERT(pthread_mutex_lock(&pos_nw_lock) == 0);
        isWifiReqSucess = reqLocationToHandler(HANDLER_NW, &reqHandlerType, HANDLER_WIFI, sh);
        LOCATION_ASSERT(pthread_mutex_unlock(&pos_nw_lock) == 0);
    }

    //NETWORK HANDLER start and make request of type cellid

    /*Handler CELL-ID*/
    if ((handlerStatus & HANDLER_CELLID_BIT) && getHandlerStatus(NETWORK)) {
        LOCATION_ASSERT(pthread_mutex_lock(&pos_nw_lock) == 0);
        isCellIdReqSucess = reqLocationToHandler(HANDLER_NW, &reqHandlerType, HANDLER_CELLID, sh);
        LOCATION_ASSERT(pthread_mutex_unlock(&pos_nw_lock) == 0);
    }

    if ((isGpsReqSucess == false) && (isWifiReqSucess == false) && (isCellIdReqSucess == false)) {
        LS_LOG_DEBUG("No handlers started");
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);

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
        responseTime = ConvetResponseTimeFromLevel(accuracyLevel, responseTimeLevel);
        timeinSec = responseTime / 1000;
        timerdata = new(std::nothrow) TimerData(message, sh, reqHandlerType);

        if (timerdata == NULL) {
            LS_LOG_DEBUG("timerdata null Out of memory");
            LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);

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
            LS_LOG_DEBUG("req null Out of memory");
            LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);

            if (serviceObject != NULL)
               json_object_put(serviceObject);

            json_object_put(m_JsonArgument);
            LSMessageUnref(message);
            return true;
        }

        boost::shared_ptr<Request> req(request);
        m_reqlist.push_back(req);
        LS_LOG_DEBUG("message=%p sh=%p", message, sh);
        LS_LOG_DEBUG("Handler Started are gpsHandlerStarted=%d,wifiHandlerStarted=%d,cellidHandlerStarted=%d", isGpsReqSucess, isWifiReqSucess,
                isCellIdReqSucess);

        if (serviceObject != NULL)
           json_object_put(serviceObject);

        json_object_put(m_JsonArgument);
        return true;
    }
}
bool LocationService::reqLocationToHandler(int handler_type, unsigned char *reqHandlerType, int subHandlerType,
        LSHandle *sh)
{
    int ret;
    LS_LOG_DEBUG("reqLocationToHandler handler_type %d subHandlerType %d", handler_type, subHandlerType);
    ret = handler_start(HANDLER_INTERFACE(handler_array[handler_type]), subHandlerType);

    if (ret != ERROR_NONE) {
        LS_LOG_DEBUG("Failed to start handler errorcode %d", ret);
        return false;
    } else {
        //LS_LOG_DEBUG("%s handler started successfully",handlerNames[subHandlerType]);
        ret = handler_get_position(HANDLER_INTERFACE(handler_array[handler_type]), START, wrapper_getCurrentPosition_cb, NULL, subHandlerType, sh);

        if (ret == ERROR_NOT_AVAILABLE) {
            LS_LOG_DEBUG("getCurrentPosition failed errorcode %d", ret);
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
bool LocationService::readLocationfromCache(LSHandle *sh, LSMessage *message, json_object *serviceObject, int maxAge,
        int accuracylevel,
        unsigned char handlerstatus)
{
    LS_LOG_DEBUG("=======readLocationfromCache=======");
    bool gpsCacheSuccess = false;
    bool wifiCacheSuccess = false;
    bool cellCacheSucess = false;
    bool mRetVal;
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
            LS_LOG_DEBUG("No Cache data present");
            location_util_add_returnValue_json(serviceObject, false);
            location_util_add_errorCode_json(serviceObject, TRACKING_POS_NOT_AVAILABLE);
            location_util_add_errorText_json(serviceObject, trakingErrorText[TRACKING_POS_NOT_AVAILABLE]);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            return true;
        }

        //Read according to accuracy value
        location_util_add_pos_json(serviceObject, &pos);
        location_util_add_acc_json(serviceObject, &acc);

        if (pos.latitude != 0 && pos.longitude != 0) {
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            return true;
        } else
            return FALSE;
    } else {
        long long currentTime = ::time(0);
        currentTime *= 1000; // milli sec
        long long longmaxAge = maxAge * 1000;

        //More accurate data return first
        if (gpsCacheSuccess && (longmaxAge <= currentTime - gpspos.timestamp) && (gpsacc.horizAccuracy <= accuracy)) {
            location_util_add_pos_json(serviceObject, &gpspos);
            location_util_add_acc_json(serviceObject, &gpsacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            } else
                return true;
        } else if (wifiCacheSuccess && (longmaxAge <= currentTime - wifipos.timestamp) && (wifiacc.horizAccuracy <= accuracy)) {
            location_util_add_pos_json(serviceObject, &wifipos);
            location_util_add_acc_json(serviceObject, &wifiacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
            }
            else
                return true;
        } else if (cellCacheSucess && (longmaxAge <= currentTime - cellidpos.timestamp) && (cellidacc.horizAccuracy <= accuracy)) {
            location_util_add_pos_json(serviceObject, &cellidpos);
            location_util_add_acc_json(serviceObject, &cellidacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false) {
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
    LS_LOG_DEBUG("ret value getCachedDatafromHandler and Cached handler type=%d", ret);

    if (ret == ERROR_NONE)
       return true;
    else
       return false;
}
/**
 * <Funciton >   CheckSubsrcriptionListEmpty
 * <Description>  Checks the list is filled or not and then adds to Subscription list
 * @param     LunaService handle
 * @param     Subscription key
 * @param     message payload from luna
 * @param    LSError error message will get updated if any error
 * @param    ListFilled variable will be updated after checking the List with key filled or not, before added new request to list
 */
// bool LocationService::CheckSubsrcriptionListEmpty(LSHandle *sh,char* key,LSMessage *message,LSError* mLSError,int *ListFilled)
// {
// int mRetVal;
// LSSubscriptionIter* iter = NULL;
// mRetVal = LSSubscriptionAcquire(sh, key, &iter,
// mLSError);
// if(!mRetVal) return false;
// return LSSubscriptionHasNext(iter);
// }
/**
 * <Funciton >  handler_Selection
 * <Description>  Getting respective handler based on accuracy and power requirement
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    handler if successful else NULL
 */
bool LocationService::handler_Selection(LSHandle *sh, LSMessage *message, void *data, int *accuracy, int *responseTime,
                                        unsigned char *handlerStatus)
{
    LS_LOG_DEBUG("handler_Selection");
    bool mboolRespose = false;
    bool mboolAcc = false;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_DEBUG("m_JsonArgument Parsing Error");
        return false;
    }

    mboolAcc = json_object_object_get_ex(m_JsonArgument, "accuracy", &m_JsonSubArgument);

    if(mboolAcc && !json_object_is_type(m_JsonSubArgument,json_type_int)) {
    json_object_put(m_JsonArgument);
    return false;
    }

    if (mboolAcc)
       *accuracy = json_object_get_int(m_JsonSubArgument);

    mboolRespose = json_object_object_get_ex(m_JsonArgument, "responseTime", &m_JsonSubArgument);

    if(mboolRespose && !json_object_is_type(m_JsonSubArgument,json_type_int)) {
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
    LS_LOG_DEBUG("accuracy=%d responseTime=%d", *accuracy, *responseTime);

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
        LS_LOG_DEBUG("Default gps handler assigned\n");
        *handlerStatus |= HANDLER_GPS_BIT;   //GPS;
        //NEW REQUIREMENT to give position from any source quickly 25/10/2013[START]
        *handlerStatus |= HANDLER_CELLID_BIT;  //CELLID
        *handlerStatus |= HANDLER_WIFI_BIT;    //WIFI
        //NEW REQUIREMENT to give position from any source quickly 25/10/2013[END]
    }

    json_object_put(m_JsonArgument);
    return true;
}

int LocationService::ConvetResponseTimeFromLevel(int accLevel, int responseTimeLevel)
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
    LS_LOG_DEBUG("=======startTracking=======");
    //LSMessageRef(message); //Not required as the message is not stored
    unsigned char startedHandlers = 0;
    LSMessagePrint(message, stdout);
    int ret;
    LSError mLSError;

    //GPS and NW both are off
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_DEBUG("GPS & NW status is OFF");
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);
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

        if (1) { //wifistate == true)
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_WIFI);

            if (ret == ERROR_NONE)
                startedHandlers |= HANDLER_WIFI_BIT;
            else
                LS_LOG_DEBUG("WIFI handler not started");
        }

        LS_LOG_DEBUG("value of  handler not started %d %d", isInternetConnectionAvailable, isTelephonyAvailable);

        if (isTelephonyAvailable) {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_CELLID);

            if (ret == ERROR_NONE)
                startedHandlers |= HANDLER_CELLID_BIT;
            else
                LS_LOG_DEBUG("CELLID handler not started");
        }

        // Add to subsciption list with method+handler as key
    }

    if (startedHandlers == HANDLER_STATE_DISABLED) {
        LS_LOG_DEBUG("None of the handlers started");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    bool mRetVal;
    mRetVal = LSSubscriptionAdd(sh, SUBSC_START_TRACK_KEY, message, &mLSError);

    if (mRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR, trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    //is it only for GPS?
    if (startedHandlers & HANDLER_GPS_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_GPS], START, wrapper_startTracking_cb, NULL, HANDLER_GPS, sh);
        trackhandlerstate |= HANDLER_GPS_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_lock) == 0);
    }

    if (startedHandlers & HANDLER_WIFI_BIT) {
        //TODO get all AP List and pass to handler.
        //LSCall(sh,"palm://com.palm.wifi/findnetworks", "{\"subscribe\":true}", LocationService::GetWifiApsList_cb, this,NULL,&mLSError);
        LOCATION_ASSERT(pthread_mutex_lock(&track_wifi_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_WIFI, sh);
        trackhandlerstate |= HANDLER_WIFI_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_wifi_lock) == 0);
        //::startTracking((Handler*) service_agent_prop->wifi_handler, wrapper_startTracking_cb, service_agent_prop->ServiceObjRef);
    }

    if (startedHandlers & HANDLER_CELLID_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_cell_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_CELLID, sh);
        trackhandlerstate |= HANDLER_CELLID_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_cell_lock) == 0);
    }

    return true;
}
/**
 * <Function >   startTrackingBestPosition
 * <Description>  API to get repeated updates about location if subscribe is set to TRUE
 * @param     LunaService handle
 * @param     LunaService message
 * @param     user data
 * @return    bool if successful return true else false
 */
bool LocationService::startTrackingBestPosition(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("=======startTrackingBestPosition=======");
    //LSMessageRef(message); //Not required as the message is not stored
    unsigned char startedHandlers = 0;
    LSMessagePrint(message, stdout);
    int ret;
    LSError mLSError;

    //GPS and NW both are off
    if ((getHandlerStatus(GPS) == HANDLER_STATE_DISABLED) && (getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {
        LS_LOG_DEBUG("startTrackingBestPosition GPS status is OFF");
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);
        return true;
    }

    if (getHandlerStatus(GPS)) {
        LS_LOG_DEBUG("startTrackingBestPosition gps is on in Settings");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS);

        if (ret == ERROR_NONE)
         startedHandlers |= HANDLER_GPS_BIT;
        else
         LS_LOG_DEBUG("startTrackingBestPosition GPS handler not started");
    }

    if (getHandlerStatus(NETWORK)) {
        LS_LOG_DEBUG("startTrackingBestPosition network is on in Settings");

        if (1) { //wifistate == true)
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_WIFI);

            if (ret == ERROR_NONE)
               startedHandlers |= HANDLER_WIFI_BIT;
            else
                LS_LOG_DEBUG("startTrackingBestPosition WIFI handler not started");
        }

        LS_LOG_DEBUG("startTrackingBestPosition value of  handler not started %d %d", isInternetConnectionAvailable, isTelephonyAvailable);

        if (isTelephonyAvailable) {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_CELLID);

            if (ret == ERROR_NONE)
                startedHandlers |= HANDLER_CELLID_BIT;
            else
                LS_LOG_DEBUG("startTrackingBestPosition CELLID handler not started");
        }

        // Add to subsciption list with method+handler as key
    }

    if (startedHandlers == HANDLER_STATE_DISABLED) {
        LS_LOG_DEBUG("startTrackingBestPosition None of the handlers started");
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
        return true;
    }

    if (LSMessageIsSubscription(message)) {
        bool mRetVal;
        mRetVal = LSSubscriptionAdd(sh, SUBSC_BEST_START_TRACK_KEY, message, &mLSError);

        if (mRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
            return true;
        }

        }else {
        LS_LOG_DEBUG("startTrackingBestPosition Not a subscription call");
       //SUBSCRIPTION RESPONSE REMOVED ON 25/10/2013 AS APP DOES NEED IT[END]
        LSMessageReplyError(sh, message, TRACKING_INVALID_INPUT, trakingErrorText[TRACKING_INVALID_INPUT]);
        return true;
    }

    //is it only for GPS?
    if (startedHandlers & HANDLER_GPS_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_GPS], START, wrapper_startTracking_cb, NULL, HANDLER_GPS, sh);
        trackhandlerstate |= HANDLER_GPS_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_lock) == 0);
    }

    if (startedHandlers & HANDLER_WIFI_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_wifi_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_WIFI, sh);
        trackhandlerstate |= HANDLER_WIFI_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_wifi_lock) == 0);
    }

    if (startedHandlers & HANDLER_CELLID_BIT) {
        LOCATION_ASSERT(pthread_mutex_lock(&track_cell_lock) == 0);
        handler_start_tracking((Handler *) handler_array[HANDLER_NW], START, wrapper_startTracking_cb, NULL, HANDLER_CELLID, sh);
        trackhandlerstate |= HANDLER_CELLID_BIT;
        LOCATION_ASSERT(pthread_mutex_unlock(&track_cell_lock) == 0);
    }

    return true;
}
//Test code subscrption adding/removing
bool LocationService::test(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("[DEBUG] Test function\n");
}

bool LocationService::getReverseLocation(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    struct json_object *serviceObject;
    bool mRetVal;
    Position pos;
    Address address;
    LSError mLSError;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_DEBUG("m_JsonArgument parsing error");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        return true;
    }

    mRetVal = json_object_object_get_ex(m_JsonArgument, "latitude", &m_JsonSubArgument);

    if (mRetVal == true && json_object_is_type(m_JsonSubArgument,json_type_double)) {
        pos.latitude = json_object_get_double(m_JsonSubArgument);
    } else {
        LS_LOG_DEBUG("Input parameter is invalid, no latitude in request");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    mRetVal = json_object_object_get_ex(m_JsonArgument, "longitude", &m_JsonSubArgument);

    if (mRetVal == true &&  json_object_is_type(m_JsonSubArgument,json_type_double)) {
        pos.longitude = json_object_get_double(m_JsonSubArgument);
    } else {
        LS_LOG_DEBUG("Input parameter is invalid, no latitude in request");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    if (pos.latitude < -90.0 || pos.latitude > 90.0 || pos.longitude < -180.0 || pos.longitude > 180.0)
    {
        LS_LOG_DEBUG("Lat and long out of range");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }
    LOCATION_ASSERT(pthread_mutex_lock(&lbs_reverse_lock) == 0);

    //if (isInternetConnectionAvailable == true) {
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);
    //} else {
    //    LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
    //    json_object_put(m_JsonArgument);
    //    LSMessageReplyError(sh, message, GET_REV_LOCATION_INTERNET_CONNETION_NOT_AVAILABLE);
    //    return true;
    //}

    if (ret == ERROR_NONE) {
        /*Position pos;

         pos.latitude = 12.934722;
         pos.longitude = 77.690538;*/
        ret = handler_get_reverse_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &pos, &address);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);

        //LS_LOG_DEBUG("value of locality  %s region %s country %s country coede %s area %s street %s postcode %s",address.locality,address.region, address.country,
        //address.countrycode, address.area,address.street,address.postcode);
        if (ret != ERROR_NONE) {
            json_object_put(m_JsonArgument);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            return true;
        }

        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            return true;
        }

        json_object_object_add(serviceObject,"returnValue",json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        if (address.street != NULL)
            json_object_object_add(serviceObject, "street", json_object_new_string(address.street));
        else
            json_object_object_add(serviceObject, "street", json_object_new_string(""));

        if (address.countrycode != NULL)
            json_object_object_add(serviceObject, "countrycode",json_object_new_string(address.countrycode));
        else
            json_object_object_add(serviceObject, "countrycode", json_object_new_string(""));

        if (address.postcode != NULL)
            json_object_object_add(serviceObject, "postcode",json_object_new_string(address.postcode));
        else
            json_object_object_add(serviceObject, "postcode", json_object_new_string(""));

        if (address.area != NULL)
            json_object_object_add(serviceObject, "area", json_object_new_string(address.area));
        else
            json_object_object_add(serviceObject, "area", json_object_new_string(""));

        if (address.locality != NULL)
            json_object_object_add(serviceObject, "locality", json_object_new_string(address.locality));
        else
            json_object_object_add(serviceObject, "locality", json_object_new_string(""));

        if (address.region != NULL)
            json_object_object_add(serviceObject, "region", json_object_new_string(address.region));
        else
            json_object_object_add(serviceObject, "region", json_object_new_string(""));

        if (address.country != NULL)
            json_object_object_add(serviceObject, "country", json_object_new_string(address.country));
        else
            json_object_object_add(serviceObject, "country", json_object_new_string(""));

        mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        json_object_put(serviceObject);
    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_reverse_lock) == 0);
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

    json_object_put(m_JsonArgument);
    return true;
}
bool LocationService::getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    Address addr;
    Accuracy acc;
    Position pos;
    LSError mLSError;
    struct json_object *serviceObject = NULL;
    bool mRetVal;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LS_LOG_DEBUG("getGeoCodeLocation m_JsonArgument Memory allocation failed");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        return true;
    }

    mRetVal = json_object_object_get_ex(m_JsonArgument, "address", &m_JsonSubArgument);

    if (mRetVal == true) {
     if(!json_object_is_type(m_JsonSubArgument,json_type_string)) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
     }
        addr.freeformaddr = json_object_get_string(m_JsonSubArgument);
        addr.freeform = true;
    } else {
        LS_LOG_DEBUG("Not free form");
        addr.freeform = false;
        if(location_util_parsejsonAddress(m_JsonArgument, &addr) == false) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
        }

    }

    LOCATION_ASSERT(pthread_mutex_lock(&lbs_geocode_lock) == 0);
    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), HANDLER_LBS);

    if (ret == ERROR_NONE) {
        ret = handler_get_geo_code(HANDLER_INTERFACE(handler_array[HANDLER_LBS]), &addr, &pos, &acc);
        handler_stop(handler_array[HANDLER_LBS], HANDLER_LBS, false);
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);

        if (ret != ERROR_NONE) {
            json_object_put(m_JsonArgument);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            return true;
        }

        LS_LOG_DEBUG("value of lattitude %f longitude %f atitude %f", pos.latitude, pos.longitude, pos.altitude);
        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            json_object_put(m_JsonArgument);
            return true;
        }

        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject,LOCATION_SUCCESS);
        json_object_object_add(serviceObject, "latitude", json_object_new_double(pos.latitude));
        json_object_object_add(serviceObject, "longitude", json_object_new_double(pos.longitude));
        LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
        json_object_put(serviceObject);
    } else {
        LOCATION_ASSERT(pthread_mutex_unlock(&lbs_geocode_lock) == 0);
        LSMessageReplyError(sh, message, HANDLER_START_FAILURE,locationErrorText[HANDLER_START_FAILURE]);
    }

    json_object_put(m_JsonArgument);
    return true;
}
bool LocationService::getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data)
{
    struct json_object *handlersArrayItem;
    struct json_object *handlersArrayItem1;
    struct json_object * handlersArray;
    struct json_object *serviceObject;
    LSError mLSError;
    bool mhandler = false;
    bool mRetVal;

    handlersArrayItem = json_object_new_object();

    if (handlersArrayItem == NULL) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    handlersArrayItem1 = json_object_new_object();

    if (handlersArrayItem1 == NULL) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        json_object_put(handlersArrayItem);
        return true;
    }

    handlersArray = json_object_new_array();

    if (handlersArray == NULL) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        json_object_put(handlersArrayItem1);
        json_object_put(handlersArrayItem);
        return true;
    }

    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        json_object_put(handlersArrayItem);
        json_object_put(handlersArrayItem1);
        json_object_put(handlersArray);
        return true;
    }

    json_object_object_add(handlersArrayItem, "name", json_object_new_string(GPS));
    json_object_object_add(handlersArrayItem, "state", json_object_new_boolean(getHandlerStatus(GPS)));
    json_object_array_add(handlersArray, handlersArrayItem);
    json_object_object_add(handlersArrayItem1, "name", json_object_new_string(NETWORK));
    json_object_object_add(handlersArrayItem1, "state", json_object_new_boolean(getHandlerStatus(NETWORK)));
    json_object_array_add(handlersArray, handlersArrayItem1);

    /*Form the json object*/
    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
    json_object_object_add(serviceObject, "handlers", handlersArray);

    LS_LOG_DEBUG("Inside LSMessageReply");

    LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
    json_object_put(handlersArrayItem);
    json_object_put(handlersArrayItem1);
    json_object_put(handlersArray);
    json_object_put(serviceObject);
    return true;
}
bool LocationService::SetGpsStatus(LSHandle *sh, LSMessage *message, void *data)
{
    ::SetGpsStatus();
    return true;
}
bool LocationService::GetGpsStatus(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("=======Subscribe for GetGpsStatus=======");
    LSError mLSError;
    LSErrorInit(&mLSError);
    int ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS);

    if (ret != ERROR_NONE) {
        LS_LOG_DEBUG("GPS handler start failed");
        LSMessageReplyError(sh, message, LOCATION_HANDLER_NOT_AVAILABLE,locationErrorText[LOCATION_HANDLER_NOT_AVAILABLE]);
    }

    if (LSMessageIsSubscription(message)) {
        bool mRetVal;
        mRetVal = LSSubscriptionAdd(sh, SUBSC_GPS_ENGINE_STATUS, message, &mLSError);

        if (mRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            return true;
        }
    } else {
        LS_LOG_DEBUG("Not subscription call");
        struct json_object *serviceObject;
        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
            return true;
        }
        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_boolean(mCachedGpsEngineStatus));
        bool mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
        if(mRetVal == false)
           LSErrorPrintAndFree(&mLSError);

        json_object_put(serviceObject);
        return true;
    }

    // TODO Reply need to be sent?
    //mRetVal = LSMessageReplySubscriptionSuccess(sh, message);
    handler_get_gps_status((Handler *)handler_array[HANDLER_GPS], wrapper_gpsStatus_cb);
    return true;
}
bool LocationService::getState(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("=======getState=======");
    int state;
    bool mRetVal;
    LPAppHandle lpHandle = 0;
    char *handler;
    LSError mLSError;
    LSErrorInit(&mLSError);
    struct json_object *serviceObject;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    mRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);

    if (mRetVal == false || (!json_object_is_type(m_JsonSubArgument,json_type_string))) {
        LS_LOG_DEBUG("handler key not present in  message");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    handler = json_object_get_string(m_JsonSubArgument);

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        int lpret;
        lpret = LPAppCopyValueInt(lpHandle, handler, &state);
        LPAppFreeHandle(lpHandle, true);

        if (lpret != LP_ERR_NONE) {
            LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
            json_object_put(m_JsonArgument);
            return true;
        }

        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LS_LOG_DEBUG("serviceObject NULL Out of memory");
            LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
            json_object_put(m_JsonArgument);
            return true;
        }
        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_int(state));

        LS_LOG_DEBUG("state=%d", state);
        LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
        json_object_put(serviceObject);
        json_object_put(m_JsonArgument);
        return true;
    } else {
        LS_LOG_DEBUG("LPAppGetHandle is not created ");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
        json_object_put(m_JsonArgument);
        return true;
    }
}
bool LocationService::setState(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("=======setState=======");
    char *handler;
    bool state;
    bool mRetVal;
    LSError mLSError;
    LSErrorInit(&mLSError);
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    struct json_object *serviceObject;
    LPAppHandle lpHandle = 0;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        return true;
    }

    //Read Handler from json
    mRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);

    if (mRetVal)
        handler = json_object_get_string(m_JsonSubArgument);
    else {
        LS_LOG_DEBUG("Invalid param:handler");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    //Read state from json
    mRetVal = json_object_object_get_ex(m_JsonArgument, "state", &m_JsonSubArgument);

    if (mRetVal && json_object_is_type(m_JsonSubArgument,json_type_boolean) && ( (strcmp(handler, GPS) == 0) ||  (strcmp(handler, NETWORK)) == 0))
     state = json_object_get_boolean(m_JsonSubArgument);
    else {
        LS_LOG_DEBUG("Invalid param:state");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        LS_LOG_DEBUG("Writing handler state");
        serviceObject = json_object_new_object();

        if (serviceObject == NULL) {
            LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
            json_object_put(m_JsonArgument);
            return true;
        }

        LPAppSetValueInt(lpHandle, handler, state);
        LPAppFreeHandle(lpHandle, true);
        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

        if (strcmp(handler, GPS) == 0) {
            mGpsStatus = state;

            if (state == false)
                handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, true);
        }

        if (strcmp(handler, NETWORK) == 0) {
            mNwStatus = state;

            if (state == false) {
                handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, true);
                handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, true);
            }
        }

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        json_object_put(serviceObject);
    } else {
        LS_LOG_DEBUG("LPAppGetHandle is not created");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

    json_object_put(m_JsonArgument);
    return true;
}
bool LocationService::sendExtraCommand(LSHandle *sh, LSMessage *message, void *data)
{
    LS_LOG_DEBUG("=======sendExtraCommand=======");
    struct json_object *json_message;
    struct json_object *json_cmd;
    struct json_object *serviceObject;
    char *cmdStr = NULL;
    bool mRetVal;
    int ret;
    LSError mLSError;
    LSErrorInit(&mLSError);
    json_message = json_tokener_parse((const char *)LSMessageGetPayload(message));

    if (json_message == NULL || is_error(json_message)) {
        LS_LOG_DEBUG("SEND_EXTRA_OUT_OF_MEM");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    mRetVal = json_object_object_get_ex(json_message, "command", &json_cmd);

    if (mRetVal == false ||( !json_object_is_type(json_cmd,json_type_string))) {
        LS_LOG_DEBUG("LOCATION_INVALID_INPUT");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(json_message);
        return true;
    }

    ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

    if (ret != ERROR_NONE) {
        LS_LOG_DEBUG("GPS Handler starting failed");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
        json_object_put(json_message);
        return true;
    }

    cmdStr = json_object_get_string(json_cmd);
    LS_LOG_DEBUG("cmdStr = %s", cmdStr);

    if (strcmp("delete_aiding_data", cmdStr) == 0 || strcmp("force_time_injection", cmdStr) == 0 || strcmp("force_xtra_injection", cmdStr) == 0) {
        LS_LOG_DEBUG("%s", cmdStr);
        int err = handler_send_extra_command(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), cmdStr);

        if (err == ERROR_NOT_AVAILABLE) {
            LS_LOG_DEBUG("Error in %s", cmdStr);
            LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
            handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
            json_object_put(json_message);
            return true;
        }
    }
    else {
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(json_message);
        return true;
    }

    handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM, locationErrorText[LOCATION_OUT_OF_MEM]);
        json_object_put(json_message);
        return true;
    }
    json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
    json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
    mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
    if (mRetVal == false)
        LSErrorPrintAndFree (&mLSError);
    json_object_put(json_message);
    json_object_put(serviceObject);
    return true;
}
bool LocationService::getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data)
{
    char *handler = NULL;
    struct json_object *serviceObject;
    int accuracy = 0, powerRequirement = 0;
    bool mRequiresNetwork = false;
    bool mRequiresCell = false;
    bool mCost = false;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool mRetVal;
    struct json_object *m_JsonArgument;
    struct json_object *m_JsonSubArgument;
    m_JsonArgument = json_tokener_parse((const char *) LSMessageGetPayload(message));

    if (m_JsonArgument == NULL || is_error(m_JsonArgument)) {
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
        return true;
    }

    mRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);

    if (mRetVal && json_object_is_type(m_JsonSubArgument, json_type_string))
        handler = json_object_get_string(m_JsonSubArgument);
    else {
        LS_LOG_DEBUG("Invalid param:Handler");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT,locationErrorText[LOCATION_INVALID_INPUT]);
        json_object_put(m_JsonArgument);
        return true;
    }

    if (NULL == handler) {
        LS_LOG_DEBUG("ParamInput is NULL");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
        json_object_put(m_JsonArgument);
        return true;
    }

    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh, message, LOCATION_OUT_OF_MEM,locationErrorText[LOCATION_OUT_OF_MEM]);
        json_object_put(m_JsonArgument);
        return true;
    }

    if (strcmp(handler, "gps") == 0) {
        accuracy = 1;
        powerRequirement = 1;
        mRequiresNetwork = false;
        mRequiresCell = false;
        mCost = false;
        LS_LOG_DEBUG("getHandlerStatus(GPS)");

        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "accuracy", json_object_new_int(accuracy));
        json_object_object_add(serviceObject, "powerRequirement", json_object_new_int(powerRequirement));
        json_object_object_add(serviceObject, "requiresNetwork", json_object_new_boolean(mRequiresNetwork));
        json_object_object_add(serviceObject, "requiresCell", json_object_new_boolean(mRequiresCell));
        json_object_object_add(serviceObject, "monetaryCost", json_object_new_boolean(mCost));

        mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else if (strcmp(handler, "network") == 0) {
        printf("Inside else");
        accuracy = 3;
        powerRequirement = 3;
        mRequiresNetwork = true;
        mRequiresCell = true;
        mCost = true;

        json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "accuracy", json_object_new_int(accuracy));
        json_object_object_add(serviceObject, "powerRequirement", json_object_new_int(powerRequirement));
        json_object_object_add(serviceObject, "requiresNetwork", json_object_new_boolean(mRequiresNetwork));
        json_object_object_add(serviceObject, "requiresCell", json_object_new_boolean(mRequiresCell));
        json_object_object_add(serviceObject, "monetaryCost", json_object_new_boolean(mCost));

        mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else {
        LS_LOG_DEBUG("LPAppGetHandle is not created");
        LSMessageReplyError(sh, message, LOCATION_UNKNOWN_ERROR,locationErrorText[LOCATION_UNKNOWN_ERROR]);
    }

    json_object_put(m_JsonArgument);
    json_object_put(serviceObject);
    return true;
}
bool LocationService::getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data)
{
    int ret;
    bool mRetVal;
    LSError mLSError;
    LSErrorInit(&mLSError);

    if (getHandlerStatus(GPS)) {
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

        if (ret != ERROR_NONE) {
            LS_LOG_DEBUG("GPS Handler not started");
            LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
            return true;
        }
    } else {
        LS_LOG_DEBUG("getGpsSatelliteData GPS OFF");
        LSMessageReplyError(sh, message, TRACKING_LOCATION_OFF,trakingErrorText[TRACKING_LOCATION_OFF]);
        return true;
    }

    mRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GPS_SATELLITE_DATA, message, &mLSError);

    if (mRetVal == false) {
        LSErrorPrintAndFree(&mLSError);
        return true;
    }

    LOCATION_ASSERT(pthread_mutex_lock(&sat_lock) == 0);
    ret = handler_get_gps_satellite_data(handler_array[HANDLER_GPS], START, wrapper_getGpsSatelliteData_cb);
    LOCATION_ASSERT(pthread_mutex_unlock(&sat_lock) == 0);

    if (ret != ERROR_NONE && ret != ERROR_DUPLICATE_REQUEST) {
        LSMessageReplyError(sh, message, TRACKING_UNKNOWN_ERROR,trakingErrorText[TRACKING_UNKNOWN_ERROR]);
    }

    return true;
}
bool LocationService::getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data)
{
    bool mRetVal ;
    long long mTTFF = handler_get_time_to_first_fix((Handler *) handler_array[HANDLER_GPS]);
    LSError mLSError;
    LSErrorInit(&mLSError);
    char str[MAX_LIMIT];
    sprintf(str, "%lld", mTTFF);
    LS_LOG_DEBUG("get time to first fix %lld\n", mTTFF);
    struct json_object *serviceObject;
    serviceObject = json_object_new_object();

    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, LOCATION_SUCCESS);
    json_object_object_add(serviceObject, "TTFF", json_object_new_string(str));

    mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
    if (mRetVal == false) {
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
void wrapper_startTracking_cb(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error, gpointer privateIns,
                              int type)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_startTracking_cb");
    LocationService::start_tracking_cb(pos, accuracy, error, privateIns, type);
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
void wrapper_getCurrentPosition_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int handlerType)
{
    LS_LOG_DEBUG("wrapper_getCurrentPosition_cb called back from handlerType=%d", handlerType);
    LocationService::getCurrentPosition_cb(position, accuracy, error, privateIns, handlerType);
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
void wrapper_getNmeaData_cb(gboolean enable_cb, int timestamp, char *nmea, int length, gpointer privateIns)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_getNmeaData_cb called back ...");
    LocationService::get_nmea_cb(enable_cb, timestamp, nmea, length, privateIns);
}

void getReverseLocation_cb(Address *address, Accuracy *accuracy, int error, gpointer data)
{
    LS_LOG_DEBUG(
        "value of locality  %s region %s country %s country coede %s area %s street %s postcode %s", address->locality,
        address->region, address->country, address->countrycode, address->area, address->street, address->postcode);
}
void getGeoCodeLocation_cb(Position *position, Accuracy *accuracy, int error, gpointer data)
{
    LS_LOG_DEBUG("value of lattitude %f longitude %f atitude %f", position->latitude, position->longitude,
                 position->altitude);
}
void wrapper_getGpsSatelliteData_cb(gboolean enable_cb, Satellite *satellite, gpointer privateIns)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_getGpsSatelliteData_cb called back ...");
    LocationService::getGpsSatelliteData_cb(enable_cb, satellite, privateIns);
}
void sendExtraCommand_cb(gboolean enable_cb, int command, gpointer privateIns)
{
}
void wrapper_gpsStatus_cb(gboolean enable_cb, int state, gpointer data)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_gpsStatus_cb called back ...");
    LocationService::getGpsStatus_cb(state);
}
/********************************Response to Application layer*********************************************************************/

void LocationService::startTrackingBestPosition_reply(struct json_object *serviceObject, Accuracy *accuracy)
{
    LS_LOG_DEBUG("==========startTrackingBestPosition_reply called============");
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool mRetVal;
        if (this->mIsStartFirstReply || accuracy->horizAccuracy < MINIMAL_ACCURACY) {
            this->mlastTrackingReplyTime = time(0);
            this->mIsStartFirstReply = false;
        mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_BEST_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
               LSErrorPrintAndFree(&mLSError);

        mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_BEST_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
               LSErrorPrintAndFree(&mLSError);
        } else {
            long long current_time = time(0);

        if (current_time - this->mlastTrackingReplyTime > 60) {
            mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_BEST_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_BEST_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);

                mlastTrackingReplyTime = time(0);
            }
        }
}
void LocationService::startTracking_reply(Position *pos, Accuracy *accuracy, int error, int type)
{
    LS_LOG_DEBUG("==========startTracking_reply called============");
    struct json_object *serviceObject;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool mRetVal;

    serviceObject = json_object_new_object();
    if (serviceObject == NULL) {
        LS_LOG_DEBUG("No memory");
        return;
    }

    if (error == ERROR_NONE) {
        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject, SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);
        mRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        mRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject),
                &mLSError);
        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

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
            location_util_add_errorCode_json(serviceObject, TRACKING_TIMEOUT);
            location_util_add_errorText_json(serviceObject, trakingErrorText[TRACKING_TIMEOUT]);

            mRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            mRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);

            if (mRetVal == false)
                LSErrorPrintAndFree(&mLSError);
        }
    }

    json_object_put(serviceObject);
}

void LocationService::get_nmea_reply(int timestamp, char *data, int length)
{
    LS_LOG_DEBUG("[DEBUG] get_nmea_reply called");
    bool mRetVal;
    struct json_object *serviceObject;
    LSError mLSError;
    LSErrorInit(&mLSError);
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("Out of memory");
        return;
    }

    LS_LOG_DEBUG(" timestamp=%d\n", timestamp);
    LS_LOG_DEBUG("data=%s\n", data);
    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, SUCCESS);
    json_object_object_add(serviceObject, "timestamp", json_object_new_int(timestamp));

    if (data != NULL)
        json_object_object_add(serviceObject, "nmea", json_object_new_string(data));

    LS_LOG_DEBUG("length=%d\n", length);
    LS_LOG_DEBUG("mServiceHandle=%x\n", mServiceHandle);
    mRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle, SUBSC_GPS_GET_NMEA_KEY, json_object_to_json_string(serviceObject), &mLSError);

    if (mRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    mRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle, SUBSC_GPS_GET_NMEA_KEY, json_object_to_json_string(serviceObject), &mLSError);

    if (mRetVal == false)
        LSErrorPrintAndFree(&mLSError);
    json_object_put(serviceObject);    //freeing the memory
}

void LocationService::getCurrentPosition_reply(Position *pos, Accuracy *accuracy, int error, int handlerType)
{
    LS_LOG_DEBUG("======getCurrentPosition_reply called=======");
    int count = 0;
    struct json_object *serviceObject;
    serviceObject = json_object_new_object();

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("Out of memory");
        return;
    }

    LSError mLSError;
    LSErrorInit(&mLSError);
    TimerData *timerdata;
#if 0
    testjson(serviceObject);
#else

    if (error == ERROR_NONE) {
        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject, SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);
    } else {
        LS_LOG_DEBUG("error = %d ERROR_NONE =%d", error, ERROR_NONE);
        location_util_add_returnValue_json(serviceObject, false);
        location_util_add_errorCode_json(serviceObject, TRACKING_POS_NOT_AVAILABLE);
        location_util_add_errorText_json(serviceObject,trakingErrorText[TRACKING_POS_NOT_AVAILABLE]);
    }

#endif
    /*List Implementation*/
    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();

    if (size <= 0) {
        LS_LOG_DEBUG("List is empty");
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
                LS_LOG_DEBUG("Resetting bit handlerType %d after reset it->get()->GetReqHandlerType() = %d", handlerType,
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
    bool mRetVal;
    char *resString;
    LSError mLSError;
    LSErrorInit(&mLSError);
    /*Creating a json object*/
    struct json_object *serviceObject = json_object_new_object();

    if (sat == NULL) {
        LS_LOG_DEBUG("satallite data NULL");
        return;
    }

    if (serviceObject == NULL) {
        LS_LOG_DEBUG("serviceObject NULL Out of memory");
        return;
    }

    while (num_satellite_used_count < sat->visible_satellites_count) {

        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject,LOCATION_SUCCESS);

        LS_LOG_DEBUG(" Service Agent value of %llf ", sat->sat_used[num_satellite_used_count].azimuth);
        json_object_object_add(serviceObject, "azimuth", json_object_new_double(sat->sat_used[num_satellite_used_count].azimuth));
        json_object_object_add(serviceObject, "elevation", json_object_new_double(sat->sat_used[num_satellite_used_count].elevation));
        json_object_object_add(serviceObject, "prn", json_object_new_int(sat->sat_used[num_satellite_used_count].prn));
        LS_LOG_DEBUG(" Service Agent value of %llf", sat->sat_used[num_satellite_used_count].snr);
        json_object_object_add(serviceObject, "snr", json_object_new_double(sat->sat_used[num_satellite_used_count].snr));
        json_object_object_add(serviceObject, "hasAlmanac", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasalmanac));
        json_object_object_add(serviceObject, "hasEphemeris", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasephemeris));
        json_object_object_add(serviceObject, "usedInFix", json_object_new_boolean(sat->sat_used[num_satellite_used_count].used));
        json_object_object_add(serviceObject, "visibleSatellites", json_object_new_int(sat->visible_satellites_count));
        num_satellite_used_count++;
        resString = json_object_to_json_string(serviceObject);
        mRetVal = LSSubscriptionNonSubscriptionRespond(mServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        mRetVal = LSSubscriptionNonSubscriptionRespond(mlgeServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);
    }
    json_object_put(serviceObject);
    LS_LOG_DEBUG("[DEBUG] mRetVal=%d\n", mRetVal);
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
    struct json_object *serviceObject = json_object_new_object();
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool isGpsEngineOn;
    int count = 0;

    if (state == GPS_STATUS_AVAILABLE)
        isGpsEngineOn = true;
    else
        isGpsEngineOn = false;

    mCachedGpsEngineStatus = isGpsEngineOn;

    if (serviceObject != NULL) {
        json_object_object_add(serviceObject,"returnValue",json_object_new_boolean(true));
        json_object_object_add(serviceObject, "errorCode", json_object_new_int(LOCATION_SUCCESS));
        json_object_object_add(serviceObject, "state", json_object_new_boolean(isGpsEngineOn));
        bool mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_GPS_ENGINE_STATUS, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_GPS_ENGINE_STATUS, json_object_to_json_string(serviceObject), &mLSError);

        if (mRetVal == false)
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
    LSErrorInit(&mLSError);
    bool mRetVal;
    char *key = LSMessageGetMethod(message);
    LS_LOG_DEBUG("LSMessageGetMethod(message) %s", key);
    LSSubscriptionIter *iter = NULL;

    if (key == NULL) {
        LS_LOG_DEBUG("Not a valid key");
        return true;
    }

    mRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

    if (mRetVal == false) {
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
    return true;
}
bool LocationService::getHandlerStatus(const char *handler)
{
    LS_LOG_DEBUG("getHandlerStatus handler %s", handler);

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
    LS_LOG_DEBUG("getHandlerStatus ");

    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE) {
        int lpret;
        lpret = LPAppCopyValueInt(lpHandle, handler, &state);
        LS_LOG_DEBUG("state=%d lpret = %d\n", state, lpret);
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
        handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        //STOP CELLID
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
        //STOP WIFI
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        this->mIsStartFirstReply = true;
        mIsStartTrackProgress = false;
    } else if (strcmp(key, SUBSC_BEST_START_TRACK_KEY) == 0) {
        //STOP GPS
        handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        //STOP CELLID
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
        //STOP WIFI
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
        this->mIsStartFirstReply = true;
    } else if (strcmp(key, SUBSC_GPS_GET_NMEA_KEY) == 0) {
        handler_get_nmea_data((Handler *) handler_array[HANDLER_GPS], STOP, wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetNmeaSubProgress = false;
    } else if (strcmp(key, SUBSC_GET_GPS_SATELLITE_DATA) == 0) {
        handler_get_gps_satellite_data(handler_array[HANDLER_GPS], STOP, wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        mIsGetSatSubProgress = false;
    }
}

void LocationService::stopNonSubcription(const char *key) {
    if (strcmp(key, SUBSC_START_TRACK_KEY) == 0 || strcmp(key, SUBSC_BEST_START_TRACK_KEY) == 0) {
        //STOP GPS
        handler_start_tracking(handler_array[HANDLER_GPS], STOP, wrapper_startTracking_cb, NULL, HANDLER_GPS, NULL);
        handler_stop(handler_array[HANDLER_GPS], HANDLER_GPS, false);
        //STOP CELLID
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_CELLID, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_CELLID, false);
        //STOP WIFI
        handler_start_tracking(handler_array[HANDLER_NW], STOP, wrapper_startTracking_cb, NULL, HANDLER_WIFI, NULL);
        handler_stop(handler_array[HANDLER_NW], HANDLER_WIFI, false);
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
        LS_LOG_DEBUG("TimerCallback timerdata is null");
        return false;
    }

    LSMessage *message = timerdata->GetMessage();
    LSHandle *sh = timerdata->GetHandle();
    unsigned char handlerType = timerdata->GetHandlerType();

    if (message == NULL || sh == NULL) {
        LS_LOG_DEBUG("message or Handle is null");
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
    LSErrorInit(&mLSError);
    TimerData *timerdata;

    if (size <= 0) {
        LS_LOG_DEBUG("List is empty");
        return false;
    }

    for (it = m_reqlist.begin(); count < size; ++it, count++) {
        if (it->get()->GetMessage() == message) {
            LS_LOG_DEBUG("Timeout Message is found in the list");

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

            LSMessageReplyError(sh, message, TRACKING_TIMEOUT,trakingErrorText[TRACKING_TIMEOUT]);
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
bool LocationService::isSubscListEmpty(LSHandle *sh, const char *key)
{
    LSError mLSError;
    LSErrorInit(&mLSError);
    LSSubscriptionIter *iter = NULL;
    bool mRetVal;
    mRetVal = LSSubscriptionAcquire(sh, key, &iter, &mLSError);

    if (mRetVal == false) {
        LSErrorPrint(&mLSError, stderr);
        LSErrorFree(&mLSError);
        return mRetVal;
    }

    mRetVal = LSSubscriptionHasNext(iter);
    LSSubscriptionRelease(iter);
    return mRetVal;
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
    if (isNonSubscibePresent && (isSubscListEmpty(sh, key) == false))
        stopNonSubcription(key);

    return retVal;
}

