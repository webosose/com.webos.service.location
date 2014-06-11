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

bool LSMessageValidateSchema(LSHandle *sh, LSMessage *message,const char *schema, jvalue_ref *parsedObj)  {

    bool ret = false;
    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer(schema), DOMOPT_NOOPT, NULL);

    if(!input_schema)
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

        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT, "Invalid input");
    }
    else
      ret = true;

    jschema_release(&input_schema);
    return ret;
}

void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode, char *errorText)
{
    LSError lserror;
    LSErrorInit(&lserror);
    char *errorString;

    errorString = g_strdup_printf("{\"returnValue\":false,\"errorText\":\"%s\",\"errorCode\":%d}", errorText, errorCode);

    bool retVal = LSMessageReply(sh, message, errorString, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    g_free(errorString);
}

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
