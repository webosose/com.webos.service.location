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


#ifndef H_WSPConfigurationFileParser
#define H_WSPConfigurationFileParser

#include <iostream>
#include <unordered_map>
#include <pbnjson.h>
#include <pbnjson.hpp>
#include <ServiceAgent.h>

typedef enum {
    NO_FEATURE_SUPPORTED,
    GEO_CODE,
    REVERSE_GEOCODE,
    SEARCH,
    TIMEZONE,
    DISTANCE,
    MAX
} FeatureType;

typedef std::unordered_map<FeatureType, std::string> SupportedFeatureList;
typedef std::unordered_map<std::string, std::string> SupportedFeaturesKeyUrlMap;

class WSPConfigurationFileParser {
public:
    WSPConfigurationFileParser(std::string confFilPath);
    ~WSPConfigurationFileParser();

    std::string getWSPname() { return wspName;}
    std::string getKey() { return apiKey;}

    std::string getUrl(FeatureType featureType) {
	return mSupportedFeaturesKeyUrlMap[mSupportedFeatureList[featureType]];
    }

    WspConfStateCode parseFile();

    bool isSupported(FeatureType featureType) {
        return (mSupportedFeatureList[featureType]).size();
    }

private:
    std::string filePath;
    std::string wspName;
    std::string apiKey;
    std::string url;

    SupportedFeaturesKeyUrlMap mSupportedFeaturesKeyUrlMap;
    SupportedFeatureList mSupportedFeatureList;

    void registerFeature (FeatureType eFeature, std::string sFeature) {
        mSupportedFeatureList[eFeature] = sFeature;
    }
};

#endif  //H_WSPConfigurationFileParser
