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
    m_refWakeLock = 0;
}

LifeCycleMonitor::~LifeCycleMonitor()
{
    setWakeLock(false, true);
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

void LifeCycleMonitor::setWakeLock(bool set, bool force)
{
    char payload[256];

    if (!m_registeredWakeLock) {
        LS_LOG_WARNING("Wakelock is not registered yet!\n");
        return;
    }

    if (set) {
        m_refWakeLock++;
    } else {
        m_refWakeLock = (--m_refWakeLock < 0) ? 0 : m_refWakeLock;
    }

    LS_LOG_INFO("Current wakelock reference count = %d\n", m_refWakeLock);

    if (m_setWakeLock == set)
        return;

    if (set == false && force == true)
        m_refWakeLock = 0;

    if (set == false && m_refWakeLock > 0)
        return;

    memset(payload, 0, 256);
    snprintf(payload, 256, "{\"clientId\":\"%s\",\"isWakeup\":%s}", m_suspendMonitorId, (set ? "true":"false"));

    if (callService(m_suspendService,
                    NULL,
                    "luna://com.palm.sleep/com/palm/power/setWakeLock",
                    payload)) {
        m_setWakeLock = set;

        LS_LOG_INFO("setWakeLock ( %s )\n", m_setWakeLock ? "true" : "false");
    }
}

bool LifeCycleMonitor::callService(LSHandle* service,
                                   LSFilterFunc callback,
                                   const char* url,
                                   const char* payload)
{
    bool result;
    LSError lsErr;

    LSErrorInit(&lsErr);

    result = LSCall(service, url, payload, callback ? callback : dummyCallback, this, NULL, &lsErr);
    if (!result) {
        LS_LOG_ERROR("Failed in LSCall with url: %s, payload: %s, Error: %s\n",
                     url, payload, lsErr.message);
        LSErrorFree(&lsErr);
    }

    return result;
}

bool LifeCycleMonitor::cbPowerdUp(LSHandle* sh, const char *serviceName, bool connected, void* ctx)
{
    LifeCycleMonitor* monitor = NULL;
    char payload[256];

    monitor = static_cast<LifeCycleMonitor*>(ctx);
    if (!monitor)
        return true;

    if (connected) {
        LS_LOG_INFO("Connected to com.palm.sleep\n");

        memset(payload, 0, 256);
        snprintf(payload, 256, "{\"subscribe\":true,\"clientName\":\"%s\"}", SUSPEND_MONITOR_CLIENT_NAME);

        monitor->callService(monitor->m_suspendService,
                             LifeCycleMonitor::cbIdentify,
                             "palm://com.palm.power/com/palm/power/identify",
                             payload);
    } else {
        LS_LOG_INFO("Disconnected from com.palm.sleep\n");

        monitor->m_registeredWakeLock = false;
        monitor->m_setWakeLock = false;
        monitor->m_refWakeLock = 0;
    }

    return true;
}

bool LifeCycleMonitor::cbIdentify(LSHandle* sh, LSMessage* message, void* ctx)
{
    LifeCycleMonitor* monitor = NULL;
    JSchemaInfo schemaInfo;
    jvalue_ref parsed_obj = NULL;
    jvalue_ref obj = NULL;
    bool subscribed = false;
    raw_buffer clientId;
    char payload[256];

    monitor = static_cast<LifeCycleMonitor*>(ctx);
    if (!monitor)
        return true;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    parsed_obj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

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
        LS_LOG_ERROR("Failed to subscribe to powerd %s\n", LSMessageGetPayload(message));
        j_release(&parsed_obj);
        return true;
    }

    LS_LOG_INFO("Got clientId %s from powerd\n", monitor->m_suspendMonitorId);

    j_release(&parsed_obj);

    // Register wake lock
    if (monitor->m_registeredWakeLock == false) {
        memset(payload, 0, 256);
        snprintf(payload, 256, "{\"register\":true,\"clientId\":\"%s\"}", monitor->m_suspendMonitorId);

        monitor->callService(monitor->m_suspendService,
                             LifeCycleMonitor::cbRegisterWakeLock,
                             "luna://com.palm.sleep/com/palm/power/wakeLockRegister",
                             payload);
    }

    return true;
}

bool LifeCycleMonitor::cbRegisterWakeLock(LSHandle* sh, LSMessage* message, void* ctx)
{
    LifeCycleMonitor* monitor = NULL;
    JSchemaInfo schemaInfo;
    jvalue_ref parsed_obj = NULL;
    jvalue_ref obj = NULL;
    bool bReturnValue = false;

    monitor = static_cast<LifeCycleMonitor*>(ctx);
    if (!monitor)
        return true;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    parsed_obj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(parsed_obj))
        return true;

    if (jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("returnValue"), &obj)) {
        jboolean_get(obj, &bReturnValue);

        if (bReturnValue) {
            monitor->m_registeredWakeLock = true;
            LS_LOG_INFO("Succeeded to register wake lock\n");
            goto EXIT;
        }
    }

    LS_LOG_ERROR("Failed to register wake lock\n");

EXIT:
    j_release(&parsed_obj);

    return true;
}
