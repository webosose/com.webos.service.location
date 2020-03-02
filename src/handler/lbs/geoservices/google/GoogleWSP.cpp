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


#include <GoogleWSP.h>
#include <LBSEngine.h>

#define GOOGLEWSPNAME  "google"

bool GoogleWSP::mRegistered = LBSEngine::getInstance()->registerWebServiceProvider(std::string(GOOGLEWSPNAME),
                                                                                   new(std::nothrow) GoogleWSP(
                                                                                           GOOGLEWSPNAME));

GeoCodeInterface *GoogleWSP::getGeocodeImpl() {
    static GoogleGeoImpl geoCodeImpl;
    return &geoCodeImpl;
}

void GoogleWSP::publishFeatures() {
    this->registerFeature(REVERSE_GEOCODE);
    this->registerFeature(GEO_CODE);
}

