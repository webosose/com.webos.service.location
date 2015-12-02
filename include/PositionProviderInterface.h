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

#ifndef POSITIONPROVIDERINTERFACE_H_
#define POSITIONPROVIDERINTERFACE_H_

#include <PositionRequest.h>
#include <iostream>
#include <location_errors.h>
#include <Location.h>
#include <functional>
#include <ILocationCallbacks.h>



class PositionProviderInterface {

private:
    std::string mProvider_name;
    ILocationCallbacks* mClientCallBacks;
protected:


    bool mEnabled;

public:
    PositionProviderInterface(std::string provider_name) :
            mProvider_name(provider_name), mClientCallBacks(nullptr), mEnabled(false) {
    }


    std::string getName() {
        return mProvider_name;
    }

    bool isEnabled() {
        return mEnabled;
    }

    virtual ~PositionProviderInterface() { }

    virtual ErrorCodes processRequest(PositionRequest request) = 0;

    virtual void enable() = 0;
    void setCallback(ILocationCallbacks* callback) {
        mClientCallBacks = callback;
    }
    ILocationCallbacks *getCallback() const {
        return mClientCallBacks;
    }
    virtual void disable() = 0;

};

#endif /* POSITIONPROVIDERINTERFACE_H_ */
