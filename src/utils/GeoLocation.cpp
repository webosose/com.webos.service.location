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


#include <GeoLocation.h>
#include <loc_log.h>
#include <cmath>

GeoLocation::GeoLocation() :
        mLatitude(0.0), mLongitude(0.0), mAltitude(0.0), mHorAccuracy(0.0),
        mTimeStamp(0.0), mDirection(0.0), mClimb(0.0), mSpeed(0.0), mVertAccuracy(0.0) {

}
GeoLocation::GeoLocation(std::string slocation) :
        mLatitude(0), mLongitude(0), mAltitude(0), mHorAccuracy(0),
        mTimeStamp(0.0), mDirection(0.0), mClimb(0.0), mSpeed(0.0), mVertAccuracy(0.0) {
    this->mStrLocation = slocation;
}
bool GeoLocation::operator==(const GeoLocation &location) const {
    bool latitudeEq = (std::isnan(mLatitude) && std::isnan(location.mLatitude));
    bool longitudeEq = (std::isnan(mLongitude)
            && std::isnan(location.mLongitude));
    bool altitudeEq = (std::isnan(mAltitude) && std::isnan(location.mAltitude));

    return (latitudeEq && longitudeEq && altitudeEq);
}

GeoLocation::GeoLocation(double latitude, double longitude, double altitude, double accuracy,
                                             double timestamp, double direction, double climb, double speed, double vertAcc) :
        mLatitude(latitude), mLongitude(longitude), mAltitude(altitude), mHorAccuracy(accuracy),
     mTimeStamp(timestamp), mDirection(direction), mClimb(climb), mSpeed(speed), mVertAccuracy(vertAcc) {

}

GeoLocation::~GeoLocation() {
}

double GeoLocation::getLatitude() const {
    return mLatitude;
}

double GeoLocation::getLongitude() const {
    return mLongitude;
}
std::string GeoLocation::toString() const {
    return mStrLocation;
}
double GeoLocation::getAltitude() const {
    return mAltitude;
}

double GeoLocation::getTimeStamp() const  {
    return mTimeStamp;
}

double GeoLocation::getSpeed() const  {
    return mSpeed;
}

double GeoLocation::getDirection() const  {
    return mDirection;
}

double GeoLocation::getClimb() const  {
    return mClimb;
}

double GeoLocation::getVerticalAccuracy() const  {
    return mVertAccuracy;
}

double GeoLocation::getHorizontalAccuracy() const  {
    return mHorAccuracy;
}

void GeoLocation::setLatitude(double latitude) {
    mLatitude = latitude;
}

void GeoLocation::setLongitude(double longitude) {
    mLongitude = longitude;
}

void GeoLocation::setAltitude(double altitude) {
    mAltitude = altitude;
}

void GeoLocation::setHorizontalAccuracy(double horAccuracy) {
    mHorAccuracy = horAccuracy;
}

void GeoLocation::setVerticalAccuracy(double vertAccuracy) {
    mVertAccuracy = vertAccuracy;
}
void GeoLocation::printLocation() {
    LS_LOG_DEBUG("-->POSITION-INFO<---");
    LS_LOG_DEBUG("mProviderId    => %s", getProviderId().c_str());
    LS_LOG_DEBUG("mLatitude    => %f", getLatitude());
    LS_LOG_DEBUG("mLongitude    => %f", getLongitude());
    LS_LOG_DEBUG("mAltitude    => %f", getAltitude());
    LS_LOG_DEBUG("-->------------<---");
}

const std::string& GeoLocation::getProviderId() const {
    return mProviderId;
}

void GeoLocation::setProviderId(const std::string& providerId) {
    mProviderId = providerId;
}
