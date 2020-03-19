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

#include <LocationWebServiceProvider.h>
#include <MapServicesImpl.h>
#include <WSPConfigurationFileParser.h>

const std::string mapServiceConfPath = "/etc/location/wsp.conf";

LocationWebServiceProvider::LocationWebServiceProvider()
    : confFile (nullptr) {
    LS_LOG_DEBUG("Ctor of LocationWebServiceProvider");
}

LocationWebServiceProvider::~LocationWebServiceProvider() {
    LS_LOG_DEBUG("Dtor of LocationWebServiceProvider");
    if (confFile) {
        delete confFile;
        confFile = nullptr;
    }
}

MapServicesInterface *LocationWebServiceProvider::getGeocodeImpl() {
    static MapServicesImpl mapServicesImpl(confFile);
    return &mapServicesImpl;
}

LocationErrorCode LocationWebServiceProvider::readWSPConfiguration() {

    LocationErrorCode wspError = LOCATION_SUCCESS;
    WspConfStateCode parserError = WSP_CONF_SUCCESS;

    if (!confFile) {
        confFile = new(std::nothrow) WSPConfigurationFileParser(mapServiceConfPath);

         parserError = confFile->parseFile();

        if (parserError != WSP_CONF_SUCCESS) {
            delete confFile;
            confFile = nullptr;
        }
    }

    switch (parserError) {
        case WSP_CONF_SUCCESS:
            wspError = LOCATION_SUCCESS;
            break;

        case WSP_CONF_READ_FAILED:
            wspError = LOCATION_WSP_CONF_READ_FAILED;
            break;

        case WSP_CONF_NAME_MISSING:
            wspError = LOCATION_WSP_CONF_NAME_MISSING;
            break;

        case WSP_CONF_APIKEY_MISSING:
            wspError = LOCATION_WSP_CONF_APIKEY_MISSING;
            break;

        case WSP_CONF_NO_SERVICES:
            wspError = LOCATION_WSP_CONF_NO_SERVICES;
            break;

        case WSP_CONF_NO_FEATURES:
            wspError = LOCATION_WSP_CONF_NO_FEATURES;
            break;

        case WSP_CONF_URL_MISSING:
            wspError = LOCATION_WSP_CONF_URL_MISSING;
            break;

        default:
            wspError = LOCATION_UNKNOWN_ERROR;
            break;
    }

    return wspError;
}
