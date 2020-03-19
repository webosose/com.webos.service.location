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


#include <WSPConfigurationFileParser.h>
#include <loc_log.h>

const std::string WSP_NAME = "wpsName";
const std::string API_KEY = "apiKey";
const std::string SERVICES = "services";
const std::string FEATURES ="features";

WSPConfigurationFileParser::WSPConfigurationFileParser(std::string confFilPath)
    : filePath(confFilPath) {
    LS_LOG_DEBUG("===WSPConfigurationFileParser Ctor====");
}

WSPConfigurationFileParser::~WSPConfigurationFileParser() {
    LS_LOG_DEBUG("===WSPConfigurationFileParser Dtor====");
}

WspConfStateCode WSPConfigurationFileParser::parseFile() {
    LS_LOG_DEBUG("filePath: %s \n", filePath.c_str());
    pbnjson::JValue root = pbnjson::JDomParser::fromFile(filePath.c_str());

    if (root.isNull()) {
        LS_LOG_DEBUG("Failed to load JSON object from file: %s\n",filePath.c_str());
        return WSP_CONF_READ_FAILED;
    }

    if (!root.isValid()) {
        LS_LOG_DEBUG("JSON is not valid from file: %s\n",filePath.c_str());
        return WSP_CONF_READ_FAILED;
    }

    if (root.hasKey(WSP_NAME)) {
        wspName = root[WSP_NAME].asString();
        if (wspName.empty())
            return WSP_CONF_NAME_MISSING;
    } else {
        return WSP_CONF_NAME_MISSING;
    }

    if (root.hasKey(API_KEY)) {
        apiKey = root[API_KEY].asString();
        if (apiKey.empty())
            return WSP_CONF_APIKEY_MISSING;
    } else {
        return WSP_CONF_APIKEY_MISSING;
    }

    if (root.hasKey(SERVICES)) {
        pbnjson::JValue supportedServices = root[SERVICES];

        if (!supportedServices.isArray() || (0 == supportedServices.arraySize()))
        {
            LS_LOG_DEBUG("Supported Services List not present \n");
            return WSP_CONF_NO_SERVICES;
        }

        for (int i = 0; i < supportedServices.arraySize(); i++) {
            std::string url;
            if (supportedServices[i].hasKey("url")) {
                url = supportedServices[i]["url"].asString();
                if (url.empty())
                    return WSP_CONF_URL_MISSING;
            } else {
                return WSP_CONF_URL_MISSING;
            }

            if (supportedServices[i].hasKey(FEATURES)) {
                pbnjson::JValue featuresList = supportedServices[i][FEATURES];
                if (!featuresList.isArray() || (0 == featuresList.arraySize())) {
                    LS_LOG_DEBUG("Supported FEATURES not present \n");
                    return WSP_CONF_NO_FEATURES;
                }

                for (int fl = 0; fl < featuresList.arraySize(); fl++) {
                    std::string featureName = featuresList[fl].asString();
                    if (featureName.empty())
                        return WSP_CONF_NO_FEATURES;
                    mSupportedFeaturesKeyUrlMap[featureName] = url;
                }
            } else {
                return WSP_CONF_NO_FEATURES;
            }
        }
    } else {
        return WSP_CONF_NO_SERVICES;
    }

    if (mSupportedFeaturesKeyUrlMap.size()) {
        for (auto suppFeatures : mSupportedFeaturesKeyUrlMap) {
            if (suppFeatures.first == "geocode") {
                registerFeature(GEO_CODE, "geocode");
            } else if (suppFeatures.first == "reverseGeocode")
                registerFeature(REVERSE_GEOCODE, "reverseGeocode");
            else if (suppFeatures.first == "search")
                registerFeature(SEARCH, "search");
            else if (suppFeatures.first == "timezone")
                registerFeature(TIMEZONE, "timezone");
            else if (suppFeatures.first == "distance")
                registerFeature(DISTANCE, "distance");
            else
                LS_LOG_DEBUG("Non supported feature %s listed", suppFeatures.first.c_str());
        }
    }

    return WSP_CONF_SUCCESS;
}
