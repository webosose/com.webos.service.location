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

#ifndef __NETWORK_MANAGER__
#define __NETWORK_MANAGER__

#include <string.h>
#include <bitset>
#include <mutex>
#include <iostream>
#include <unordered_map>
#include <HttpInterface.h>
#include <NetworkRequestManagerDefines.h>
#include <location_errors.h>
#include <lunaservice.h>


class NetworkRequestManager {

public:

    static NetworkRequestManager *getInstance() {
        static NetworkRequestManager netMgrReq;
        return &netMgrReq;
    }

    ~NetworkRequestManager();

    void init();

    void deInit();

    ErrorCodes initiateTransaction(const char **headers, int size, std::string url, bool isSync, LSMessage *message,
                                   HttpInterface *client, char *post_data = NULL);

    void cancelTransaction(HttpReqTask *task);

    void clearTransaction(HttpReqTask *task);

    //callback from loc_http
    static void handleDataCb(HttpReqTask *task, void *user_data);

private:

    NetworkRequestManager();

    NetworkRequestManager(const NetworkRequestManager &rhs) = delete; // prevent copy construction
    NetworkRequestManager &operator=(const NetworkRequestManager &rhs) = delete; // prevent assignment operation

private:

    std::bitset<sizeof(unsigned long)> _attributes;
    std::unordered_map<HttpReqTask *, HttpInterface *> httpRequestList;

};

#endif
