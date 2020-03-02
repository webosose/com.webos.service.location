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
