// @@@LICENSE
//
//      Copyright (c) 2015 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#include <GeoLocation.h>
#include <loc_log.h>
#include <cmath>

GeoLocation::GeoLocation() :
        mLatitude(0), mLongitude(0), mAltitude(0) {

}
GeoLocation::GeoLocation(std::string slocation) :
        mLatitude(0), mLongitude(0), mAltitude(0) {
    this->mStrLocation = slocation;
}
bool GeoLocation::operator==(const GeoLocation &location) const {
    bool latitudeEq = (std::isnan(mLatitude) && std::isnan(location.mLatitude));
    bool longitudeEq = (std::isnan(mLongitude)
            && std::isnan(location.mLongitude));
    bool altitudeEq = (std::isnan(mAltitude) && std::isnan(location.mAltitude));

    return (latitudeEq && longitudeEq && altitudeEq);
}
GeoLocation::~GeoLocation() {
}

double GeoLocation::getLatitude() const {
    return this->mLatitude;
}

double GeoLocation::getLongitude() const {
    return this->mLongitude;
}
std::string GeoLocation::toString() const {
    return this->mStrLocation;
}
double GeoLocation::getAltitude() const {
    return this->mAltitude;
}

void GeoLocation::setLatitude(double latitude) {
    this->mLatitude = latitude;
}

void GeoLocation::setLongitude(double longitude) {
    this->mLongitude = longitude;
}

void GeoLocation::setAltitude(double altitude) {
    this->mAltitude = altitude;
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
