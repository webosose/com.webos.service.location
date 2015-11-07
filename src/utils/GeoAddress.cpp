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

#include <GeoAddress.h>

GeoAddress::GeoAddress(std::string addressString) {
    mAddressStr = addressString;
}
const std::string GeoAddress::getCity() const {
   return mCity;
}

void GeoAddress::setCity(const std::string& city) {
    mCity = city;
}

const std::string GeoAddress::getCountry() const {
   return mCountry;
}

void GeoAddress::setCountry(const std::string& country) {
    mCountry = country;
}

const std::string GeoAddress::getCountryCode() const {
    return mCountryCode;
}

void GeoAddress::setCountryCode(const std::string& countryCode) {
    mCountryCode = countryCode;
}

const std::string GeoAddress::getDistrict() const {
    return mDistrict;
}

void GeoAddress::setDistrict(const std::string& district) {
    mDistrict = district;
}

const std::string GeoAddress::getPostalCode() const {
    return mPostalCode;
}

void GeoAddress::setPostalCode(const std::string& postalCode) {
    mPostalCode = postalCode;
}

const std::string GeoAddress::getState() const {
    return mState;
}

void GeoAddress::setState(const std::string& state) {
    mState = state;
}

const std::string GeoAddress::getStreet() const {
    return mStreet;
}
const std::string GeoAddress::toString() const {
    return mAddressStr;
}
void GeoAddress::setStreet(const std::string& street) {
    mStreet = street;
}

