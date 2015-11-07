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


#include <GoogleWSP.h>
#include <LBSEngine.h>

#define GOOGLEWSPNAME  "google"

bool GoogleWSP::mRegistered = LBSEngine::getInstance()->registerWebServiceProvider(std::string(GOOGLEWSPNAME),
                                                                                   new(std::nothrow) GoogleWSP(
                                                                                           GOOGLEWSPNAME));

GeoCodeInterface *GoogleWSP::getGeocodeImpl() {
    static GoogleGeoImpl geoCodeImpl;
    return &geoCodeImpl;
}

void GoogleWSP::publishFeatures() {
    this->registerFeature(REVERSE_GEOCODE);
    this->registerFeature(GEO_CODE);
}

