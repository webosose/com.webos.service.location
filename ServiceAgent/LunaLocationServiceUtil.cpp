/**
 @file      LunaLocationService_Util.cpp
 @brief     Utility File to handle error conditions while parsing JSON objects recieved
 Null Checks on memory allocations, variables recieved in callbacks

 @author    Sameer
 @date      2013-03-22
 @version   TDB
 @todo      TDB

 @copyright Copyright (c) 2012 LG Electronics.  All rights reserved.
 */

#include "LunaLocationServiceUtil.h"
#include "ServiceAgent.h"
#include <LocationService_Log.h>

typedef struct locErrorTextPair {
    LocationErrorCode code;
    const char *text;
} locErrorTextPair_t;

static const locErrorTextPair_t mapLocErrorText[LOCATION_ERROR_MAX] = {
    {LOCATION_SUCCESS,                      "Success"},
    {LOCATION_TIME_OUT,                     "Time out"},
    {LOCATION_POS_NOT_AVAILABLE,            "Position not available"},
    {LOCATION_UNKNOWN_ERROR,                "Unknown error"},
    {LOCATION_GPS_UNAVAILABLE,              "Gps not supported"},
    {LOCATION_LOCATION_OFF,                 "Location source is off"},
    {LOCATION_PENDING_REQ,                  "Request pending"},
    {LOCATION_APP_BLACK_LISTED,             "Application is blacklisted"},
    {LOCATION_START_FAILURE,                "Handler start failure"},
    {LOCATION_STATE_UNKNOWN,                "State unknown"},
    {LOCATION_INVALID_INPUT,                "Invalid input"},
    {LOCATION_DATA_CONNECTION_OFF,          "No internet connection"},
    {LOCATION_WIFI_CONNECTION_OFF,          "Wifi is not turned on"},
    {LOCATION_OUT_OF_MEM,                   "Out of memory"}
};

char *locationErrorReply[LOCATION_ERROR_MAX] = {0};



bool LSMessageInitErrorReply()
{
    int i, j;

    LSMessageReleaseErrorReply();

    for (i=0; i<LOCATION_ERROR_MAX; i++) {
        locationErrorReply[i] = g_strdup_printf("{\"returnValue\":true,\"errorCode\":%d,\"errorText\":\"%s\"}",
                                                mapLocErrorText[i].code,
                                                mapLocErrorText[i].text);

        if (locationErrorReply[i] == NULL) {
            //release all previous allocated memory in array
            for (j = i-1; j >= 0; j--) {
                g_free(locationErrorReply[j]);
                locationErrorReply[j] = NULL;
            }

            return false;
        }
    }

    return true;
}

void LSMessageReleaseErrorReply()
{
    int i;

    for (i=0; i<LOCATION_ERROR_MAX; i++) {
        if (locationErrorReply[i]) {
            g_free(locationErrorReply[i]);
            locationErrorReply[i] = NULL;
        }
    }
}

char *LSMessageGetErrorReply(int errorCode)
{
    if (errorCode < 0 || errorCode >= LOCATION_ERROR_MAX)
        return NULL;

    return locationErrorReply[errorCode];
}

void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode)
{
    LSError lserror;

    if (locationErrorReply[errorCode] == NULL)
        return;

    LSErrorInit(&lserror);

    if (!LSMessageReply(sh, message, locationErrorReply[errorCode], &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

/*void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode, char *errorText)
{
    LSError lserror;
    LSErrorInit(&lserror);
    char *errorString;

    errorString = g_strdup_printf("{\"returnValue\":true,\"errorText\":\"%s\",\"errorCode\":%d}", errorText, errorCode);

    bool retVal = LSMessageReply(sh, message, errorString, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    g_free(errorString);
}*/

bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message)
{
    const char *subscribe_answer = "{\"returnValue\":true, \"subscribed\":true}";
    LSError mLSError;
    LSErrorInit(&mLSError);

    if (LSMessageReply(sh, message, subscribe_answer, &mLSError) == false) {
        LSErrorPrint(&mLSError, stderr);
        LSErrorFree(&mLSError);
        return false;
    }

    return true;
}

void LSMessageReplySuccess(LSHandle *sh, LSMessage *message)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool retVal = LSMessageReply(sh, message, "{\"returnValue\":true}", &lserror);

    if (retVal == false) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool LSMessageValidateSchema(LSHandle *sh, LSMessage *message,const char *schema, jvalue_ref *parsedObj)
{
    bool ret = false;
    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer(schema), DOMOPT_NOOPT, NULL);

    if (!input_schema)
        return false;

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    *parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(*parsedObj)) {
        input_schema = jschema_parse (j_cstr_to_buffer(SCHEMA_ANY), DOMOPT_NOOPT, NULL);
        jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
        *parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

        if(!jis_null(*parsedObj))
           j_release(parsedObj);

        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
    } else {
        ret = true;
    }

    jschema_release(&input_schema);

    return ret;
}
