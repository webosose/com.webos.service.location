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

#include <string.h>
#include <luna-service2/lunaservice.h>

class GpsWanInterface
{
    public:
        GpsWanInterface() {
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
