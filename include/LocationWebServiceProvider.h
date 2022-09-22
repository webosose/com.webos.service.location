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


#ifndef H_LocationWebServiceProvider
#define H_LocationWebServiceProvider

#include <loc_log.h>
#include <MapServicesInterface.h>
#include <ServiceAgent.h>

class WSPConfigurationFileParser;

class LocationWebServiceProvider {
public:
    static LocationWebServiceProvider *getInstance() {
        static LocationWebServiceProvider LocationWebServiceProviderObject;
        return &LocationWebServiceProviderObject;
    }

    MapServicesInterface* getGeocodeImpl();

   ~LocationWebServiceProvider();

    std::string getWSPName() const {
        return mWspId;
    }

    LocationErrorCode readWSPConfiguration();

private:
    LocationWebServiceProvider();
    LocationWebServiceProvider(const LocationWebServiceProvider &rhs) = delete;
    LocationWebServiceProvider &operator=(const LocationWebServiceProvider &rhs) = delete;

    WSPConfigurationFileParser* confFile;

    std::string mWspId;
};

#endif  //  H_LocationWebServiceProvider

