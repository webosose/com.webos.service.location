#include <lunaservice.h>
#include <LocationService_Log.h>
#include <LocationService.h>
#include <LunaLocationServiceUtil.h>
#include <LunaCriteriaCategoryHandler.h>
#include <JsonUtility.h>
#include <glib.h>
#include <glib-object.h>
#include <math.h>

/**
 @var LSMethod LocationService::rootMethod[]
 methods belonging to root category
 */
LSMethod LunaCriteriaCategoryHandler::criteriaCategoryMethods[] = {
    {"startTrackingCriteriaBased", _startTrackingCriteriaBased },
    {"getLocationCriteriaHandlerDetails", _getLocationCriteriaHandlerDetails },
    { 0, 0 }
};

LunaCriteriaCategoryHandler::LunaCriteriaCategoryHandler()
{
    mpalmSrvHandle = NULL;
    mpalmLgeSrvHandle = NULL;
    mlastLat = 0;
    mlastLong = 0;
    trackhandlerstate = 0;
    handler_array = NULL;
    LS_LOG_DEBUG("CriteriaCategoryLunaService object created");
}

LunaCriteriaCategoryHandler::~LunaCriteriaCategoryHandler()
{
    LS_LOG_DEBUG("CriteriaCategoryLunaService object destroyed");

    if (m_criteria_req_list.empty() == false) {
        m_criteria_req_list.clear();
    }
}

LunaCriteriaCategoryHandler *LunaCriteriaCategoryHandler::CriteriaCatSvc = NULL;

LunaCriteriaCategoryHandler *LunaCriteriaCategoryHandler::getInstance()
{
    if (CriteriaCatSvc == NULL) {
        CriteriaCatSvc = new(std::nothrow) LunaCriteriaCategoryHandler();
        return CriteriaCatSvc;
    } else {
        return CriteriaCatSvc;
    }
}

bool LunaCriteriaCategoryHandler::init(LSPalmService *sh, LSPalmService *sh_lge,
                                       Handler **handler1)
{
    bool retVal;
    LSError mLSError;

    LSErrorInit(&mLSError);
    mpalmSrvHandle = sh;
    mpalmLgeSrvHandle = sh_lge;
    handler_array = handler1;

    LSPalmServiceRegisterCategory(mpalmSrvHandle,
                                  "/criteria",
                                  criteriaCategoryMethods,
                                  NULL,
                                  NULL,
                                  this,
                                  &mLSError);
    LSERROR_CHECK_AND_PRINT(retVal);

    LSPalmServiceRegisterCategory(mpalmLgeSrvHandle,
                                  "/criteria",
                                  criteriaCategoryMethods,
                                  NULL,
                                  NULL,
                                  this,
                                  &mLSError);
    LSERROR_CHECK_AND_PRINT(retVal);
}

bool LunaCriteriaCategoryHandler::startTrackingCriteriaBased(LSHandle *sh,
                                                             LSMessage *message)
{
    int minInterval = 0;
    int minDistance = 0;
    int accLevel = LunaCriteriaCategoryHandler::NO_REQUIREMENT;
    int powLevel = LunaCriteriaCategoryHandler::NO_REQUIREMENT;
    char *handlerName = NULL;
    bool bRetVal = false;
    int sel_handler = 0;
    unsigned char startedHandlers = 0;
    char sub_key_gps[KEY_MAX] = { 0x00 };
    char sub_key_nw[KEY_MAX] = { 0x00 };
    char sub_key_gps_nw[KEY_MAX] = { 0x00 };
    LSError mLSError;
    jvalue_ref serviceObj = NULL;
    LocationErrorCode errorCode = LOCATION_SUCCESS;
    jvalue_ref parsedObj = NULL;

    LSErrorInit(&mLSError);
    LS_LOG_INFO("======startTrackingCriteriaBased=====");

    if (LocationService::getInstance()->isSubscribeTypeValid(sh, message, false, NULL) == false)
        return true;

    if ((LocationService::getInstance()->getHandlerStatus(GPS) == HANDLER_STATE_DISABLED)
         && (LocationService::getInstance()->getHandlerStatus(NETWORK) == HANDLER_STATE_DISABLED)) {

        LS_LOG_ERROR("Both handler are OFF\n");
        errorCode = LOCATION_LOCATION_OFF;
        goto EXIT;
    }

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_START_TRACKING_CRITERIA_BASED, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in StartTracking Criteria");
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumInterval"), &serviceObj)) {
        jnumber_get_i32(serviceObj,&minInterval);

        LS_LOG_DEBUG("minimumInterval %d", minInterval);

        if ((minInterval < LunaCriteriaCategoryHandler::MIN_RANGE || minInterval > LunaCriteriaCategoryHandler::MAX_RANGE)) {
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &serviceObj)) {
        jnumber_get_i32(serviceObj,&minDistance);

        LS_LOG_DEBUG("minimumInterval %d", minInterval);

        if (minDistance > LunaCriteriaCategoryHandler::MAX_RANGE) {
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("Handler"), &serviceObj)) {
        raw_buffer nameBuf = jstring_get(serviceObj);
        handlerName = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);

        LS_LOG_INFO("handlerName %s", handlerName);

        if (strcmp(handlerName, GPS) == 0)
            sel_handler = LunaCriteriaCategoryHandler::CRITERIA_GPS;
        else if (strcmp(handlerName, NETWORK) == 0)
            sel_handler = LunaCriteriaCategoryHandler::CRITERIA_NW;
        else {
            errorCode = LOCATION_INVALID_INPUT;
            goto EXIT;
        }

    } else {

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("accuracyLevel"), &serviceObj)) {
            jnumber_get_i32(serviceObj, &accLevel);

            if (accLevel <= 0 || accLevel >= 3) {
                errorCode = LOCATION_INVALID_INPUT;
                goto EXIT;
            }
        }


        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("powerLevel"), &serviceObj)) {
            jnumber_get_i32(serviceObj, &powLevel);

            if (powLevel <= 0 || powLevel >= 3) {
                errorCode = LOCATION_INVALID_INPUT;
                goto EXIT;
            }
        }

        sel_handler = handlerSelection(accLevel, powLevel);
    }

    LS_LOG_DEBUG("sel_handler %d", sel_handler);

    if (enableHandlers(sel_handler, sub_key_gps, sub_key_nw, sub_key_gps_nw, &startedHandlers)) {
        LS_LOG_DEBUG("startedHandlers %d sub_key_gps %s sub_key_nw %s sub_key_gps_nw %s",
                     startedHandlers, sub_key_gps, sub_key_nw,sub_key_gps_nw);
        struct timeval tv;
        gettimeofday(&tv, (struct timezone *) NULL);
        long long reqTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

        CriteriaRequest *criteriaReq = new(std::nothrow) CriteriaRequest(message,
                                                                         reqTime,
                                                                         LunaCriteriaCategoryHandler::INVALID_LAT,
                                                                         LunaCriteriaCategoryHandler::INVALID_LAT );

        if (criteriaReq == NULL) {
            LS_LOG_ERROR("Criteriareq null Out of memory");
            errorCode = LOCATION_OUT_OF_MEM;
            goto EXIT;
        }

        boost::shared_ptr<CriteriaRequest> req(criteriaReq);
        m_criteria_req_list.push_back(req);

        if (strcmp(sub_key_gps, GPS_CRITERIA_KEY) == 0) {
            if (startedHandlers != 0) {
                bRetVal = LSSubscriptionAdd(sh, GPS_CRITERIA_KEY, message, &mLSError);

                if (bRetVal == false) {
                    LSErrorPrintAndFree(&mLSError);
                    errorCode = LOCATION_UNKNOWN_ERROR;
                    goto EXIT;
                }
            }

            handler_start_tracking_criteria((Handler *) handler_array[HANDLER_GPS],
                                            START,
                                            start_tracking_criteria_cb,
                                            NULL,
                                            HANDLER_GPS,
                                            sh);

            trackhandlerstate = startedHandlers;
        }

        if (strcmp(sub_key_nw, NW_CRITERIA_KEY) == 0) {
            if (startedHandlers != 0) {
                bRetVal = LSSubscriptionAdd(sh, NW_CRITERIA_KEY, message, &mLSError);

                if (bRetVal == false) {
                    LSErrorPrintAndFree(&mLSError);
                    errorCode = LOCATION_UNKNOWN_ERROR;
                    goto EXIT;
                }
            }

            LS_LOG_INFO("startedHandlers %d", startedHandlers);

            if (startedHandlers & HANDLER_WIFI_BIT)
                handler_start_tracking_criteria((Handler *) handler_array[HANDLER_NW],
                                                START,
                                                start_tracking_criteria_cb,
                                                NULL,
                                                HANDLER_WIFI,
                                                sh);

            if (startedHandlers & HANDLER_CELLID_BIT)
                handler_start_tracking_criteria((Handler *) handler_array[HANDLER_NW],
                                                START,
                                                start_tracking_criteria_cb,
                                                NULL,
                                                HANDLER_CELLID,
                                                sh);

            trackhandlerstate = startedHandlers;
        }

        if (strcmp(sub_key_gps_nw, GPS_NW_CRITERIA_KEY) == 0)  {

            if (startedHandlers != 0) {
                bRetVal = LSSubscriptionAdd(sh, GPS_NW_CRITERIA_KEY, message, &mLSError);
                LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY added");
                if (bRetVal == false) {
                    LSErrorPrintAndFree(&mLSError);
                    errorCode = LOCATION_UNKNOWN_ERROR;
                    goto EXIT;
                }
            }

        if (startedHandlers & HANDLER_GPS_BIT)
            handler_start_tracking_criteria((Handler *) handler_array[HANDLER_GPS],
                                             START,
                                             start_tracking_criteria_cb,
                                             NULL,
                                             HANDLER_GPS,
                                             sh);

        if (startedHandlers & HANDLER_WIFI_BIT)
            handler_start_tracking_criteria((Handler *) handler_array[HANDLER_NW],
                                             START,
                                             start_tracking_criteria_cb,
                                             NULL,
                                             HANDLER_WIFI,
                                             sh);

        if (startedHandlers & HANDLER_CELLID_BIT)
            handler_start_tracking_criteria((Handler *) handler_array[HANDLER_NW],
                                             START,
                                             start_tracking_criteria_cb,
                                             NULL,
                                             HANDLER_CELLID,
                                             sh);

            trackhandlerstate = startedHandlers;
        }
    } else {

        LS_LOG_ERROR("Both handler are OFF\n");
        if (LocationService::getInstance()->getHandlerStatus(NETWORK)
            || LocationService::getInstance()->getHandlerStatus(GPS)) {

            if ((LocationService::getInstance()->getConnectionManagerState() == false
                && LocationService::getInstance()->getWifiInternetState() == false))
                errorCode = LOCATION_DATA_CONNECTION_OFF;
            else if (LocationService::getInstance()->getWifiState() == false)
                errorCode = LOCATION_WIFI_CONNECTION_OFF;
            else
                errorCode = LOCATION_UNKNOWN_ERROR;

        } else
            errorCode = LOCATION_LOCATION_OFF;
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
/* Returns
 0 for only network handler
 1 for both gps and network handler
 2 for only gps handler
 */
int LunaCriteriaCategoryHandler::handlerSelection(int accLevel, int powLevel)
{
    int ret;

    switch (accLevel) {
        case LunaCriteriaCategoryHandler::ACCURACY_FINE:
            //select gps and network handler both
            ret = LunaCriteriaCategoryHandler::CRITERIA_GPS_NW; //1
            break;

        case LunaCriteriaCategoryHandler::ACCURACY_COARSE:
            //select only network handler
            ret = LunaCriteriaCategoryHandler::CRITERIA_NW; //0
            break;

        default: {
            switch (powLevel) {
                case LunaCriteriaCategoryHandler::POWER_HIGH:
                    //select gps and network both
                    ret = LunaCriteriaCategoryHandler::CRITERIA_GPS_NW; //1;
                    break;

                default:
                    //select only network handler
                    ret = LunaCriteriaCategoryHandler::CRITERIA_NW; //0
                    break;
            }
        }
    }

    return ret;
}

int LunaCriteriaCategoryHandler::enableHandlers(int sel_handler,
                                                char *sub_key_gps,
                                                char *sub_key_nw,
                                                char *sub_key_gps_nw,
                                                unsigned char *startedHandlers)
{
    bool ret;

    switch (sel_handler) {
        case LunaCriteriaCategoryHandler::CRITERIA_GPS:
            ret = enableGpsHandler(startedHandlers);

            if (ret) {
                strcpy(sub_key_gps, GPS_CRITERIA_KEY);
            }

            break;

        case LunaCriteriaCategoryHandler::CRITERIA_NW:
            if (enableNwHandler(startedHandlers)) {
                strcpy(sub_key_nw, NW_CRITERIA_KEY);
            }

            break;

        case LunaCriteriaCategoryHandler::CRITERIA_GPS_NW:
            bool gpsHandlerStatus = enableGpsHandler(startedHandlers);
            bool wifiHandlerStatus= enableNwHandler(startedHandlers);

            if (gpsHandlerStatus || wifiHandlerStatus) {
                strcpy(sub_key_gps_nw, GPS_NW_CRITERIA_KEY);
            }

            break;
    }

    if (*startedHandlers == HANDLER_STATE_DISABLED) {
        LS_LOG_DEBUG("All handlers disabled");
        return false;
    }

    return true;
}

bool LunaCriteriaCategoryHandler::enableNwHandler(unsigned char *startedHandlers)
{
    bool ret = false;

    if (LocationService::getInstance()->getHandlerStatus(NETWORK)) {
        LS_LOG_DEBUG("network is on in Settings");

        if ((LocationService::getInstance()->getWifiState() == true) && (LocationService::getInstance()->getConnectionManagerState()
            || LocationService::getInstance()->getWifiInternetState())) { //wifistate == true)
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                HANDLER_WIFI, LocationService::getInstance()->getNwKey());

            if (ret != ERROR_NONE) {
                LS_LOG_ERROR("WIFI handler not started");
            } else
                *startedHandlers |= HANDLER_WIFI_BIT;
        }

        LS_LOG_INFO("isInternetConnectionAvailable %d TelephonyState %d",
                     LocationService::getInstance()->getConnectionManagerState(),
                     LocationService::getInstance()->getTelephonyState());

        if (LocationService::getInstance()->getTelephonyState()
            && (LocationService::getInstance()->getConnectionManagerState()
            || LocationService::getInstance()->getWifiInternetState())) {

            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                HANDLER_CELLID, LocationService::getInstance()->getNwKey());

            if (ret != ERROR_NONE) {
                LS_LOG_ERROR("CELLID handler not started");
            } else
                *startedHandlers |= HANDLER_CELLID_BIT;
        }

        if (*startedHandlers == HANDLER_STATE_DISABLED)
            return false;

        // Add to subsciption list with method+handler as key
    }

    return true;
}

bool LunaCriteriaCategoryHandler::enableGpsHandler(unsigned char *startedHandlers)
{
    LS_LOG_DEBUG("handler[HANDLER_GPS] = %u", handler_array[HANDLER_GPS]);

    if (LocationService::getInstance()->getHandlerStatus(GPS)) {
        bool ret = false;
        LS_LOG_DEBUG("gps is on in Settings");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                            HANDLER_GPS, NULL);

        if (ret != ERROR_NONE) {
            LS_LOG_ERROR("GPS handler not started");
            return false;
        }

        *startedHandlers |= HANDLER_GPS_BIT;
    } else
        return false;

    return true;
}

void LunaCriteriaCategoryHandler::LSErrorPrintAndFree(LSError *ptrLSError)
{
    if (ptrLSError != NULL) {
        LSErrorPrint(ptrLSError, stderr);
        LSErrorFree(ptrLSError);
    }
}

void LunaCriteriaCategoryHandler::startTrackingCriteriaBased_reply(Position *pos, Accuracy *accuracy, int error, int type)
{
    LS_LOG_DEBUG("===startTrackingCriteriaBased_reply called error %d===",error);
    char *key1 = NULL;
    char *key2 = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool bRetVal;
    jvalue_ref serviceObject = NULL;

    serviceObject = jobject_create();

    if (type == HANDLER_GPS) {
        key1 = GPS_CRITERIA_KEY;
        key2 = GPS_NW_CRITERIA_KEY;
    } else {
        key1 = NW_CRITERIA_KEY;
        key2 = GPS_NW_CRITERIA_KEY;
    }

    if (jis_null(serviceObject)) {
        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           mpalmLgeSrvHandle,
                                                           key1,
                                                           LSMessageGetErrorReply(LOCATION_OUT_OF_MEM),
                                                           &mLSError);
        LSERROR_CHECK_AND_PRINT(bRetVal);
        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           mpalmLgeSrvHandle,
                                                           key2,
                                                           LSMessageGetErrorReply(LOCATION_OUT_OF_MEM),
                                                           &mLSError);
        LSERROR_CHECK_AND_PRINT(bRetVal);
        return;
    }

    if (error == ERROR_NONE) {
        location_util_form_json_reply(serviceObject, true, LOCATION_SUCCESS);
        location_util_add_pos_json(serviceObject, pos);
        location_util_add_acc_json(serviceObject,accuracy);

        LS_LOG_DEBUG("key1 %s key2 %s", key1,key2);

        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           mpalmLgeSrvHandle,
                                                           key1,
                                                           jvalue_tostring_simple(serviceObject),
                                                           &mLSError);

        LSERROR_CHECK_AND_PRINT(bRetVal);
        /*reply to gps_nw subscription list*/
        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           mpalmLgeSrvHandle,
                                                           key2,
                                                           jvalue_tostring_simple(serviceObject),
                                                           &mLSError);
        LSERROR_CHECK_AND_PRINT(bRetVal);
        //Store last lat and long
        mlastLat = pos->latitude;
        mlastLong = pos->longitude;

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
            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmSrvHandle,
                                                               mpalmLgeSrvHandle,
                                                               GPS_CRITERIA_KEY,
                                                               LSMessageGetErrorReply(LOCATION_TIME_OUT),
                                                               &mLSError);
            LSERROR_CHECK_AND_PRINT(bRetVal);

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmSrvHandle,
                                                               mpalmLgeSrvHandle,
                                                               NW_CRITERIA_KEY,
                                                               LSMessageGetErrorReply(LOCATION_TIME_OUT),
                                                               &mLSError);
            LSERROR_CHECK_AND_PRINT(bRetVal);
            /*reply to gps nw subscription list */

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmSrvHandle,
                                                               mpalmLgeSrvHandle,
                                                               GPS_NW_CRITERIA_KEY,
                                                               LSMessageGetErrorReply(LOCATION_TIME_OUT),
                                                               &mLSError);
            LSERROR_CHECK_AND_PRINT(bRetVal);
        }
    }
    j_release(&serviceObject);
}

bool LunaCriteriaCategoryHandler::LSSubscriptionNonSubMeetsCriteriaRespond(Position *pos,
                                                                           LSPalmService *psh,
                                                                           LSPalmService *psh_lge,
                                                                           const char *key,
                                                                           const char *payload,
                                                                           LSError *lserror)
{
    LSHandle *public_bus = LSPalmServiceGetPublicConnection(psh);
    LSHandle *private_bus = LSPalmServiceGetPrivateConnection(psh);
    LSHandle *public_bus_lge = LSPalmServiceGetPublicConnection(psh_lge);
    LSHandle *private_bus_lge  = LSPalmServiceGetPrivateConnection(psh_lge);
    bool retVal;

    retVal = LSSubscriptionNonMeetsCriteriaReply(pos, public_bus, key, payload, lserror);

    if (retVal == false)
        return retVal;

    retVal = LSSubscriptionNonMeetsCriteriaReply(pos, private_bus, key, payload, lserror);

    if (retVal == false)
        return retVal;

    retVal = LSSubscriptionNonMeetsCriteriaReply(pos, public_bus_lge, key, payload, lserror);

    if (retVal == false)
        return retVal;


    retVal = LSSubscriptionNonMeetsCriteriaReply(pos, private_bus_lge, key, payload, lserror);

    if (retVal == false)
        return retVal;

}

bool LunaCriteriaCategoryHandler::LSSubscriptionNonMeetsCriteriaReply(Position *pos,
                                                                      LSHandle *sh,
                                                                      const char *key,
                                                                      const char *payload,
                                                                      LSError *lserror)
{
    LSSubscriptionIter *iter = NULL;
    bool retVal;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    jschema_ref input_schema = NULL ;
    JSchemaInfo schemaInfo;
    int minDist;
    int minInterval;
    LS_LOG_DEBUG("key = %s", key);
    bool isNonSubscibePresent = false;


    retVal = LSSubscriptionAcquire(sh, key, &iter, lserror);

    if (retVal == true && LSSubscriptionHasNext(iter)) {
        do {
            LSMessage *msg = LSSubscriptionNext(iter);
            //parse message to get minDistance
            if (!LSMessageIsSubscription(msg))
                 isNonSubscibePresent = true;

            input_schema = jschema_parse (j_cstr_to_buffer(SCHEMA_ANY), DOMOPT_NOOPT, NULL);

            if(!input_schema)
               return false;

            jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
            parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)), DOMOPT_NOOPT, &schemaInfo);

            if (jis_null(parsedObj)) {
                LS_LOG_ERROR("parsing error");
                return false;
            }

            //Check Minimum Interval
            if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumInterval"), &jsonSubObject)) {
                //check for criteria meets minimumDistance
                jnumber_get_i32(jsonSubObject, &minInterval);
                LS_LOG_DEBUG("minInterval in message = %d", minInterval);

                if (meetsCriteria(msg, pos, minInterval, 0, true, false)) {

                    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &jsonSubObject)) {
                        jnumber_get_i32(jsonSubObject, &minDist);

                        if (meetsCriteria(msg, pos, 0, minDist, false, true)) {
                            LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                        }
                    } else {
                        LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                    }
                }
            } else {
                //Check minimum Distance
                if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &jsonSubObject)) {
                    //check for criteria meets minimumDistance
                    jnumber_get_i32(jsonSubObject, &minDist);

                    if (meetsCriteria(msg, pos, 0, minDist, false, true))
                        LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                } else {
                    //No critria, just reply
                    LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                }
            }

            j_release(&parsedObj);

            // check for key sub list and gps_nw sub list*/
            if (isNonSubscibePresent) {
                if (strcmp(key, GPS_NW_CRITERIA_KEY) == 0) {

                    //check gps list empty
                    if((LocationService::getInstance()->isSubscListFilled(sh, msg, GPS_NW_CRITERIA_KEY, false) ==  false) && (LocationService::getInstance()->isSubscListFilled(sh, msg, GPS_CRITERIA_KEY, false) == false)) {
                        LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY and GPS_CRITERIA_KEY empty");
                        LocationService::getInstance()->stopNonSubcription(GPS_CRITERIA_KEY);
                    }

                    //check nw list empty and stop nw handler
                    if((LocationService::getInstance()->isSubscListFilled(sh, msg, GPS_NW_CRITERIA_KEY, false) == false) && LocationService::getInstance()->isSubscListFilled(sh, msg, NW_CRITERIA_KEY, false) == false) {
                        LS_LOG_DEBUG("GPS_NW_CRITERIA_KEY and NW_CRITERIA_KEY empty");
                        LocationService::getInstance()->stopNonSubcription(NW_CRITERIA_KEY);
                    }

                } else if((LocationService::getInstance()->isSubscListFilled(sh, msg,key, false) == false) && (LocationService::getInstance()->isSubscListFilled(sh, msg, GPS_NW_CRITERIA_KEY, false) == false)) {
                    LS_LOG_DEBUG("key %s empty",key);
                    LocationService::getInstance()->stopNonSubcription(key);
                }
            }
        } while (LSSubscriptionHasNext(iter));
    }
    LSSubscriptionRelease(iter);
    jschema_release(&input_schema);

    return retVal;
}

bool LunaCriteriaCategoryHandler::LSMessageReplyCriteria(LSMessage *msg,
                                                         Position *pos,
                                                         LSHandle *sh,
                                                         const char *key,
                                                         const char *payload,
                                                         LSSubscriptionIter *iter,
                                                         LSError *lserror)
{
    bool retVal = false;

    retVal = LSMessageReply(sh, msg, payload, lserror);

    if (!LSMessageIsSubscription(msg))
        LSSubscriptionRemove(iter);

    return retVal;
}

bool LunaCriteriaCategoryHandler::getLocationCriteriaHandlerDetails(LSHandle *sh, LSMessage *message)
{
    char *handler = NULL;
    jvalue_ref serviceObject = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool bRetVal;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;

    if (!LSMessageValidateSchema(sh, message, JSCHEMA_GET_LOCATION_CRITERIA_HANDLER_DETAILS, &parsedObj)) {
        LS_LOG_ERROR("Schema Error in StartTracking Criteria");
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

    if (handler == NULL) {
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

        LS_LOG_DEBUG("getHandlerStatus(GPS)");
        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("accuracyLevel"), jnumber_create_i32(LunaCriteriaCategoryHandler::ACCURACY_FINE));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("powerLevel"), jnumber_create_i32(LunaCriteriaCategoryHandler::POWER_HIGH));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresNetwork"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresCell"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("monetaryCost"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideAltitude"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideLongitude"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideLatitude"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideSpeed"), jboolean_create(true));

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else if (strcmp(handler, "network") == 0) {
        LS_LOG_DEBUG("Network details");
        jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(LOCATION_SUCCESS));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("accuracyLevel"), jnumber_create_i32(LunaCriteriaCategoryHandler::ACCURACY_COARSE));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("powerLevel"), jnumber_create_i32(LunaCriteriaCategoryHandler::POWER_LOW));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresNetwork"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("requiresCell"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("monetaryCost"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideAltitude"), jboolean_create(false));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideLongitude"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideLatitude"), jboolean_create(true));
        jobject_put(serviceObject, J_CSTR_TO_JVAL("provideSpeed"), jboolean_create(false));

        bRetVal = LSMessageReply(sh, message, jvalue_tostring_simple(serviceObject), &mLSError);

        if (bRetVal == false) {
            LSErrorPrintAndFree(&mLSError);
        }
    } else {
        LS_LOG_ERROR("LPAppGetHandle is not created");
        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
    }

    g_free(handler);
    j_release(&parsedObj);
    j_release(&serviceObject);

    return true;
}

void LunaCriteriaCategoryHandler::removeMessageFromCriteriaReqList(LSMessage *message)
{
    int count = 0;
    std::vector<CriteriaRequestPtr>::iterator it = m_criteria_req_list.begin();
    int size = m_criteria_req_list.size();
    LS_LOG_DEBUG("size = %d", size);

    for (it = m_criteria_req_list.begin(); count < size; ++it, count++) {
        if (*it != NULL && (it->get()->getMessage() == message)) {
            m_criteria_req_list.erase(it);
        }
    }
}

bool LunaCriteriaCategoryHandler::meetsCriteria(LSMessage *msg,
                                                Position *pos,
                                                int minInterval,
                                                int minDist,
                                                bool minIntervalCase,
                                                bool minDistCase )
{

    std::vector<CriteriaRequestPtr>::iterator it = m_criteria_req_list.begin();
    int size = m_criteria_req_list.size();
    int count = 0;
    bool isFirst = false;

    LS_LOG_DEBUG("m_criteria_req_list size = %d", size);

    if (size <= 0) {
        LS_LOG_DEBUG("Criteria Request list is empty");
        return false;
    }

    for (it = m_criteria_req_list.begin(); count < size; ++it, count++) {
        if (*it != NULL) {
            LS_LOG_DEBUG("List iterating");
            LSMessage *listmsg = it->get()->getMessage();

            if (msg == listmsg) {
                isFirst = it->get()->getFirstReply();

                if (isFirst) {
                    it->get()->updateFirstReply(false);
                }

                if (minIntervalCase) {
                    long long reqTime = it->get()->getRequestTime();
                    struct timeval tv;
                    gettimeofday(&tv, (struct timezone *) NULL);
                    long long currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

                    LS_LOG_DEBUG("currentTime-reqTime = %d ", currentTime - reqTime);
                    LS_LOG_DEBUG("minInterval = %d", minInterval);

                    if (isFirst || (currentTime - reqTime) > minInterval) {
                        it->get()->updateRequestTime(currentTime);
                        LS_LOG_DEBUG("Minimum Interval meets criteria");
                        return true;
                    }
                } else if (minDistCase) {
                    if (minDist == 0) {
                        LS_LOG_DEBUG("Minimum distance zero");
                        it->get()->updateLatAndLong(pos->latitude,pos->longitude);
                        return true;
                    }
                    double lastLat = it->get()->getLatitude();
                    double lastLong = it->get()->getLongitude();

                    if (isFirst || LocationService::getInstance()->minDistance(minDist, pos->latitude, pos->longitude, lastLat, lastLong)){
                        LS_LOG_DEBUG("Minimum Distance meets criteria");
                        it->get()->updateLatAndLong(pos->latitude,pos->longitude);
                        return true;
                    }
                }
            } else
                LS_LOG_DEBUG("m_criteria_req_list msg %p listmsg %p", msg, listmsg);
        } else
            LS_LOG_DEBUG("m_criteria_req_list Iterator is NULL");
    }
    return false;
}
