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


#include "LunaLocationService_Util.h"
#include "ServiceAgent.h"

void LSMessageReplyErrorInvalidHandle(LSHandle *sh, LSMessage *message)
{
    LSError lserror;
    char mResult[50];
    LSErrorInit(&lserror);

    sprintf(mResult, "{\"returnValue\":true, \"errorCode\":%d}", TRACKING_UNKNOWN_ERROR);
    bool retVal = LSMessageReply(sh, message, mResult, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

void LSMessageReplyErrorNullPointer(LSHandle *sh, LSMessage *message)
{
    LSError lserror;
    char mResult[50];
    LSErrorInit(&lserror);
    sprintf(mResult, "{\"returnValue\":true, \"errorCode\":%d}", TRACKING_UNKNOWN_ERROR);
    bool retVal = LSMessageReply(sh, message, mResult, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

void LSMessageReplyErrorInvalidJSON(LSHandle *sh, LSMessage *message)
{
    LSError lserror;
    char mResult[50];
    LSErrorInit(&lserror);

    sprintf(mResult, "{\"returnValue\":true, \"errorCode\":%d}", TRACKING_UNKNOWN_ERROR);
    bool retVal = LSMessageReply(sh, message, mResult, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool LSMessageReplyError(LSHandle *sh, LSMessage *message,int errorCode)
{
    LSError lserror;
    char mResult[50];
    LSErrorInit(&lserror);


    sprintf(mResult, "{\"returnValue\":true, \"errorCode\":%d}", errorCode);
    bool retVal = LSMessageReply(sh, message, mResult, &lserror);
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    return true;
}

bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message)
{
    const char *subscribe_answer = "{\"returnValue\":true, \"subscribed\":true}";
    LSError mLSError;
    LSErrorInit(&mLSError);

    if(!LSMessageReply(sh, message, subscribe_answer, &mLSError)){
        LSErrorPrint (&mLSError, stderr);
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
    if (!retVal)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}
