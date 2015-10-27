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


#if !defined(_GEOLOCATION_H)
#define _GEOLOCATION_H

#include <string>

class GeoLocation {
public:
    GeoLocation();
    GeoLocation(std::string slocation);
    GeoLocation(const GeoLocation& rhs) = default;
    GeoLocation& operator=(const GeoLocation& rhs) = default;
    bool operator==(const GeoLocation &other) const;
    ~GeoLocation();
    double getLatitude() const;
    double getLongitude() const;
    std::string toString() const;
    double getAltitude() const;
    void setLatitude(double latitude);
    void setLongitude(double longitude);
    void setAltitude(double altitude);
    void printLocation();
    const std::string& getProviderId() const;
    void setProviderId(const std::string& providerId);

private:
    std::string mStrLocation;
    double mLatitude;
    double mLongitude;
    double mAltitude;
    std::string mProviderId;
};

#endif  //_GEOLOCATION_H
