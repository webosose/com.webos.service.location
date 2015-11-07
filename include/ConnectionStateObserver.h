/**********************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Red Rose Platform location
 * Team   : SWF Team
 * Security  : Confidential
 * ***********************************************************/

/*********************************************************
 * @file
 * Filename  : ConnectionStateObserver.cpp
 * Purpose  : Provides location related API to application
 * Platform  : RedRose
 * Author(s)  : Mohammed Sameer Mulla
 * E-mail id. : sameer.mulla@lge.com
 * Creation date :
 *
 * Modifications:
 *
 * Sl No Modified by  Date  Version Description
 *
 **********************************************************/
#ifndef _LOCATION_SERVICE_STATE_H_
#define _LOCATION_SERVICE_STATE_H_


#include <lunaservice.h>
#include <set>
#include "IConnectivityListener.h"

#define WIFI_SERVICE "com.webos.service.wifi"
#define WAN_SERVICE "com.webos.service.wan"
#define TELEPHONY_SERVICE "com.webos.service.telephony"

class ConnectionStateObserver {
public:
    ~ConnectionStateObserver() {
    }

    void RegisterListener(IConnectivityListener *);

    void init(LSHandle *);

    void finalize(LSHandle *);

    void UnregisterListener(IConnectivityListener *);

private:
    // void NotifyStateChange();
    bool _wifi_status_cb(LSHandle *sh, LSMessage *reply);

    bool _connectivity_status_cb(LSHandle *sh, LSMessage *reply);

    bool _telephony_status_cb(LSHandle *sh, LSMessage *reply);

    bool _wifi_service_status_cb(LSHandle *sh, const char *serviceName, bool connected);

    bool _wan_service_status_cb(LSHandle *sh, const char *serviceName, bool connected);

    bool _tel_service_status_cb(LSHandle *sh, const char *serviceName, bool connected);

    bool _resume_cb(LSHandle *sh, LSMessage *message);

    bool _suspended_cb(LSHandle *sh, LSMessage *message);

    static bool wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_wifi_status_cb(sh, message);
    }

    static bool telephony_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_telephony_status_cb(sh, message);
    }

    static bool suspended_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_suspended_cb(sh, message);
    }

    static bool resume_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_resume_cb(sh, message);
    }

    static bool connectivity_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_connectivity_status_cb(sh, message);
    }

    static bool wifi_service_status_cb(LSHandle *sh,
                                       const char *serviceName,
                                       bool connected,
                                       void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_wifi_service_status_cb(sh, serviceName, connected);
    }

    static bool wan_service_status_cb(LSHandle *sh,
                                      const char *serviceName,
                                      bool connected,
                                      void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_wan_service_status_cb(sh, serviceName, connected);
    }

    static bool tel_service_status_cb(LSHandle *sh,
                                      const char *serviceName,
                                      bool connected,
                                      void *ctx) {
        return ((ConnectionStateObserver *) ctx)->_tel_service_status_cb(sh, serviceName, connected);
    }

    void register_wifi_status(LSHandle *);

    void register_telephony_status(LSHandle *);

    void register_connectivity_status(LSHandle *);

    void Notify_WifiStateChange(bool);

    void Notify_ConnectivityStateChange(bool);

    void Notify_TelephonyStateChange(bool);

    void Notify_WifiInternetStateChange(bool);

    void Notify_SuspendedStateChange(bool);

    std::set<IConnectivityListener *> m_listeners;
    void *m_telephony_cookie;
    void *m_wifi_cookie;
    void *m_wan_cookie;

};

#endif
