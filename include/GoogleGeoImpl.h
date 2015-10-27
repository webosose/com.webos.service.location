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


#if !defined(_GOOGLEGEOIMPL_H)
#define _GOOGLEGEOIMPL_H

#include <memory>
#include <algorithm>
#include <string.h>
#include <GeoCodeInterface.h>
#include <loc_http.h>
#include <loc_security.h>
#include <WSPInterface.h>
#include <HttpInterface.h>
#include <NetworkRequestManager.h>
#include <loc_log.h>

#define GOOGLE_LBS_URL             https:\/\/maps.googleapis.com
#define GOOGLE_SUB_URL_TO_SIGN     /maps/api/geocode/json?
#define GOOGLE_CLIENT_KEY          &client=gme-lgelectronics1
#define GEOCODEKEY_CONFIG_PATH      "/etc/geocode.conf"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


class GoogleGeoImpl: public GeoCodeInterface, public HttpInterface {

public:

    GoogleGeoImpl();
    ~GoogleGeoImpl();
    ErrorCodes geoCode(GeoAddress address,GeoCodeCb geocodeCallback ,bool isSync, LSMessage *message);
    ErrorCodes reverseGeoCode(GeoLocation geolocation, ReverseGeoCodeCb revGeocodeCallback, bool isSync, LSMessage *message);
    ErrorCodes lbsPostQuery(std::string url ,bool isSync, LSMessage *message);
    std::string formatUrl(std::string address ,const char* key);
    char* readApiKey();
    void handleResponse(HttpReqTask *task);

private:

    std::string mGoogleGeoCodeApiKey;
};

#endif  //_GOOGLEGEOIMPL_H
