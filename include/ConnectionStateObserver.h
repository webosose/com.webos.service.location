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

#include <LocationService.h>
#include <LocationService_Log.h>
#include <iostream>
#include <set>
#include <algorithm>
class ConnectionStateObserver
{
public:
    /*ConnectionStateObserver(LocationService *locSrvcPtr) {
        m_locSrvcPtr = locSrvcPtr;
    }
    LocationService *m_locSrvcPtr;*/
    ~ConnectionStateObserver() {
    }
    void RegisterListener(IConnectivityListener *);
    void init(LSHandle *);
    void UnregisterListener(IConnectivityListener *);

    enum SERVICE_STATE {
        SERVICE_NOT_RUNNING = 0,
        SERVICE_GETTING_READY,
        SERVICE_READY
    };
private:
    // void NotifyStateChange();
    bool _wifi_status_cb(LSHandle *sh, LSMessage *reply);
    bool _connectivity_status_cb(LSHandle *sh, LSMessage *reply);
    bool _telephony_status_cb(LSHandle *sh, LSMessage *reply);
    static bool wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx);
    static bool telephony_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *)ctx)->_telephony_status_cb(sh, message);
    }
    static bool connectivity_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((ConnectionStateObserver *)ctx)->_connectivity_status_cb(sh, message);
    }
    void register_wifi_status(LSHandle *);
    void register_telephony_status(LSHandle *);
    void register_connectivity_status(LSHandle *);
    void Notify_WifiStateChange(bool);
    void Notify_ConnectivityStateChange(bool);
    void Notify_TelephonyStateChange(bool);
    std::set<IConnectivityListener *> m_listeners;


};

#endif
