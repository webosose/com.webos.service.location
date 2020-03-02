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

#if !defined(_GOOGLEGEOIMPL_H)
#define _GOOGLEGEOIMPL_H

#include <memory>
#include <algorithm>
#include <string.h>
#include <GeoCodeInterface.h>
#include <loc_http.h>
#include <loc_security.h>
#include <WSPInterface.h>
#include <HttpInterface.h>
#include <NetworkRequestManager.h>
#include <loc_log.h>

#define GOOGLE_LBS_URL             https:\/\/maps.googleapis.com
#define GOOGLE_SUB_URL_TO_SIGN     /maps/api/geocode/json?
#define GOOGLE_CLIENT_KEY          &client=gme-lgelectronics1
#define GEOCODEKEY_CONFIG_PATH      "/etc/geocode.conf"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


class GoogleGeoImpl : public GeoCodeInterface, public HttpInterface {

public:

    GoogleGeoImpl();

    ~GoogleGeoImpl();

    ErrorCodes geoCode(GeoAddress address, GeoCodeCb geocodeCallback, bool isSync, LSMessage *message);

    ErrorCodes reverseGeoCode(GeoLocation geolocation, ReverseGeoCodeCb revGeocodeCallback, bool isSync,
                              LSMessage *message);

    ErrorCodes lbsPostQuery(std::string url, bool isSync, LSMessage *message);

    std::string formatUrl(std::string address, const char *key);

    char *readApiKey();

    void handleResponse(HttpReqTask *task);

private:

    std::string mGoogleGeoCodeApiKey;
};

#endif  //_GOOGLEGEOIMPL_H
