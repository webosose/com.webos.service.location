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
