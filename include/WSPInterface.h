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


#if !defined(_WSPINTERFACE_H)
#define _WSPINTERFACE_H


#include <GeoCodeInterface.h>
#include <vector>
#include <array>
#include <location_errors.h>

typedef enum {
    NO_FEATURE_SUPPORTED,
    GEO_CODE,
    REVERSE_GEOCODE,
    SEARCH,
    TIMEZONE,
    DISTANCE,
    MAX
} FeatureType;

typedef std::array<FeatureType, FeatureType::MAX> SupportedFeatureList;

class WSPInterface {

public:

    virtual void publishFeatures() {
    }

    WSPInterface(std::string wspId) {
        mWspId = wspId;
     mSupportedFeatureList.fill(NO_FEATURE_SUPPORTED);
    }

    virtual ~WSPInterface() {
    }

    virtual GeoCodeInterface* getGeocodeImpl() {
        return nullptr;
    }

    void registerFeature(FeatureType feature) {
        mSupportedFeatureList[feature] = feature;
    }

    SupportedFeatureList getSupportedFeatures() {
        return mSupportedFeatureList;
    }

    bool isRegistered(FeatureType featureType) {
        return mSupportedFeatureList[featureType];
    }

    std::string getName() const {
        return mWspId;
    }

private:

    std::string mWspId;
    SupportedFeatureList mSupportedFeatureList;

};

#endif  //_WSPINTERFACE_H
