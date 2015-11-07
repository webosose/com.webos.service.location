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


#if !defined(_WSPINTERFACE_H)
#define _WSPINTERFACE_H


#include <GeoCodeInterface.h>
#include <vector>
#include <array>
#include <location_errors.h>

typedef enum {
    NO_FEATURE_SUPPORTED,
    GEO_CODE,
    REVERSE_GEOCODE,
    SEARCH,
    TIMEZONE,
    DISTANCE,
    MAX
} FeatureType;

typedef std::array<FeatureType, FeatureType::MAX> SupportedFeatureList;

class WSPInterface {

public:

    virtual void publishFeatures() {
    }

    WSPInterface(std::string wspId) {
        mWspId = wspId;
     mSupportedFeatureList.fill(NO_FEATURE_SUPPORTED);
    }

    virtual ~WSPInterface() {
    }

    virtual GeoCodeInterface* getGeocodeImpl() {
        return nullptr;
    }

    void registerFeature(FeatureType feature) {
        mSupportedFeatureList[feature] = feature;
    }

    SupportedFeatureList getSupportedFeatures() {
        return mSupportedFeatureList;
    }

    bool isRegistered(FeatureType featureType) {
        return mSupportedFeatureList[featureType];
    }

    std::string getName() const {
        return mWspId;
    }

private:

    std::string mWspId;
    SupportedFeatureList mSupportedFeatureList;

};

#endif  //_WSPINTERFACE_H
