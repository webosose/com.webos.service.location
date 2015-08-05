/*
 * Copyright (c) 2012-2015 LG Electronics Inc. All Rights Reserved.
 */

#ifndef _LUNALBSREQUEST_H_
#define _LUNALBSREQUEST_H_

#include <lunaservice.h>
#include <string>

class LunaLbsRequest {
public:
    LunaLbsRequest() {}

    LunaLbsRequest(LSMessage* message, LSHandle* sh, char* requestString): mMessage(message), mSh(sh), mRequestString(requestString) {

    }

    LSMessage* getMessage() const {
        return mMessage;
    }
    LSHandle* getHandle() const {
        return mSh;
    }
    std::string getRequestString() const {
        return mRequestString;
    }

private:
    LSMessage* mMessage;
    LSHandle* mSh;
    std::string  mRequestString;
};

#endif