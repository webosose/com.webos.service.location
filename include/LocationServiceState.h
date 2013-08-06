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
 * Filename  : LocationServiceState.cpp
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

class LocationServiceState
{
public:
    LocationServiceState(LocationService *locSrvcPtr) {
        m_locSrvcPtr = locSrvcPtr;
    }
    LocationService *m_locSrvcPtr;
    ~LocationServiceState() {
    }
    void init();

    enum SERVICE_STATE {
        SERVICE_NOT_RUNNING = 0,
        SERVICE_GETTING_READY,
        SERVICE_READY
    };
private:
    void NotifyStateChange(int);
    bool _wifi_status_cb(LSHandle *sh, LSMessage *reply);
    bool _connectivity_status_cb(LSHandle *sh, LSMessage *reply);
    bool _telephony_status_cb(LSHandle *sh, LSMessage *reply);
    static bool wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx);
    static bool telephony_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((LocationServiceState *)ctx)->_telephony_status_cb(sh, message);
    }
    static bool connectivity_status_cb(LSHandle *sh, LSMessage *message, void *ctx) {
        return ((LocationServiceState *)ctx)->_connectivity_status_cb(sh, message);
    }
    void register_wifi_status();
    void register_telephony_status();
    void register_connectivity_status();
};

#endif
