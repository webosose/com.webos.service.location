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
        mLatitude(0.0), mLongitude(0.0), mAltitude(0.0), mHorAccuracy(0.0),
        mTimeStamp(    0.0), mDirection(0.0), mClimb(0.0), mSpeed(0.0), mVertAccuracy(    0.0) {

}
GeoLocation::GeoLocation(std::string slocation) :
        mLatitude(0), mLongitude(0), mAltitude(0), mHorAccuracy(0), mVertAccuracy(0) {
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
