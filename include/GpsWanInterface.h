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


#include <string.h>
#include <luna-service2/lunaservice.h>

class GpsWanInterface
{
    public:
        GpsWanInterface() :
            mWanGetContext(LSMESSAGE_TOKEN_INVALID), _mLSHandle(nullptr), mServiceConnected(false) {
        }
        ~GpsWanInterface() {
        }
        void registerLunaMethods();
        void initialize(LSHandle *sh);
        void connect();
        void disconnect();
        bool settingsServiceLunaCall();
        bool serviceStatus(){return mServiceConnected;};

    private:
        void wanServiceStatus();
        void serviceStarted();
        void serviceStopped();

    private:
        static bool getContextCb(LSHandle *sh, LSMessage *message, void * context);
        static bool connectCb(LSHandle *sh, LSMessage *message, void * context);
        static bool disconnectCb(LSHandle *sh, LSMessage *message, void * context);
        static bool settingServiceCb(LSHandle *sh, LSMessage *message, void * context);
        static bool serviceUpCb(LSHandle* handle, LSMessage* message, void* ctxt);

    private:
        LSMessageToken mWanGetContext;
        LSHandle *_mLSHandle;
        bool mServiceConnected;
        static std::string mApnName;
};
