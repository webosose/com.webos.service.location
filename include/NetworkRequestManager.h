// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


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
#include <luna-service2/lunaservice.h>


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
