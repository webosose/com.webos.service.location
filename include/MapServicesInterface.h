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


#ifndef H_MapServicesInterface
#define H_MapServicesInterface


#include <iostream>
#include <GeoLocation.h>
#include <GeoAddress.h>
#include <location_errors.h>
#include <luna-service2/lunaservice.h>
#include <loc_log.h>
#include <functional>

typedef std::function<void(GeoLocation&, int, LSMessage *)> GeoCodeCb;
typedef std::function<void(GeoAddress&, int, LSMessage *)> ReverseGeoCodeCb;

class MapServicesInterface {
public:

    MapServicesInterface() {
    }

    virtual ~MapServicesInterface() {
    }

    virtual ErrorCodes geoCode(GeoAddress& address, GeoCodeCb geocodeCallback, bool isSync, LSMessage *message) {
        LS_LOG_ERROR("No support for geocode");
        return ERROR_NOT_IMPLEMENTED;
    }

    virtual ErrorCodes reverseGeoCode(GeoLocation& geolocation, ReverseGeoCodeCb revGecodeCallback, bool isSync,
                                      LSMessage *message) {
        LS_LOG_ERROR("No support for reverseGeoCode");
        return ERROR_NOT_IMPLEMENTED;
    }

public:

    GeoCodeCb mGeoCodeCb = nullptr;
    ReverseGeoCodeCb mRevGeoCodeCb = nullptr;

};

#endif  //H_MapServicesInterface
