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



#if !defined(_GOOGLEWSP_H)
#define _GOOGLEWSP_H

#include <WSPInterface.h>
#include <GoogleGeoImpl.h>


class GoogleWSP : public WSPInterface {

public:

    GoogleWSP(std::string googleWSPName) : WSPInterface(googleWSPName) {
        publishFeatures();
    }

    GeoCodeInterface *getGeocodeImpl();

    virtual void publishFeatures();

private:

    static bool mRegistered;
};

#endif  //_GOOGLEWSP_H
