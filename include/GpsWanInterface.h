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
        void register_luna_methods();
        void initialize(LSHandle *sh);
        void connect();
        void disconnect();
        bool settingsservicelunacall();
        bool serviceStatus(){return serviceConnected;};

    private:
        void wan_service_status();
        void service_started();
        void service_stopped();

    private:
        static bool getContextCb(LSHandle *sh, LSMessage *message, void * context);
        static bool connectCb(LSHandle *sh, LSMessage *message, void * context);
        static bool disconnectCb(LSHandle *sh, LSMessage *message, void * context);
        static bool settingServiceCb(LSHandle *sh, LSMessage *message, void * context);
        static bool service_up_cb(LSHandle* handle, LSMessage* message, void* ctxt);

    private:
        LSMessageToken mWanGetContext;
        LSHandle *_mLSHandle;
        bool serviceConnected;
        static std::string mApnName;
};
