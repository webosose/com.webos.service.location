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
    mlastLat = NULL;
    mlastLong = NULL;
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
    Position pos;
    Accuracy acc;
    LSError mLSError;
    jvalue_ref serviceObj = NULL;
    TrakingErrorCode errorCode = LOCATION_SUCCESS;
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

        if (strcmp(handlerName, GPS) == NULL)
            sel_handler = LunaCriteriaCategoryHandler::CRITERIA_GPS;
        else if (strcmp(handlerName, NETWORK) == NULL)
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

    if (enableHandlers(sel_handler, sub_key_gps, sub_key_nw, &startedHandlers)) {
        LS_LOG_DEBUG("startedHandlers %d sub_key_gps %s sub_key_nw %s",
                     startedHandlers, sub_key_gps, sub_key_nw);

        long long reqTime = time(0);
        reqTime *= 1000;
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

        if (strcmp(sub_key_gps, GPS_CRITERIA_KEY) == NULL) {
            bRetVal = LSSubscriptionAdd(sh, sub_key_gps, message, &mLSError);

            if (bRetVal == false) {

                LSErrorPrintAndFree(&mLSError);
                errorCode = LOCATION_UNKNOWN_ERROR;
                goto EXIT;
            }

            handler_start_tracking_criteria((Handler *) handler_array[HANDLER_GPS],
                                            START,
                                            start_tracking_criteria_cb,
                                            NULL,
                                            HANDLER_GPS,
                                            sh);

            trackhandlerstate = startedHandlers;
        }

        if (strcmp(sub_key_nw, NW_CRITERIA_KEY) == NULL) {
            bRetVal = LSSubscriptionAdd(sh, sub_key_nw, message, &mLSError);

            if (bRetVal == false) {
                LSErrorPrintAndFree(&mLSError);
                errorCode = LOCATION_UNKNOWN_ERROR;
                goto EXIT;
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
        LSMessageReplyError(sh,
                            message,
                            errorCode,
                            LocationService::getInstance()->positionErrorText(errorCode));
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
            if (enableGpsHandler(startedHandlers)) {
                strcpy(sub_key_gps, GPS_CRITERIA_KEY);
            }

            if (enableNwHandler(startedHandlers)) {
                strcpy(sub_key_nw, NW_CRITERIA_KEY);
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
    bool ret;

    if (LocationService::getInstance()->getHandlerStatus(NETWORK)) {
        LS_LOG_DEBUG("network is on in Settings");

        if ((LocationService::getInstance()->getWifiState() == true) && (LocationService::getInstance()->getConnectionManagerState()
            || LocationService::getInstance()->getWifiInternetState())) { //wifistate == true)
            ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_NW]),
                                HANDLER_WIFI);

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
                                HANDLER_CELLID);

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
    bool ret;

    if (LocationService::getInstance()->getHandlerStatus(GPS)) {
        LS_LOG_DEBUG("gps is on in Settings");
        ret = handler_start(HANDLER_INTERFACE(handler_array[HANDLER_GPS]),
                            HANDLER_GPS);

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
    char *key = NULL;
    LSError mLSError;
    LSErrorInit(&mLSError);
    bool bRetVal;
    char *errorString = NULL;
    jvalue_ref serviceObject = NULL;

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {

        LS_LOG_ERROR("Out of memory");
        errorString = g_strdup_printf("{\"returnValue\":false,\"errorText\":\"%s\",\"errorCode\":%d}",
                                      "Out of memory", LOCATION_OUT_OF_MEM);

        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           key,
                                                           errorString,
                                                           &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmLgeSrvHandle,
                                                           key,
                                                           errorString,
                                                           &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        g_free(errorString);

        return;
    }

    if (error == ERROR_NONE) {
        location_util_form_json_reply(&serviceObject, true, SUCCESSFUL);
        location_util_add_pos_json(&serviceObject, pos);
        location_util_add_acc_json(&serviceObject,accuracy);

        if (type == HANDLER_GPS)
            key = GPS_CRITERIA_KEY;
        else
            key = NW_CRITERIA_KEY;

        LS_LOG_DEBUG("key %s", key);

        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmSrvHandle,
                                                           key,
                                                           jvalue_tostring_simple(serviceObject),
                                                           &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

        bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                           mpalmLgeSrvHandle,
                                                           key,
                                                           jvalue_tostring_simple(serviceObject),
                                                           &mLSError);

        if (bRetVal == false)
            LSErrorPrintAndFree(&mLSError);

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
            location_util_form_json_reply(&serviceObject, false, LOCATION_TIME_OUT);
            location_util_add_errorText_json(&serviceObject, LocationService::getInstance()->positionErrorText(LOCATION_TIME_OUT));

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmSrvHandle,
                                                               GPS_CRITERIA_KEY,
                                                               jvalue_tostring_simple(serviceObject),
                                                               &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmLgeSrvHandle,
                                                               GPS_CRITERIA_KEY,
                                                               jvalue_tostring_simple(serviceObject),
                                                               &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmSrvHandle,
                                                               NW_CRITERIA_KEY,
                                                               jvalue_tostring_simple(serviceObject),
                                                               &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);

            bRetVal = LSSubscriptionNonSubMeetsCriteriaRespond(pos,
                                                               mpalmLgeSrvHandle,
                                                               NW_CRITERIA_KEY,
                                                               jvalue_tostring_simple(serviceObject),
                                                               &mLSError);

            if (bRetVal == false)
                LSErrorPrintAndFree(&mLSError);
        }
    }

    j_release(&serviceObject);
}

bool LunaCriteriaCategoryHandler::LSSubscriptionNonSubMeetsCriteriaRespond(Position *pos,
                                                                           LSPalmService *psh,
                                                                           const char *key,
                                                                           const char *payload,
                                                                           LSError *lserror)
{
    LSHandle *public_bus = LSPalmServiceGetPublicConnection(psh);
    LSHandle *private_bus = LSPalmServiceGetPrivateConnection(psh);
    bool retVal = LSSubscriptionNonMeetsCriteriaReply(pos, public_bus, key, payload, lserror);

    if (retVal == false)
        return retVal;

    retVal = LSSubscriptionNonMeetsCriteriaReply(pos, private_bus, key, payload, lserror);

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
    bool isNonSubscibePresent = false;
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    jschema_ref input_schema = NULL ;
    JSchemaInfo schemaInfo;
    int minDist;
    int minInterval;
    LS_LOG_DEBUG("key = %s", key);

    retVal = LSSubscriptionAcquire(sh, key, &iter, lserror);

    if (retVal == true && LSSubscriptionHasNext(iter)) {
        do {
            LSMessage *msg = LSSubscriptionNext(iter);
            //parse message to get minDistance
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

                if (meetsCriteria(msg, pos, minInterval, NULL, true, false)) {

                    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("minimumDistance"), &jsonSubObject)) {
                        jnumber_get_i32(jsonSubObject, &minDist);

                        if (meetsCriteria(msg, pos, NULL, minDist, false, true)) {
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

                    if (meetsCriteria(msg, pos, NULL, minDist, false, true))
                        LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                } else {
                    //No critria, just reply
                    LSMessageReplyCriteria(msg, pos, sh, key, payload, iter, lserror);
                }
            }

            j_release(&parsedObj);
          } while (LSSubscriptionHasNext(iter));
    }
    LSSubscriptionRelease(iter);
    jschema_release(&input_schema);

    if (LocationService::getInstance()->isSubscListFilled(sh, key, false) == false)
        LocationService::getInstance()->stopNonSubcription(key);

    return retVal;
}

void LunaCriteriaCategoryHandler::LSMessageReplyCriteria(LSMessage *msg,
                                                         Position *pos,
                                                         LSHandle *sh,
                                                         const char *key,
                                                         const char *payload,
                                                         LSSubscriptionIter *iter,
                                                         LSError *lserror)
{
    bool retVal;

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
    }
}

/* Vincenty formula. WGS-84 */
//minDistance(minDistance,pos->latitude,pos->latitude,mlastLat,mlastLong)
bool LunaCriteriaCategoryHandler::minDistance(int minimumDistance, double latitude1, double longitude1, double latitude2 , double longitude2)
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
        LSMessageReplyError(sh,
                            message,
                            LOCATION_INVALID_INPUT,
                            LocationService::getInstance()->positionErrorText(LOCATION_INVALID_INPUT));

        j_release(&parsedObj);
        return true;
    }

    if (handler == NULL) {
        LS_LOG_ERROR("ParamInput is NULL");
        LSMessageReplyError(sh,
                            message,
                            LOCATION_INVALID_INPUT,
                            LocationService::getInstance()->positionErrorText(LOCATION_INVALID_INPUT));

        j_release(&parsedObj);
        return true;
    }

    serviceObject = jobject_create();

    if (jis_null(serviceObject)) {
        LS_LOG_ERROR("Failed to allocate memory to serviceObject");
        LSMessageReplyError(sh,
                            message,
                            LOCATION_OUT_OF_MEM,
                            LocationService::getInstance()->positionErrorText(LOCATION_OUT_OF_MEM));

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
        LSMessageReplyError(sh,
                            message,
                            LOCATION_INVALID_INPUT,
                            LocationService::getInstance()->positionErrorText(LOCATION_INVALID_INPUT));
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
                    long long currentTime = time(0);
                    currentTime *= 1000;
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

                    if (isFirst || minDistance(minDist, pos->latitude, pos->longitude, lastLat, lastLong)){
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
