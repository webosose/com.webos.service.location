/*
 * Copyright (c) 2012-2014 LG Electronics Inc. All Rights Reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <pbnjson.h>
#include <loc_log.h>
#include "LifeCycleMonitor.h"

#define SUSPEND_MONITOR_CLIENT_NAME     "location-suspend"


static bool dummyCallback(LSHandle* sh, LSMessage* message, void* ctx)
{
    LS_LOG_INFO("%s\n", LSMessageGetPayload(message));

    return true;
}



LifeCycleMonitor::LifeCycleMonitor()
{
    m_suspendService = NULL;
    m_suspendMonitorId = NULL;
    m_registeredWakeLock = false;
    m_setWakeLock = false;
}

LifeCycleMonitor::~LifeCycleMonitor()
{
    setWakeLock(false);
    g_free(m_suspendMonitorId);
}

bool LifeCycleMonitor::registerSuspendMonitor(LSHandle* service)
{
    bool result;
    LSError lsErr;

    if (!service)
        return false;

    LSErrorInit(&lsErr);

    m_suspendService = service;

    result = LSRegisterServerStatusEx(m_suspendService,
                                      "com.palm.sleep",
                                      LifeCycleMonitor::cbPowerdUp,
                                      this,
                                      NULL,
                                      &lsErr);
    if (!result) {
        LS_LOG_ERROR("Failed to register server status of com.palm.sleep: %s\n", lsErr.message);
        LSErrorFree(&lsErr);
        return false;
    }

    return true;
}

void LifeCycleMonitor::setWakeLock(bool set)
{
    char message[256];

    if (!m_registeredWakeLock || m_setWakeLock == set)
        return;

    memset(message, 0, 256);
    sprintf(message, "{\"clientId\":\"%s\",\"isWakeup\":%s}", m_suspendMonitorId, (set ? "true":"false"));

    if (callService(m_suspendService,
                    NULL,
                    "luna://com.palm.sleep/com/palm/power/setWakeLock",
                    message)) {
        m_setWakeLock = set;

        LS_LOG_INFO("setWakeLock ( %s )\n", m_setWakeLock ? "true" : "false");
    }
}

bool LifeCycleMonitor::callService(LSHandle* service,
                                   LSFilterFunc callback,
                                   const char* url,
                                   const char* message)
{
    bool result;
    LSError lsErr;
    LSErrorInit(&lsErr);

    result = LSCall(service, url, message, callback ? callback : dummyCallback, this, NULL, &lsErr);
    if (!result) {
        LS_LOG_ERROR("Failed in LSCall with url: %s, message: %s, Error: %s\n",
                url, message, lsErr.message);
        LSErrorFree(&lsErr);
    }

    return result;
}

void LifeCycleMonitor::registerWakeLock()
{
    char message[256];

    if (m_registeredWakeLock)
        return;

    memset(message, 0, 256);
    sprintf(message, "{\"register\":true,\"clientId\":\"%s\"}", m_suspendMonitorId);

    if (callService(m_suspendService,
                    NULL,
                    "luna://com.palm.sleep/com/palm/power/wakeLockRegister",
                    message)) {
        m_registeredWakeLock = true;

        LS_LOG_INFO("wake lock registered\n");
    }
}

bool LifeCycleMonitor::cbPowerdUp(LSHandle* sh, const char *serviceName, bool connected, void* ctx)
{
    LifeCycleMonitor* monitor = NULL;
    char message[256];

    monitor = static_cast<LifeCycleMonitor*>(ctx);
    if (!monitor)
        return true;

    if (connected) {
        LS_LOG_INFO("Connected to com.palm.sleep\n");

        memset(message, 0, 256);
        sprintf(message, "{\"subscribe\":true,\"clientName\":\"%s\"}", SUSPEND_MONITOR_CLIENT_NAME);

        monitor->callService(monitor->m_suspendService,
                             LifeCycleMonitor::cbIdentify,
                             "palm://com.palm.power/com/palm/power/identify",
                             message);
    } else {
        LS_LOG_INFO("Disconnected from com.palm.sleep\n");

        monitor->m_registeredWakeLock = false;
        monitor->m_setWakeLock = false;
    }

    return true;
}

bool LifeCycleMonitor::cbIdentify(LSHandle* sh, LSMessage* msg, void* ctx)
{
    LifeCycleMonitor* monitor = NULL;
    JSchemaInfo schemaInfo;
    jvalue_ref parsed_obj = NULL;
    jvalue_ref obj = NULL;
    bool subscribed = false;
    raw_buffer clientId;

    monitor = static_cast<LifeCycleMonitor*>(ctx);
    if (!monitor)
        return true;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    parsed_obj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(parsed_obj))
        return true;

    if (jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("subscribed"), &obj))
        jboolean_get(obj, &subscribed);

    if (monitor->m_suspendMonitorId) {
        g_free(monitor->m_suspendMonitorId);
        monitor->m_suspendMonitorId = NULL;
    }

    if (jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("clientId"), &obj)) {
        clientId = jstring_get(obj);
        monitor->m_suspendMonitorId = g_strdup(clientId.m_str);
        jstring_free_buffer(clientId);
    }

    if (!subscribed || !monitor->m_suspendMonitorId) {
        LS_LOG_ERROR("Failed to subscribe to powerd %s\n", LSMessageGetPayload(msg));
        j_release(&parsed_obj);
        return true;
    }

    LS_LOG_INFO("Got clientId %s from powerd\n", monitor->m_suspendMonitorId);

    monitor->registerWakeLock();

    j_release(&parsed_obj);

    return true;
}

