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

#if !defined(_GEOADDRESS_H)
#define _GEOADDRESS_H

#include <string>
class GeoAddress {
public:

    GeoAddress(std::string addressString);
    const std::string getCity() const;
    void setCity(const std::string& city);
    const std::string getCountry() const;
    void setCountry(const std::string& country);
    const std::string getCountryCode() const;
    void setCountryCode(const std::string& countryCode);
    const std::string getDistrict() const;
    void setDistrict(const std::string& district);
    const std::string getPostalCode() const;
    void setPostalCode(const std::string& postalCode);
    const std::string getState() const;
    void setState(const std::string& state);
    const std::string getStreet() const;
    void setStreet(const std::string& street);
    const std::string toString() const;

private:
    std::string mAddressStr;
    std::string mCountry;
    std::string mCountryCode;
    std::string mState;
    std::string mCity;
    std::string mDistrict;
    std::string mStreet;
    std::string mPostalCode;
};

#endif  //_GEOADDRESS_H
