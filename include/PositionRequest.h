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


#ifndef POSITIONREQUEST_H_
#define POSITIONREQUEST_H_

#include <string>
#include <location_errors.h>
#include <GeoLocation.h>
#include <HandlerRequestCommands.h>

class PositionRequest {

private:
    typedef std::string ProviderID_t;
    typedef int PositionInterval_t;
    typedef std::string RequestParams_t;

    ProviderID_t mProviderId;
    HandlerRequestType mRequestType;
    RequestParams_t mRequestParams;
    int mGeofenceID;
    double mGeofenceLatitude;
    double mGeofenceRadius;
    double mGeofenceLongitude;

public:
    PositionRequest(): mRequestType(UNKNOWN_CMD), mGeofenceID(0),
                       mGeofenceLatitude(0.0), mGeofenceRadius(0.0),
                       mGeofenceLongitude(0.0) {
    }

    PositionRequest(ProviderID_t providerId, HandlerRequestType requestType):
                  mProviderId(providerId), mRequestType(requestType) {
    }

    const std::string& getProviderId() const {
    return mProviderId;
    }

    void setProviderId(const std::string& providerId) {
    mProviderId = providerId;
    }

    ~PositionRequest() {};

    const std::string& getRequestParams() const {
        return mRequestParams;
    }

    void setRequestParams(const std::string requestParams) {
        mRequestParams = requestParams;
    }

    void printRequest() {
        LS_LOG_INFO("===>position-request: \n");
        LS_LOG_INFO("[mProvider_id]: %s\n", mProviderId.c_str());
    }
    HandlerRequestType getRequestType() const {
    return mRequestType;
    }

    void setRequestType(HandlerRequestType requestType) {
        mRequestType = requestType;
    }

    void setGeofenceID (int geofenceId){
        mGeofenceID = geofenceId;
        }

    int getGeofenceID (){
        return mGeofenceID;
    }

    void setGeofenceCoordinates (double longitude, double latitude, double radius){
        this->mGeofenceLongitude = longitude;
        this->mGeofenceLatitude = latitude;
        this->mGeofenceRadius = radius;
    }

    double getGeofenceLongitude (){
        return mGeofenceLongitude;
    }

    double getGeofenceLatitude (){
        return mGeofenceLatitude;
    }

    double getGeofenceRadius (){
        return mGeofenceRadius;
    }
};

#endif /* POSITIONREQUEST_H_ */
