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
#include <Wifi_Handler.h>
#include <Cell_Handler.h>
#include <Network_Handler.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>
#include <JsonUtility.h>
#include <db_util.h>
#include <libxml/parser.h>
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
LSMethod LocationService::rootMethod[] =
{
    {"getNmeaData", LocationService::_getNmeaData },
    {"getCurrentPosition", LocationService::_getCurrentPosition },
    {"startTracking", LocationService::_startTracking },
    {"getReverseLocation" ,LocationService::_getReverseLocation},
    {"getGeoCodeLocation" ,LocationService::_getGeoCodeLocation},
    {"getAllLocationHandlers" ,LocationService::_getAllLocationHandlers},
    {"SetGpsStatus" ,LocationService::_SetGpsStatus},
    {"GetGpsStatus" ,LocationService::_GetGpsStatus},
    {"setState" ,LocationService::_setState},
    {"getState" ,LocationService::_getState},
    {"sendExtraCommand" ,LocationService::_sendExtraCommand},
    {"getLocationHandlerDetails" ,LocationService::_getLocationHandlerDetails},
    {"getGpsSatelliteData" ,LocationService::_getGpsSatelliteData},
    {"getTimeToFirstFix" ,LocationService::_getTimeToFirstFix},
    {"test",LocationService::_test},
    {0,0}

};

LocationService* LocationService::locService = NULL;
GMainLoop* LocationService::mainLoop = NULL;
int LocationService::method = METHOD_NONE;

LocationService* LocationService::getInstance(GMainLoop *mainloop)
{
    if(locService == NULL)
    {
        locService = new (std::nothrow) LocationService(mainloop);
        return locService;
    }
    else
    {
        return locService;
    }

}

void LocationService::setGmainloop(GMainLoop *mainloop)
{
    LocationService::mainLoop = mainloop;
}

    LocationService::LocationService(GMainLoop *mainLoop)
: mServiceHandle(0),mlgeServiceHandle(0), m_JsonArgument(0), m_JsonSubArgument(0)
{
    LS_LOG_DEBUG("LocationService object created");
}
void LocationService::LSErrorPrintAndFree(LSError *ptrLSError)
{
    LSErrorPrint(ptrLSError, stderr);
    LSErrorFree(ptrLSError);
}
bool LocationService::init()
{
    LSError mLSError;
    LSErrorInit(&mLSError);
    mRetVal = LSRegisterPalmService(LOCATION_SERVICE_NAME, &mServiceHandle, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }


    mRetVal = LSGmainAttachPalmService(mServiceHandle, mainLoop, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    // add root category
    mRetVal = LSPalmServiceRegisterCategory(mServiceHandle, "/", rootMethod, NULL, NULL,this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPublicConnection(mServiceHandle), LocationService::_cancelSubscription, this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPrivateConnection(mServiceHandle), LocationService::_cancelSubscription, this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    /*Service name com.lge.location */

    mRetVal = LSRegisterPalmService(LOCATION_SERVICE_ALIAS_LGE_NAME, &mlgeServiceHandle, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    mRetVal = LSGmainAttachPalmService(mlgeServiceHandle, mainLoop, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    // add root category
    mRetVal = LSPalmServiceRegisterCategory(mlgeServiceHandle, "/", rootMethod, NULL, NULL,this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPublicConnection(mlgeServiceHandle), LocationService::_cancelSubscription, this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    mRetVal = LSSubscriptionSetCancelFunction(LSPalmServiceGetPrivateConnection(mlgeServiceHandle), LocationService::_cancelSubscription, this, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }

    g_type_init ();
    LS_LOG_DEBUG("[DEBUG] %s::%s( %d ) init ,g_type_init called \n",__FILE__,__PRETTY_FUNCTION__,__LINE__);



    //load all handlers
    GObject *gps_handler = static_cast<GObject*>(g_object_new(HANDLER_TYPE_GPS, NULL));
    GObject *nw_handler = static_cast<GObject*>(g_object_new(HANDLER_TYPE_NW, NULL));

    if((gps_handler == NULL) || (nw_handler == NULL))
        return false;
    handler_array[HANDLER_GPS] = (Handler*)gps_handler;
    handler_array[HANDLER_NW] = (Handler*)nw_handler;


    LS_LOG_DEBUG("Successfully LocationService object initialized");
    return true;
}

LocationService::~LocationService()
{
    // Reviewed by Sathya & Abhishek
    // If only GetCurrentPosition is called and if service is killed then we need to free the memory allocated for handler.

    bool ret;

    if (!m_reqlist.empty()) {
        m_reqlist.clear();
    }
    if(handler_array[HANDLER_GPS])
        g_object_unref(handler_array[HANDLER_GPS]);
    if(handler_array[HANDLER_NW])
        g_object_unref(handler_array[HANDLER_NW]);
    LSError m_LSErr;
    LSErrorInit(&m_LSErr);

    ret = LSUnregisterPalmService(mServiceHandle, &m_LSErr);
    if(ret == false)
    {
        LSErrorPrint (&m_LSErr, stderr);
        LSErrorFree(&m_LSErr);
    }

    ret = LSUnregisterPalmService(mlgeServiceHandle, &m_LSErr);
    if(ret == false)
    {
        LSErrorPrint (&m_LSErr, stderr);
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
    const char* subscribe_answer;
    LSSubscriptionIter* iter = NULL;
    char* errorCode;
    bool ListFilled = FALSE;
    int ret;
    LSError mLSError;
    //LSMessageRef(message); //Not required as the message is not stored
    LSMessagePrint(message, stdout);
    if(getHandlerStatus(GPS) == false)
    {
        LSMessageReplyError(sh,message,TRACKING_LOCATION_OFF);
        return true;
    }
    if(handler_array[HANDLER_GPS] == NULL)
    {
        LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        return true;
    }
    else
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);

    if(ret)
    {
        LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        return true;
    }
    // Add to subsciption list with method+handler as key
    if (LSMessageIsSubscription(message))
    {
        ListFilled = isSubscListEmpty(sh,SUBSC_GPS_GET_NMEA_KEY);
        LS_LOG_DEBUG("ListFilled=%d",ListFilled);
        mRetVal = LSSubscriptionAdd(sh, SUBSC_GPS_GET_NMEA_KEY, message, &mLSError);
        if (mRetVal == false)
        {
            LS_LOG_DEBUG("Failed to add to subscription list");
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
            return true;
        }
        LSMessageReplySubscriptionSuccess(sh,message);
    }

    else
    {
        LS_LOG_DEBUG("Not a subscription call");
        LSMessageReplyError(sh,message,TRACKING_NOT_SUBSC_CALL);
        return true;
    }

    if (!ListFilled)
    {
        LS_LOG_DEBUG("Call getNmeaData handler");
        /*::getNmeaData((Handler*) handler_array[HANDLER_GPS],
          wrapper_getNmeaData_cb, service_agent_prop->data);*/
        ret = ::getNmeaData((Handler*) handler_array[HANDLER_GPS],START,wrapper_getNmeaData_cb,NULL);
        if(ret!= ERROR_NONE)
        {
            LS_LOG_DEBUG("Error in getNmeaData");
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        }


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

    int count = 0;
    int ret;
    int accuracyLevel = 0;
    int responseTimeLevel = 0;
    int timeinSec;
    int  maximumAge;
    int responseTime;
    unsigned char handlerType=0;
    guint timerID = 0;
    LSSubscriptionIter* iter = NULL;
    TimerData *timerdata;
    Request *request = NULL;
    LSError mLSError;
    bool gpsHandlerStarted = FALSE , cellidHandlerStarted = FALSE ,wifiHandlerStarted = FALSE;
    HandlerStatus handlerStatus;
    /*Initialize all handlers status*/
    handlerStatus.gps = 0;
    handlerStatus.cellid = 0;
    handlerStatus.wifi = 0;

    serviceObject = json_object_new_object();
    if(serviceObject == NULL)
    {
        LS_LOG_DEBUG("serviceObject NULL Out of memory");
        LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        return true;
    }

    m_JsonArgument = json_tokener_parse((const char*)LSMessageGetPayload(message));

    //TODO check posistion data with maximum age in cache
    handler_Selection(sh,message,data,&accuracyLevel,&responseTimeLevel,&handlerStatus);

    //check in cache first if maximumAge is -1
    mRetVal = json_object_object_get_ex(m_JsonArgument, "maximumAge", &m_JsonSubArgument);
    if (mRetVal == true)
    {
        maximumAge = json_object_get_int(m_JsonSubArgument);
        if(maximumAge != 0)
        {
            ret = readLocationfromCache(sh,message,serviceObject,maximumAge,handlerStatus);
            if(ret == TRUE)
            {
                if(serviceObject!= NULL)
                    json_object_put(serviceObject);
                LSMessageUnref(message);
                return true;
            }
            //failed to get data from cache continue
        }
    }

    //Before selecting Handler check all handlers disabled from settings then throw LocationServiceOFF error
    if(!getHandlerStatus(GPS) && !getHandlerStatus(NETWORK))
    {
        LS_LOG_DEBUG("Both handler are OFF\n");
        LSMessageReplyError(sh,message,TRACKING_LOCATION_OFF);
        if(serviceObject!= NULL)
            json_object_put(serviceObject);
        LSMessageUnref(message);
        return true;
    }




    /*Handler GPS*/
    if(handlerStatus.gps == HANDLER_STATE_ENABLED)
    {
        if(getHandlerStatus(GPS))
        {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),HANDLER_GPS);
            if(ret)
            {
                LS_LOG_DEBUG("Failed to start GPS handler errorcode= %d", ret);
                gpsHandlerStarted = FALSE;
            }
            else
            {
                LS_LOG_DEBUG("GPS handler started successfully");

                ret = ::getCurrentPosition(handler_array[HANDLER_GPS], START, wrapper_getCurrentPosition_cb,HANDLER_GPS,sh);
                if(ret == ERROR_NOT_AVAILABLE)
                {
                    LS_LOG_DEBUG("getCurrentPosition failed errorcode %d",ret);
                    ::getCurrentPosition(handler_array[HANDLER_GPS], STOP, wrapper_getCurrentPosition_cb,HANDLER_GPS,sh);
                    handler_stop(handler_array[HANDLER_GPS],HANDLER_GPS);
                }
                else
                {
                    gpsHandlerStarted = TRUE;
                    handlerType |= REQUEST_GPS;
                }

            }
        }
        else
        {
            gpsHandlerStarted = FALSE;
            LS_LOG_DEBUG("GPS is OFF\n");
        }
    }

    /*Handler WIFI*/
    if(handlerStatus.wifi == HANDLER_STATE_ENABLED)
    {
        if(getHandlerStatus(NETWORK))
        {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),HANDLER_WIFI);////TODO Handler Type needs to passed as wifi
            if(ret)
            {
                LS_LOG_DEBUG("Failed to start WIFI-NETWORK handler errorcode =%d",ret);
                wifiHandlerStarted = FALSE;
            }
            else
            {
                LS_LOG_DEBUG("WIFI-NETWORK handler started successfully\n");

                ret = ::getCurrentPosition(handler_array[HANDLER_NW], START, wrapper_getCurrentPosition_cb,HANDLER_WIFI,sh);
                if(ret == ERROR_NOT_AVAILABLE)
                {
                    ::getCurrentPosition(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb,HANDLER_WIFI,sh);
                    handler_stop(handler_array[HANDLER_NW],HANDLER_WIFI);
                    LS_LOG_DEBUG("getCurrentPosition failed errorcode %d",ret);
                }
                else
                {
                    wifiHandlerStarted = TRUE;
                    handlerType |= REQUEST_WIFI;
                }
            }
        }
        else
        {
            wifiHandlerStarted = FALSE;
            LS_LOG_DEBUG("Location WIFI Source OFF in settings");
        }
    }

    /*Handler CELL-ID*/
    if(handlerStatus.cellid == HANDLER_STATE_ENABLED)
    {
        if(getHandlerStatus(NETWORK))
        {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),HANDLER_CELLID); //TODO Handler Type needs to passed as cell-id
            if(ret)
            {
                LS_LOG_DEBUG("Failed to start CELI-ID-NETWORK handler errorcode =%d",ret);
                cellidHandlerStarted = FALSE;
            }
            else
            {
                LS_LOG_DEBUG("CELL-ID NETWORK handler started successfully");

                ret = ::getCurrentPosition(handler_array[HANDLER_NW], START, wrapper_getCurrentPosition_cb,HANDLER_CELLID,sh);
                if(ret == ERROR_NOT_AVAILABLE)
                {
                    ::getCurrentPosition(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb,HANDLER_CELLID,sh);
                    handler_stop(handler_array[HANDLER_NW],HANDLER_CELLID);
                    LS_LOG_DEBUG("getCurrentPosition HANDLER_NW failed errorcode %d",ret);
                }
                else
                {
                    cellidHandlerStarted = TRUE;
                    handlerType |= REQUEST_CELLID;
                }
            }
        }
        else
        {
            cellidHandlerStarted = FALSE;
            LS_LOG_DEBUG("CELL-ID Network settings is OFF");
        }
    }
    if (!gpsHandlerStarted && !wifiHandlerStarted && !cellidHandlerStarted)
    {
        LS_LOG_DEBUG("No handlers started");
        LSMessageReplyError(sh,message,TRACKING_LOCATION_OFF);
        if(serviceObject!= NULL)
            json_object_put(serviceObject);
        LSMessageUnref(message);
        return true;
    }
    else
    {
        //Start Timer
        responseTime = ConvetResponseTimeFromLevel(accuracyLevel,responseTimeLevel);
        timeinSec = responseTime/1000;
        timerdata = new (std::nothrow)TimerData(message, sh,handlerType);
        if(timerdata == NULL)
        {
            LS_LOG_DEBUG("timerdata null Out of memory");
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
            if(serviceObject!= NULL)
                json_object_put(serviceObject);
            LSMessageUnref(message);
            return true;
        }
        timerID = g_timeout_add_seconds(timeinSec, &TimerCallback, timerdata);
        LS_LOG_DEBUG("Add to Requestlist handlerType=%x timerID=%d",handlerType,timerID);

        //Add to Requestlist
        request = new (std::nothrow)Request(message,sh,timerID,handlerType,timerdata);
        if(request == NULL)
        {
            LS_LOG_DEBUG("req null Out of memory");
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
            if(serviceObject!= NULL)
                json_object_put(serviceObject);
            LSMessageUnref(message);
            return true;
        }
        boost::shared_ptr<Request> req(request);
        m_reqlist.push_back(req);
        LS_LOG_DEBUG("message=%p sh=%p",message,sh);
        LS_LOG_DEBUG("Handler Started are gpsHandlerStarted=%d,wifiHandlerStarted=%d,cellidHandlerStarted=%d",gpsHandlerStarted,wifiHandlerStarted,cellidHandlerStarted);
        if(serviceObject!= NULL)
            json_object_put(serviceObject);
        return true;
    }
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
bool LocationService::readLocationfromCache(LSHandle *sh,LSMessage *message,json_object *serviceObject,int maxAge,HandlerStatus handlerstatus)
{

    LS_LOG_DEBUG("=======GetCurrentPosition=======");
    int ret;
    bool gpsCacheSuccess = false;
    bool wifiCacheSuccess = false;
    bool cellCacheSucess = false;
    Position pos;
    Accuracy acc;
    Position gpspos;
    Accuracy gpsacc;
    Position wifipos;
    Accuracy wifiacc;
    Position cellidpos;
    Accuracy cellidacc;
    LSError mLSError;

    memset(&gpspos,0,sizeof(Position));
    memset(&wifipos,0,sizeof(Position));
    memset(&cellidpos,0,sizeof(Position));

    memset(&gpsacc,0,sizeof(Accuracy));
    memset(&wifiacc,0,sizeof(Accuracy));
    memset(&cellidacc,0,sizeof(Accuracy));


    if(getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),&gpspos,&gpsacc,HANDLER_GPS))
        gpsCacheSuccess = TRUE;
    /* if(getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),&wifipos,&wifiacc,HANDLER_WIFI))
       wifiCacheSuccess = TRUE;
       if(getCachedDatafromHandler(HANDLER_INTERFACE(handler_array[HANDLER_NW]),&cellidpos,&cellidacc,HANDLER_CELLID))
       cellCacheSucess = TRUE;*/

    if(maxAge == -1)
    {
        //compare all three source location
        if (gpsCacheSuccess &&(gpspos.timestamp > wifipos.timestamp && gpspos.timestamp > cellidpos.timestamp))
        {
            // gpspos is latest
            pos = gpspos;
            acc = gpsacc;
        }
        else if(wifiCacheSuccess && (wifipos.timestamp > gpspos.timestamp && wifipos.timestamp > cellidpos.timestamp))
        {
            //wifipos is latest
            pos = wifipos;
            acc = wifiacc;
        }
        else if(cellCacheSucess)
        {
            //cellid latest
            pos = cellidpos;
            acc = cellidacc;
        }
        else
        {
            LS_LOG_DEBUG("No Cache data present");
            return false;
        }
        location_util_add_pos_json(serviceObject, &pos);
        location_util_add_acc_json(serviceObject, &acc);
        mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
        if(mRetVal == false)
            LSErrorPrintAndFree(&mLSError);
        return true;
    }
    else
    {
        long long currentTime = ::time(0);
        currentTime *= 1000; // milli sec

        //More accurate data return first
        if((handlerstatus.gps == HANDLER_STATE_ENABLED) && gpsCacheSuccess && (maxAge >= currentTime - gpspos.timestamp))
        {
            location_util_add_pos_json(serviceObject, &gpspos);
            location_util_add_acc_json(serviceObject, &gpsacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
            if(mRetVal == false)
            {
                LSErrorPrintAndFree(&mLSError);
            }
            else
                return true;
        }
        if((handlerstatus.wifi == HANDLER_STATE_ENABLED) && wifiCacheSuccess && (maxAge >= currentTime - wifipos.timestamp))
        {
            location_util_add_pos_json(serviceObject, &wifipos);
            location_util_add_acc_json(serviceObject, &wifiacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
            if(mRetVal == false)
                LSErrorPrintAndFree(&mLSError);
            else
                return true;

        }
        if((handlerstatus.cellid == HANDLER_STATE_ENABLED) && cellCacheSucess && (maxAge >= currentTime - cellidpos.timestamp))
        {
            location_util_add_pos_json(serviceObject, &cellidpos);
            location_util_add_acc_json(serviceObject, &cellidacc);
            mRetVal = LSMessageReply(sh, message, json_object_to_json_string(serviceObject), &mLSError);
            if(mRetVal == false)
            {
                LSErrorPrintAndFree(&mLSError);
            }
            else
                return true;
        }
        return false;
    }
}

bool LocationService::getCachedDatafromHandler(Handler *hdl,Position *pos,Accuracy *acc,int type)
{
    bool ret;
    LSError mLSError;
    ret = handler_get_last_position(hdl,pos,acc,type);
    LS_LOG_DEBUG("ret value getCachedDatafromHandler and Cached handler type=%d",ret);
    if(ret == ERROR_NONE)
        return true;
    else return false;
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
void LocationService::handler_Selection(LSHandle *sh, LSMessage *message,void *data, int *accuracy, int *responseTime,HandlerStatus *handlerStatus)
{
    LS_LOG_DEBUG("handler_Selection\n");
    bool mboolRespose = false;
    bool mboolAcc = false;
    m_JsonArgument = json_tokener_parse((const char*)LSMessageGetPayload(message));
    mboolAcc = json_object_object_get_ex(m_JsonArgument, "accuracy", &m_JsonSubArgument);
    if(m_JsonSubArgument)
        *accuracy = json_object_get_int(m_JsonSubArgument);
    mboolRespose = json_object_object_get_ex(m_JsonArgument, "responseTime", &m_JsonSubArgument);
    if(m_JsonSubArgument)
        *responseTime = json_object_get_int(m_JsonSubArgument);

    LS_LOG_DEBUG("\naccuracy=%d responseTime=%d\n",*accuracy,*responseTime);

    if (mboolAcc || mboolRespose )
    {
        if(3 == *accuracy &&  1 == *responseTime)
        {
            handlerStatus->cellid = HANDLER_STATE_ENABLED;  //CELLID
        }
        else
            handlerStatus->wifi = HANDLER_STATE_ENABLED;  //WIFI;
        if(*accuracy != 3)
            handlerStatus->gps = HANDLER_STATE_ENABLED;   //GPS;

    }
    else
    {
        LS_LOG_DEBUG("Default gps handler assigned\n");
        handlerStatus->gps = HANDLER_STATE_ENABLED;
    }
}

int LocationService::ConvetResponseTimeFromLevel(int accLevel,int responseTimeLevel)
{
    int responseTime;
    if (responseTimeLevel == 1)
    {
        //High 10 sec
        responseTime = 50000;//proper value 10000
    }
    else if (responseTimeLevel == 2)
    {
        // mediuRemoveCellWifiInfoListnerm range 30 sec to 60 sec
        if (accLevel == 1)
        {
            // if need greater accuracy without any time limit then  go for the big response time as
            // GPS fix may take that long
            responseTime = 60000;
        }
        else
            responseTime = 30000;
    }
    else
    {
        //Low 100 sec to 60 sec
        if (accLevel == 1)
        {
            // if need greater accuracy without any time limit then  go for the big response time as
            // GPS fix may take that long
            responseTime = 100000;
        }
        else
            responseTime = 60000;
    }
    LS_LOG_DEBUG("responseTime=%d",responseTime);
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
bool LocationService::startTracking(LSHandle *sh, LSMessage *message,void *data)
{
    LS_LOG_DEBUG("=======startTracking=======");
    //LSMessageRef(message); //Not required as the message is not stored
    bool wifiHandlerStarted = false;
    bool gpsHandlerStarted = false;
    bool cellHandlerStarted = false;
    LSMessagePrint(message, stdout);
    LSSubscriptionIter* iter = NULL;
    bool ListFilled = FALSE;
    int ret;
    LSError mLSError;
    //GPS and NW both are off
    if(!getHandlerStatus("gps") && !getHandlerStatus("network"))
    {
        LS_LOG_DEBUG("GPS status is OFF");
        LSMessageReplyError(sh,message,TRACKING_LOCATION_OFF);
        return true;
    }

    if(getHandlerStatus("gps"))
    {
        LS_LOG_DEBUG("gps_handler is on");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_GPS);
        if(ret)
        {
            LS_LOG_DEBUG("GPS handler not started...StartTracking failed");
        }
        else
            gpsHandlerStarted = true;
    }
    if(getHandlerStatus("network"))
    {
        LS_LOG_DEBUG("network Settings is on");

        // ret_cell = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_CELLID);
        if(wifistate == true)
        {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_WIFI);
            if(ret == ERROR_NONE)
                wifiHandlerStarted = true;
        }

        if(isInternetConnectionAvailable == true)
        {
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]), HANDLER_CELLID);
            if(ret == ERROR_NONE)
                cellHandlerStarted = true;
        }

        // Add to subsciption list with method+handler as key
    }
    if(!wifiHandlerStarted && !gpsHandlerStarted && !cellHandlerStarted)
    {
        LS_LOG_DEBUG("All handlers not started");
        LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        return true;
    }
    if (LSMessageIsSubscription(message))
    {
        mRetVal = LSSubscriptionAdd(sh, SUBSC_START_TRACK_KEY, message, &mLSError);
        if (mRetVal == false)
        {
            LSErrorPrintAndFree(&mLSError);
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
            return true;
        }
        mRetVal = LSMessageReplySubscriptionSuccess(sh,message);
        if(mRetVal == false)
            return true;
    }


    else
    {
        LS_LOG_DEBUG("Not a subscription call");
        LSMessageReplyError(sh,message,TRACKING_NOT_SUBSC_CALL);
        return true;
    }
    //is it only for GPS?
    if (gpsHandlerStarted)
    {
        ::startTracking((Handler*) handler_array[HANDLER_GPS],START, wrapper_startTracking_cb, HANDLER_GPS, sh);
        trackhandlerstate |= REQUEST_GPS;
    }
    if(wifiHandlerStarted)
    {
        //TODO get all AP List and pass to handler.
        //LSCall(sh,"palm://com.palm.wifi/findnetworks", "{\"subscribe\":true}", LocationService::GetWifiApsList_cb, this,NULL,&mLSError);
        ::startTracking((Handler*) handler_array[HANDLER_NW],START, wrapper_startTracking_cb, HANDLER_WIFI, sh);
        trackhandlerstate |= REQUEST_WIFI;
        //::startTracking((Handler*) service_agent_prop->wifi_handler, wrapper_startTracking_cb, service_agent_prop->ServiceObjRef);

    }
    if(cellHandlerStarted)
    {
        ::startTracking((Handler*) handler_array[HANDLER_NW],START, wrapper_startTracking_cb, HANDLER_CELLID, sh);
        trackhandlerstate |= REQUEST_CELLID;
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
    ::getReverseLocation();
    return true;
}
bool LocationService::getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data)
{
    ::getGeoCodeLocation();
    return true;
}
bool LocationService::getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data)
{
    ::getAllLocationHandlers();
    return true;
}
bool LocationService::SetGpsStatus(LSHandle *sh, LSMessage *message, void *data)
{
    ::SetGpsStatus();
    return true;
}
bool LocationService::GetGpsStatus(LSHandle *sh, LSMessage *message, void *data)
{
    ::GetGpsStatus();
    return true;
}
bool LocationService::getState(LSHandle *sh, LSMessage *message, void *data)
{
    //::getState();
    struct json_object *serviceObject = json_object_new_object();
    int state;
    int lpret;
    bool returnValue;
    LPAppHandle lpHandle = 0;
    char* handler;
    LSError mLSError;
    m_JsonArgument = json_tokener_parse((const char*)LSMessageGetPayload(message));
    mRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);
    handler = json_object_get_string(m_JsonSubArgument);
    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE)
    {
        lpret=LPAppCopyValueInt(lpHandle, handler, &state);
        LPAppFreeHandle(lpHandle, true);
        if(lpret)
        {
            LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
            return true;
        }
        json_object_object_add(serviceObject, "errorCode",json_object_new_int(SUCCESS));
        json_object_object_add(serviceObject, "state",json_object_new_int(state));
        json_object_put(serviceObject);
        LS_LOG_DEBUG("json_object_to_json_string(serviceObject)=%d\n",state);
        LSMessageReply(sh, message, json_object_to_json_string(serviceObject) , &mLSError);
        return true;
    }
    else
    {
        LS_LOG_DEBUG("LPAppGetHandle is not created ");
        LSMessageReplyError(sh,message,TRACKING_UNKNOWN_ERROR);
        return true;
    }
}
bool LocationService::setState(LSHandle *sh, LSMessage *message, void *data)
{
    char* handler;
    char input[10];
    char resultstr[50];
    bool state;
    int ret = UNKNOWN_ERROR;
    LSError mLSError;
    LS_LOG_DEBUG("[DEBUG] setState\n");
    m_JsonArgument = json_tokener_parse((const char*)LSMessageGetPayload(message));
    mRetVal = json_object_object_get_ex(m_JsonArgument, "Handler", &m_JsonSubArgument);
    handler = json_object_get_string(m_JsonSubArgument);
    mRetVal = json_object_object_get_ex(m_JsonArgument, "state", &m_JsonSubArgument);
    state = json_object_get_boolean(m_JsonSubArgument);
    sprintf(input, "%d", state);

    //#if defined(1)
    LPAppHandle lpHandle = 0;
    if (LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE)
    {
        LS_LOG_DEBUG("%s: Writing handler state",__FUNCTION__);
        LPAppSetValueInt(lpHandle, handler,state);
        LPAppFreeHandle(lpHandle, true);
        ret = SUCCESS;
        sprintf(resultstr, "{\"errorCode\":%d}", ret);
        LSMessageReply(sh, message, resultstr , &mLSError);
        return true;
    }
    else
    {
        LS_LOG_DEBUG("LPAppGetHandle is not created");
        LSMessageReplyError(sh,message,ret);
        return true;
    }
    //#endif
    // Reviewed by Sathya & Abhishek
    // Freeing m_JsonArgument
    json_object_put(m_JsonArgument);
    return true;
}
bool LocationService::sendExtraCommand(LSHandle *sh, LSMessage *message, void *data)
{
    ::sendExtraCommand();
    return true;
}
bool LocationService::getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data)
{
    ::getLocationHandlerDetails();
    return true;
}
bool LocationService::getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data)
{
    const char *errorCode;
    int ret;
    int count=0;
    LSSubscriptionIter* iter = NULL;
    LSError mLSError;
    if(getHandlerStatus("gps"))
    {
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]), HANDLER_TYPE_GPS);
        if(ret)
        {
            LS_LOG_DEBUG("[DEBUG] %s::%s( %d ) GPS handler not started...StartTracking failed\n",__FILE__,__PRETTY_FUNCTION__,__LINE__);
            errorCode = "{\"returnValue\":true, \"errorCode\":\"GPS handler not started\"}";
            LSMessageReply(sh, message, errorCode, &mLSError);
            return true;
        }
    }
    else
    {
        LS_LOG_DEBUG("[DEBUG] GPS status is OFF\n");
        errorCode = "{\"returnValue\":true, \"errorCode\":\"GPS status is OFF\"}";
        LSMessageReply(sh, message, errorCode, &mLSError);
        return true;
    }
    mRetVal = LSSubscriptionAcquire(sh, SUBSC_GET_GPS_SATELLITE_DATA, &iter, &mLSError);

    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return true;
    }

    mRetVal = LSSubscriptionAdd(sh, SUBSC_GET_GPS_SATELLITE_DATA, message, &mLSError);
    if (mRetVal == false)
    {
        LSErrorPrintAndFree(&mLSError);
        return true;
    }

    while(LSSubscriptionHasNext(iter))
    {
        count++;
        LSMessage* msg = LSSubscriptionNext(iter);
        LS_LOG_DEBUG("[DEBUG] LSMessageGetMethod(message)=%s\n",LSMessageGetMethod(message));
    }
    if(handler_array[HANDLER_GPS] != NULL)
        ::getGpsSatelliteData(handler_array[HANDLER_GPS], START, wrapper_getGpsSatelliteData_cb);
    return true;

}
bool LocationService::getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data)
{
    ::getTimeToFirstFix();
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
void wrapper_startTracking_cb(gboolean enable_cb,Position* pos, Accuracy* accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_startTracking_cb");
    LocationService::start_tracking_cb(pos,accuracy, error, privateIns, type);

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
void wrapper_getCurrentPosition_cb(gboolean enable_cb ,Position *position, Accuracy *accuracy,int error,gpointer privateIns,int handlerType){
    LS_LOG_DEBUG("wrapper_getCurrentPosition_cb called back from handlerType=%d",handlerType);
    LocationService::getCurrentPosition_cb(position,accuracy, error, privateIns,handlerType);
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
    LocationService::get_nmea_cb(enable_cb,timestamp,nmea,length,privateIns);
}

void getReverseLocation_cb(gboolean enable_cb,Address* address, gpointer privateIns)
{

}
void getGeoCodeLocation_cb(gboolean enable_cb,Position* position, gpointer privateIns)
{

}
void wrapper_getGpsSatelliteData_cb(gboolean enable_cb,Satellite *satellite,
        gpointer privateIns)
{
    LS_LOG_DEBUG("[DEBUG] wrapper_getGpsSatelliteData_cb called back ...");
    LocationService::getGpsSatelliteData_cb(enable_cb,satellite,privateIns);
}
void sendExtraCommand_cb(gboolean enable_cb,int command, gpointer privateIns)
{

}


/********************************Response to Application layer*********************************************************************/

void LocationService::startTracking_reply(Position* pos,Accuracy *accuracy, int error, int type)
{
    LS_LOG_DEBUG("[DEBUG] startTracking_reply called");
    struct json_object *serviceObject;
    LSError mLSError;
    char mResult[40];

    if( error == ERROR_NONE){
        trackhandlerstate = 0;

        serviceObject = json_object_new_object();
        location_util_add_returnValue_json(serviceObject, true);
        location_util_add_errorCode_json(serviceObject, SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject, accuracy);
        mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);
        if(!mRetVal)
            LSErrorPrintAndFree(&mLSError);
        mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_START_TRACK_KEY, json_object_to_json_string(serviceObject), &mLSError);
        if(!mRetVal)
            LSErrorPrintAndFree(&mLSError);
        json_object_put(serviceObject);
    }
    else{
        switch(type){
            case HANDLER_GPS:
                trackhandlerstate &= ~REQUEST_GPS;
                break;
            case HANDLER_WIFI:
                trackhandlerstate &= ~REQUEST_WIFI;
                break;
            case HANDLER_CELLID:
                trackhandlerstate &= ~REQUEST_CELLID;
                break;
        }
        if(trackhandlerstate == 0)
        {
            sprintf(mResult, "{\"returnValue\":true, \"errorCode\":%d}", TRACKING_TIMEOUT);
            mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_START_TRACK_KEY, mResult, &mLSError);
            if(mRetVal == FALSE)
                LSErrorPrintAndFree(&mLSError);
            mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_START_TRACK_KEY, mResult, &mLSError);
            if(mRetVal == FALSE)
                LSErrorPrintAndFree(&mLSError);

        }
    }

}


void LocationService::get_nmea_reply(int timestamp,char *data,int length)
{
    LS_LOG_DEBUG("[DEBUG] get_nmea_reply called");
    char* errorCode;
    struct json_object *serviceObject;
    LSError mLSError;
    serviceObject = json_object_new_object();
    LS_LOG_DEBUG(" timestamp=%d\n",timestamp);
    LS_LOG_DEBUG("data=%s\n",data);
    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, SUCCESS);
    json_object_object_add(serviceObject, "timestamp", json_object_new_int(timestamp));
    if(data != NULL)
        json_object_object_add(serviceObject, "nmea", json_object_new_string (data));


    LS_LOG_DEBUG("length=%d\n",length);
    LS_LOG_DEBUG("mServiceHandle=%x\n",mServiceHandle);

    mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_GPS_GET_NMEA_KEY,json_object_to_json_string(serviceObject), &mLSError);

    if(mRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_GPS_GET_NMEA_KEY,json_object_to_json_string(serviceObject), &mLSError);
    if(mRetVal == false)
        LSErrorPrintAndFree(&mLSError);

    json_object_put(serviceObject);//freeing the memory
}

void LocationService::getCurrentPosition_reply(Position *pos, Accuracy *accuracy, int error,int handlerType)
{
    LSSubscriptionIter* iter = NULL;
    int count = 0;
    LS_LOG_DEBUG("======getCurrentPosition_reply called=======");
    struct json_object *serviceObject;
    serviceObject = json_object_new_object();
    LSError mLSError;
    TimerData *timerdata;
#if 0
    testjson(serviceObject);
#else
    location_util_add_returnValue_json(serviceObject, true);
    location_util_add_errorCode_json(serviceObject, SUCCESS);
    location_util_add_pos_json(serviceObject, pos);
    location_util_add_acc_json(serviceObject, accuracy);
#endif

    /*List Implementation*/
    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();

    if(size <= 0)
    {
        LS_LOG_DEBUG("List is empty");
        return;
    }

    for(it = m_reqlist.begin(); count < size; it++,count++)
    {
        LS_LOG_DEBUG("count = %d size= %d",count,size);

        //Match Handlertype and GetReqhandlerType
        if(*it != NULL)
        {
            LS_LOG_DEBUG("it->get()->GetReqHandlerType()=%d",it->get()->GetReqHandlerType());
            if((1u << handlerType) & it->get()->GetReqHandlerType())
            {
                LS_LOG_DEBUG("handlerType = %d bit is set in the Request",handlerType);
                LS_LOG_DEBUG("it->get()->GetHandle() = %p it->get()->GetMessage() = %p",it->get()->GetHandle(),it->get()->GetMessage());
                LS_LOG_DEBUG("Payload=%s",LSMessageGetPayload(it->get()->GetMessage()));
                LS_LOG_DEBUG("App ID = %s",LSMessageGetApplicationID(it->get()->GetMessage()));
                if(handlerType == HANDLER_WIFI || handlerType == HANDLER_CELLID)
                    handler_stop(handler_array[HANDLER_NW],handlerType);
                else
                    handler_stop(handler_array[handlerType],handlerType);
                LSMessageReply(it->get()->GetHandle(), it->get()->GetMessage(), json_object_to_json_string(serviceObject), &mLSError);
                LSMessageUnref(it->get()->GetMessage());
                //Remove timer
                g_source_remove(it->get()->GetTimerID());
                timerdata = it->get()->GetTimerData();
                if(timerdata != NULL)
                {
                    delete timerdata;
                    timerdata = NULL;
                }

                //Remove from list
                m_reqlist.erase(it);
            }
        }
    }
    json_object_put(serviceObject);
}

void LocationService::getGpsSatelliteData_reply(Satellite* sat)
{
    LS_LOG_DEBUG("[DEBUG] getGpsSatelliteData_reply called, reply to application\n");
    int num_satellite_used_count = 0;
    LSSubscriptionIter* iter = NULL;
    char* resString;
    /*Creating a json object*/
    struct json_object *serviceObject = json_object_new_object();
    LSError mLSError;
    while(num_satellite_used_count < sat->visible_satellites_count)
    {
        LS_LOG_DEBUG(" Service Agent value of %llf ", sat->sat_used[num_satellite_used_count].azimuth);
        json_object_object_add(serviceObject, "azimuth",json_object_new_double(sat->sat_used[num_satellite_used_count].azimuth));
        json_object_object_add(serviceObject, "elevation",json_object_new_double(sat->sat_used[num_satellite_used_count].elevation));
        json_object_object_add(serviceObject, "PRN",json_object_new_int(sat->sat_used[num_satellite_used_count].prn));
        LS_LOG_DEBUG(" Service Agent value of %llf", sat->sat_used[num_satellite_used_count].snr);
        json_object_object_add(serviceObject, "snr",json_object_new_double(sat->sat_used[num_satellite_used_count].snr));
        json_object_object_add(serviceObject, "hasAlmanac", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasalmanac));
        json_object_object_add(serviceObject, "hasEphemeris", json_object_new_boolean(sat->sat_used[num_satellite_used_count].hasephemeris));
        json_object_object_add(serviceObject, "usedInFix", json_object_new_boolean(sat->sat_used[num_satellite_used_count].used));
        json_object_object_add(serviceObject, "VisibleSatellites", json_object_new_boolean(sat->visible_satellites_count));
        num_satellite_used_count++;
        resString = json_object_to_json_string(serviceObject);

        mRetVal = LSSubscriptionRespond(mServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);
        if (!mRetVal)
            LSErrorPrintAndFree(&mLSError);
        mRetVal = LSSubscriptionRespond(mlgeServiceHandle, SUBSC_GET_GPS_SATELLITE_DATA, resString, &mLSError);
        if (!mRetVal)
            LSErrorPrintAndFree(&mLSError);

    }
    json_object_put(serviceObject);


    LS_LOG_DEBUG("[DEBUG] mRetVal=%d\n",mRetVal);
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
    const char *key;
    LSError mLSError;
    LS_LOG_DEBUG("LSMessageGetMethod(message) %s strcmp=%d ",LSMessageGetMethod(message),strcmp(LSMessageGetMethod(message),"startTracking"));
    if(!strcmp(LSMessageGetMethod(message),"startTracking"))
        key = SUBSC_START_TRACK_KEY;
    else if(!strcmp(LSMessageGetMethod(message),"getNmeaData"))
        key = SUBSC_GPS_GET_NMEA_KEY;
    else if(!strcmp(LSMessageGetMethod(message),"getGpsSatelliteData"))
        key = SUBSC_GET_GPS_SATELLITE_DATA;

    LSSubscriptionIter* iter = NULL;
    if(key == NULL)
    {
        LS_LOG_DEBUG("[DEBUG] Not a valid key\n");
        return true;
    }

    mRetVal = LSSubscriptionAcquire(sh, key, &iter,&mLSError);
    if (!mRetVal)
    {
        LSErrorPrintAndFree(&mLSError);
        return false;
    }
    LSMessage* msg = LSSubscriptionNext(iter);
    LS_LOG_DEBUG("LSSubscriptionHasNext=%d ",LSSubscriptionHasNext(iter));
    if(!LSSubscriptionHasNext(iter))
        stopSubcription((Handler*) handler_array[HANDLER_GPS],key);
    LSSubscriptionRelease(iter);
    return true;
}


int LocationService::getHandlerStatus(const char* handler){
    LPAppHandle lpHandle = 0;
    int state = 0;
    int lpret;
    LS_LOG_DEBUG("getHandlerStatus ");
    if(LPAppGetHandle("com.palm.location", &lpHandle) == LP_ERR_NONE)
    {
        lpret = LPAppCopyValueInt(lpHandle, handler, &state);
        LS_LOG_DEBUG("state=%d\n",state);
        LPAppFreeHandle(lpHandle, true);
    }
    else
        LS_LOG_DEBUG("[DEBUG] handle initialization failed\n");
    return state;
}

void LocationService::stopSubcription(Handler *hdl,const char* key)
{
    if(!strcmp(key,SUBSC_START_TRACK_KEY))
    {
        ::startTracking((Handler*) handler_array[HANDLER_GPS],STOP, wrapper_startTracking_cb, HANDLER_GPS,NULL);
        handler_stop(handler_array[HANDLER_GPS],HANDLER_GPS);
    }
    else if(!strcmp(key,SUBSC_GPS_GET_NMEA_KEY))
    {
        ::getNmeaData((Handler*) handler_array[HANDLER_GPS],STOP,wrapper_getNmeaData_cb, NULL);
        handler_stop(handler_array[HANDLER_GPS],HANDLER_GPS);
    }
    else if(!strcmp(key,SUBSC_GET_GPS_SATELLITE_DATA))
    {
        ::getGpsSatelliteData(handler_array[HANDLER_GPS], STOP, wrapper_getGpsSatelliteData_cb);
        handler_stop(handler_array[HANDLER_GPS],HANDLER_GPS);
    }
}



/*Timer Implementation START */
gboolean LocationService::_TimerCallback(void* data)
{
    TimerData* timerdata = (TimerData*) data;
    if(timerdata == NULL)
    {
        LS_LOG_DEBUG("TimerCallback timerdata is null");
        return false;
    }
    LSMessage *message = timerdata->GetMessage();
    LSHandle *sh = timerdata->GetHandle();
    unsigned char handlerType = timerdata->GetHandlerType();

    if(message== NULL || sh == NULL)
    {
        LS_LOG_DEBUG("message or Handle is null");
        return false;
    }
    else
    {
        //Remove from list
        if(replyAndRemoveFromRequestList(sh,message,handlerType) == false)
            LS_LOG_DEBUG("Failed in replyAndRemoveFromRequestList");
    }
    // return false otherwise it will be called repeatedly until it returns false
    return false;
}

bool LocationService::replyAndRemoveFromRequestList(LSHandle* sh,LSMessage* message,unsigned char handlerType)
{
    bool found = false;

    std::vector<Requestptr>::iterator it = m_reqlist.begin();
    int size = m_reqlist.size();
    int count = 0;
    LSError mLSError;
    TimerData *timerdata;
    if(size <= 0)
    {
        LS_LOG_DEBUG("List is empty");
        return false;
    }
    for(it = m_reqlist.begin(); count < size; it++,count++)
    {

        if(it->get()->GetMessage() == message)
        {
            LS_LOG_DEBUG("Timeout Message is found in the list");

            if((1u << HANDLER_GPS) & handlerType)
            {
                ::getCurrentPosition(handler_array[HANDLER_GPS], STOP, wrapper_getCurrentPosition_cb,HANDLER_GPS,NULL);
                handler_stop(handler_array[HANDLER_GPS],HANDLER_GPS);

            }
            if((1u << HANDLER_WIFI) & handlerType)
            {
                ::getCurrentPosition(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb,HANDLER_WIFI,sh);
                handler_stop(handler_array[HANDLER_NW],HANDLER_WIFI);
            }
            if((1u << HANDLER_CELLID) & handlerType)
            {
                ::getCurrentPosition(handler_array[HANDLER_NW], STOP, wrapper_getCurrentPosition_cb,HANDLER_CELLID,sh);
                handler_stop(handler_array[HANDLER_NW],HANDLER_CELLID);
            }
            LSMessageReplyError(sh,message,TRACKING_TIMEOUT);
            LSMessageUnref(message);
            timerdata = it->get()->GetTimerData();
            if(timerdata != NULL)
            {
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
void LocationService::startWifiHandler()
{
    if(isSubscListEmpty(LSPalmServiceGetPrivateConnection(mServiceHandle),SUBSC_START_TRACK_KEY) == true)
    {
    }
}
void LocationService::startCellIDHandler()
{
    if(isSubscListEmpty(LSPalmServiceGetPrivateConnection(mServiceHandle),SUBSC_START_TRACK_KEY) == true)
    {
    }
}
bool LocationService::isSubscListEmpty(LSHandle *sh,const char *key)
{
    LSError mLSError;
    LSSubscriptionIter* iter = NULL;
    bool ret = false;
    mRetVal = LSSubscriptionAcquire(sh,key, &iter,&mLSError);
    if (!mRetVal)
    {
        LSErrorPrint(&mLSError, stderr);
        LSErrorFree(&mLSError);
        return false;
    }
    ret = LSSubscriptionHasNext(iter);
    LSSubscriptionRelease(iter);
    return ret;
}
