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


#ifndef H_MapServicesImpl
#define H_MapServicesImpl


#include <memory>
#include <algorithm>
#include <string.h>
#include <loc_http.h>
#include <loc_security.h>
#include <HttpInterface.h>
#include <MapServicesInterface.h>
#include <NetworkRequestManager.h>
#include <loc_log.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

class WSPConfigurationFileParser;

class MapServicesImpl : public MapServicesInterface, public HttpInterface {
    WSPConfigurationFileParser *configurationData;

public:
    MapServicesImpl(WSPConfigurationFileParser* confData);

    virtual ~MapServicesImpl();

    ErrorCodes geoCode(GeoAddress& address, GeoCodeCb geocodeCallback, bool isSync, LSMessage *message);
    ErrorCodes reverseGeoCode(GeoLocation& geolocation, ReverseGeoCodeCb revGeocodeCallback, bool isSync,
                              LSMessage *message);

    ErrorCodes lbsPostQuery(std::string url, bool isSync, LSMessage *message);
    std::string formatUrl(std::string address, std::string url, const char *key);
    void handleResponse(HttpReqTask *task);
};

#endif  //H_MapServicesImpl
