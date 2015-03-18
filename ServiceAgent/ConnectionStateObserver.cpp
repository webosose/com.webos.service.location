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
#include <ConnectionStateObserver.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <string>
#include <boost/foreach.hpp>
#include <JsonUtility.h>

/*
 * JSON SCHEMA: _suspended_cb ()
 */
#define JSCHEMA_SIGNAL_SUSPEND             SCHEMA_NONE

/*
 * JSON SCHEMA: _resume_cb ()
 */
#define JSCHEMA_SIGNAL_RESUME              STRICT_SCHEMA(\
    PROPS_1(PROP(resumetype, integer)) \
    REQUIRED_1(resumetype))



void ConnectionStateObserver::RegisterListener(IConnectivityListener *l)
{
    if (l == NULL)
        return;

    m_listeners.insert(l);
}

void ConnectionStateObserver::UnregisterListener(IConnectivityListener *l)
{
    if (l == NULL)
        return;

    std::set<IConnectivityListener *>::const_iterator iter = m_listeners.find(l);

    if (iter != m_listeners.end()) {
        m_listeners.erase(iter);
    } else {
        LS_LOG_DEBUG("Could not unregister the specified listener object as it is not registered.\n");
    }
}

void ConnectionStateObserver::init(LSHandle *ConnHandle)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    LS_LOG_INFO("init\n");

    if (ConnHandle == NULL)
        return;

    //Register for status of wifi,wan,telephony service
    result =  LSRegisterServerStatusEx(ConnHandle,
                                       WIFI_SERVICE,
                                       ConnectionStateObserver::wifi_service_status_cb,
                                       this,
                                       &m_wifi_cookie,
                                       &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
    result =  LSRegisterServerStatusEx(ConnHandle,
                                       WAN_SERVICE,
                                       ConnectionStateObserver::wan_service_status_cb,
                                       this,
                                       &m_wan_cookie,
                                       &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
    result =  LSRegisterServerStatusEx(ConnHandle,
                                       TELEPHONY_SERVICE,
                                       ConnectionStateObserver::tel_service_status_cb,
                                       this,
                                       &m_telephony_cookie,
                                       &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSCall(ConnHandle,
                    "palm://com.palm.bus/signal/addmatch",
                    "{\"category\":\"/com/palm/power\", \"method\":\"suspended\"}",
                    ConnectionStateObserver::suspended_cb,
                    this,
                    NULL,
                    &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSCall(ConnHandle,
                    "palm://com.palm.bus/signal/addmatch",
                    "{\"category\":\"/com/palm/power\", \"method\":\"resume\"}",
                    ConnectionStateObserver::resume_cb,
                    this,
                    NULL,
                    &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
}

bool ConnectionStateObserver::_suspended_cb(LSHandle* sh, LSMessage* msg)
{
    jvalue_ref parsedObj = NULL;
    JSchemaInfo schemaInfo;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer(JSCHEMA_SIGNAL_SUSPEND), DOMOPT_NOOPT, NULL);
    if (!input_schema)
        return true;

    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(parsedObj))
        return true;

    LS_LOG_INFO("_suspended_cb called, %s\n", LSMessageGetPayload(msg));

    Notify_SuspendedStateChange(true);

    j_release(&parsedObj);

    return true;
}

bool ConnectionStateObserver::_resume_cb(LSHandle* sh, LSMessage* msg)
{
    int resumetype;
    jvalue_ref jsonSubObj = NULL;
    jvalue_ref parsedObj = NULL;
    JSchemaInfo schemaInfo;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer(JSCHEMA_SIGNAL_RESUME), DOMOPT_NOOPT, NULL);
    if (!input_schema)
        return true;

    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(parsedObj))
        return true;

    LS_LOG_INFO("_resume_cb called, %s\n", LSMessageGetPayload(msg));

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("resumetype"), &jsonSubObj)) {
        jnumber_get_i32(jsonSubObj, &resumetype);

        if (resumetype == 0 || resumetype == 1 || resumetype == 2) {
            Notify_SuspendedStateChange(false);
        }
    }

    j_release(&parsedObj);

    return true;

}

void ConnectionStateObserver::finalize(LSHandle *ConnHandle)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    result = LSCancelServerStatus(ConnHandle, m_telephony_cookie, &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSCancelServerStatus(ConnHandle, m_wifi_cookie, &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSCancelServerStatus(ConnHandle, m_wan_cookie, &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
}

void ConnectionStateObserver::Notify_WifiStateChange(bool WifiState)
{
    BOOST_FOREACH(IConnectivityListener * l, m_listeners) {
        l->Handle_WifiNotification(WifiState);
    }
    LS_LOG_DEBUG("Notify_WifiStateChange\n");
}
void ConnectionStateObserver::Notify_WifiInternetStateChange(bool WifiInternetState)
{
    BOOST_FOREACH(IConnectivityListener * l, m_listeners) {
        l->Handle_WifiInternetNotification(WifiInternetState);
    }
    LS_LOG_DEBUG("Notify_WifiInternetStateChange\n");
}
void ConnectionStateObserver::Notify_ConnectivityStateChange(bool ConnState)
{
    BOOST_FOREACH(IConnectivityListener * l, m_listeners) {
        l->Handle_ConnectivityNotification(ConnState);
    }
    LS_LOG_DEBUG("Notify_ConnectivityStateChange\n");
}
void ConnectionStateObserver::Notify_TelephonyStateChange(bool TeleState)
{
    BOOST_FOREACH(IConnectivityListener * l, m_listeners) {
        l->Handle_TelephonyNotification(TeleState);
    }
    LS_LOG_DEBUG("Notify_TelephonyStateChange\n");
}
void ConnectionStateObserver::Notify_SuspendedStateChange(bool SuspendState)
{
    BOOST_FOREACH(IConnectivityListener * l, m_listeners) {
        l->Handle_SuspendedNotification(SuspendState);
    }
    LS_LOG_DEBUG("Notify_SuspendedStateChange = %d\n",SuspendState);
}
void ConnectionStateObserver::register_wifi_status(LSHandle *HandleConn)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    result = LSCall(HandleConn,
                    "palm://com.palm.wifi/getstatus",
                    "{\"subscribe\":true}",
                    ConnectionStateObserver::wifi_status_cb,
                    this,
                    NULL,
                    &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
}

void ConnectionStateObserver::register_connectivity_status(LSHandle *HandleConn)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    result = LSCall(HandleConn,
                    "palm://com.palm.wan/getstatus",
                    "{\"subscribe\":true}",
                    ConnectionStateObserver::connectivity_status_cb,
                    this,
                    NULL,
                    &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
}

void ConnectionStateObserver::register_telephony_status(LSHandle *HandleConn)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    //Check modem status
    //Not implemented, will be implemented in future
    result = LSCall(HandleConn,
                    "palm://com.palm.telephony/isTelephonyReady",
                    "{\"subscribe\":true}",
                    ConnectionStateObserver::telephony_status_cb,
                    this,
                    NULL,
                    &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
}

bool ConnectionStateObserver::wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx)
{
    return ((ConnectionStateObserver *) ctx)->_wifi_status_cb(sh, message);
}

bool ConnectionStateObserver::_wifi_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret = false;
    bool found = false;
    char *wifisrvEnable = NULL;
    bool wifistatus = false;
    jvalue_ref jsonSubObj = NULL;
    jvalue_ref parsedObj = NULL;

    JSchemaInfo schemaInfo;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
    if (!input_schema)
        return true;

    jschema_info_init(&schemaInfo, input_schema, NULL, NULL); // no external refs & no error handlers
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(parsedObj)){
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("returnValue"), &jsonSubObj)){
        jboolean_get(jsonSubObj, &ret);
        found = true;
    }

    if (found == false) {
        Notify_WifiStateChange(wifistatus);
        j_release(&parsedObj);
        return true;
    }

    found = false;

    if (ret == false) {
        int error = -1;

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("errorCode"), &jsonSubObj)){
            jnumber_get_i32(jsonSubObj, &error);
        }

        if (error == -1) {
            LS_LOG_ERROR("wifi service is not running");
            Notify_WifiStateChange(false);
        }

        j_release(&parsedObj);
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("status"), &jsonSubObj)){
        raw_buffer buf = jstring_get(jsonSubObj);
        wifisrvEnable = strdup(buf.m_str);
        jstring_free_buffer(buf);
        if(wifisrvEnable != NULL)
           found = true;
    }

    if (found == false) {
        Notify_WifiStateChange(wifistatus);
        j_release(&parsedObj);
        return true;
    }

    LS_LOG_DEBUG("wifisrvEnable %s", wifisrvEnable);

    if ((strcmp(wifisrvEnable, "serviceEnabled") == 0)) {
        wifistatus = true;
        Notify_WifiStateChange(wifistatus);
    } else if (strcmp(wifisrvEnable, "serviceDisabled") == 0) {
        wifistatus = false;
        Notify_WifiStateChange(wifistatus);
        Notify_WifiInternetStateChange(false);
    } else if (strcmp(wifisrvEnable, "connectionStateChanged") == 0) {
        LS_LOG_DEBUG("connectionStateChanged %s", wifisrvEnable);
        wifistatus = true;
        Notify_WifiStateChange(wifistatus);
        Notify_WifiInternetStateChange(true);
    } else {
        Notify_WifiInternetStateChange(true);
        Notify_WifiStateChange(wifistatus);
    }

    if (wifisrvEnable != NULL)
        g_free(wifisrvEnable);

    j_release(&parsedObj);
    return true;
}

/*luna-send -i palm://com.palm.connectionmanager/getstatus '{}'
 {"returnValue":true,"isInternetConnectionAvailable":false,"wired":{"state":"disconnected"},"wifi":{"state":"connected","interfaceName":"wlan0","ipAddress":"192.168.207.11
 9","netmask":"255.255.248.0","gateway":"192.168.200.1","dns1":"8.8.8.8","dns2":"4.2.2.2","method":"dhcp","ssid":"LGSI-TEST","isWakeOnWifiEnabled":false,"onInternet":"no"}
 }*/

bool ConnectionStateObserver::_connectivity_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret = false;
    bool found = false;
    bool isInternetConnectionAvailable = false;
    jvalue_ref jsonSubObj = NULL;
    jvalue_ref parsedObj = NULL;
    JSchemaInfo schemaInfo;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
    if(!input_schema)
       return false;

    jschema_info_init(&schemaInfo, input_schema, NULL, NULL); // no external refs & no error handlers
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(parsedObj)){
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("returnValue"), &jsonSubObj)){
        jboolean_get(jsonSubObj, &ret);
        found = true;
    }

    if (found == false) {
        Notify_ConnectivityStateChange(isInternetConnectionAvailable);
        j_release(&parsedObj);
        return true;
    }

    found = false;

    if (ret) {
        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("isInternetConnectionAvailable"), &jsonSubObj)){
            jboolean_get(jsonSubObj, &isInternetConnectionAvailable);
            found=true;
        }

        if (found == false) {
            Notify_ConnectivityStateChange(false);
            j_release(&parsedObj);
            return true;
        }

    } else {
        int error = -1;

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("errorCode"), &jsonSubObj)){
            jnumber_get_i32(jsonSubObj, &error);
        }

        if (error == -1) {
            LS_LOG_DEBUG("WAN service is not running");
            Notify_ConnectivityStateChange(false);
        }

        j_release(&parsedObj);
        return true;
    }
    LS_LOG_DEBUG("isInternetConnectionAvailable %d", isInternetConnectionAvailable);

    Notify_ConnectivityStateChange(isInternetConnectionAvailable);

    j_release(&parsedObj);
    return true;
}
bool ConnectionStateObserver::_telephony_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret = false;
    bool found = false;
    bool isTelephonyAvailable = false;
    jvalue_ref extendedobj = NULL;
    jvalue_ref jsonSubObj = NULL;
    jvalue_ref parsedObj = NULL;
    JSchemaInfo schemaInfo;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);

    if (!input_schema)
        return true;

    jschema_info_init(&schemaInfo, input_schema, NULL, NULL); // no external refs & no error handlers
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(parsedObj)){
        return true;
    }

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("returnValue"), &jsonSubObj)){
        jboolean_get(jsonSubObj, &ret);
        found = true;
    }

    if (found == false) {
        Notify_TelephonyStateChange(isTelephonyAvailable);
        j_release(&parsedObj);
        return true;
    }

    if (ret == false) {
        int error = -1;

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("errorCode"), &jsonSubObj)){
            jnumber_get_i32(jsonSubObj, &error);
        }

        if (error == -1) {
            LS_LOG_ERROR("Telephony service is not running");
            Notify_TelephonyStateChange(false);
        }

        j_release(&parsedObj);
        return true;
    } else {

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("extended"), &extendedobj)){
            LS_LOG_DEBUG("extendedobj reading power state %s", jvalue_tostring_simple(extendedobj));
            isTelephonyAvailable = jobject_get_exists(extendedobj,  J_CSTR_TO_BUF("power"), &jsonSubObj);
        }

    }

    Notify_TelephonyStateChange(isTelephonyAvailable);
    j_release(&parsedObj);
    return true;
}

bool ConnectionStateObserver::_wifi_service_status_cb(LSHandle*sh, const char *serviceName, bool connected) {
    if(connected) {
        LS_LOG_DEBUG("wifi service Name = %s connected",serviceName);
        register_wifi_status(sh);
    } else {
        LS_LOG_DEBUG("wifi service Name = %s disconnected",serviceName);
    }
    return true;
}

bool ConnectionStateObserver::_wan_service_status_cb(LSHandle *sh, const char *serviceName, bool connected) {
    if(connected) {
        LS_LOG_DEBUG("WAN service Name = %s connected",serviceName);
        register_connectivity_status(sh);
    } else {
        LS_LOG_DEBUG("WAN service Name = %s disconnected",serviceName);
    }
    return true;
}

bool ConnectionStateObserver::_tel_service_status_cb(LSHandle *sh, const char *serviceName, bool connected) {
    if(connected) {
        LS_LOG_DEBUG("telephony service Name = %s connected",serviceName);
        register_telephony_status(sh);
    } else {
        LS_LOG_DEBUG("telephony service Name = %s disconnected",serviceName);
    }
    return true;
}
