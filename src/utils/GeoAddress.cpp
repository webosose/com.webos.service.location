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

