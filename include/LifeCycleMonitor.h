/*
 * Copyright (c) 2012-2014 LG Electronics Inc. All Rights Reserved.
 */

#ifndef _LIFECYCLEMONITOR_H_
#define _LIFECYCLEMONITOR_H_

#include <glib.h>
#include <lunaservice.h>


class LifeCycleMonitor
{
public:
    LifeCycleMonitor();
    ~LifeCycleMonitor();

    // Suspend Monitor
    bool registerSuspendMonitor(LSHandle* service);

    // Set wake lock
    void setWakeLock(bool set, bool force = false);

private:
    bool callService(LSHandle* service, LSFilterFunc callback,
                     const char* url, const char* payload);

    // Suspend Monitor callbacks
    static bool cbPowerdUp(LSHandle* sh, const char *serviceName, bool connected, void* ctx);
    static bool cbIdentify(LSHandle* sh, LSMessage* message, void* ctx);
    static bool cbRegisterWakeLock(LSHandle* sh, LSMessage* message, void* ctx);

private:
    LSHandle* m_suspendService;
    char* m_suspendMonitorId;
    bool m_registeredWakeLock;
    bool m_setWakeLock;
    int m_refWakeLock;
};


#endif
