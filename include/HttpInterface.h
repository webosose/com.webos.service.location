// @@@LICENSE
//
//      Copyright (c) 2015 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef __HTTP_INTERFACE__
#define __HTTP_INTERFACE__

#include "loc_http.h"
//class HttpReqTask;
/*This interface is necessary to be implemented by teh clients of HTTPEngine*/

class HttpInterface {
public:
    virtual void handleResponse(HttpReqTask *task) = 0;

};

#endif
