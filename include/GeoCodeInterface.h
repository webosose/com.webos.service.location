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


#if !defined(_GEOCODEINTERFACE_H)
#define _GEOCODEINTERFACE_H
#include <iostream>
#include <GeoLocation.h>
#include <GeoAddress.h>
#include <location_errors.h>
#include <lunaservice.h>
#include <loc_log.h>
#include <functional>

typedef  std::function <void (GeoLocation, int, LSMessage*)> GeoCodeCb;
typedef  std::function <void (GeoAddress, int, LSMessage*)> ReverseGeoCodeCb;

class GeoCodeInterface {
public:

    GeoCodeInterface() {
    }
    virtual ~GeoCodeInterface() {

    }
    virtual ErrorCodes geoCode(GeoAddress address ,GeoCodeCb geocodeCallback ,bool isSync, LSMessage *message) {
        LS_LOG_ERROR("No support for geocode");
        return  ERROR_NOT_IMPLEMENTED;
    }
    virtual ErrorCodes reverseGeoCode(GeoLocation geolocation, ReverseGeoCodeCb revGecodeCallback, bool isSync, LSMessage *message) {
        LS_LOG_ERROR("No support for reverseGeoCode");
        return ERROR_NOT_IMPLEMENTED;
    }

public:

    GeoCodeCb mGeoCodeCb = nullptr;
    ReverseGeoCodeCb mRevGeoCodeCb = nullptr;

};

#endif  //_GEOCODEINTERFACE_H
