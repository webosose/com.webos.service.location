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
#include<ConnectionStateObserver.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <string>
#include <boost/foreach.hpp>
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
    /*  LSHandle *ConnHandle;
     BOOST_FOREACH(IConnectivityListener *l, m_listeners)
         {
         (ConnHandle = l->getPrivatehandle());
      }*/
    LS_LOG_DEBUG("init\n");

    if (ConnHandle == NULL)
        return;

    register_wifi_status(ConnHandle);
    register_telephony_status(ConnHandle);
    register_connectivity_status(ConnHandle);
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
void ConnectionStateObserver::register_wifi_status(LSHandle *HandleConn)
{
    LSCall(HandleConn, "palm://com.palm.wifi/getstatus", "{\"subscribe\":true}", ConnectionStateObserver::wifi_status_cb, this, NULL, NULL);
}

void ConnectionStateObserver::register_connectivity_status(LSHandle *HandleConn)
{
    LSCall(HandleConn, "palm://com.palm.wan/getstatus", "{\"subscribe\":true}", ConnectionStateObserver::connectivity_status_cb, this,
            NULL, NULL);
}

void ConnectionStateObserver::register_telephony_status(LSHandle *HandleConn)
{
    //Check modem status
    //Not implemented, will be implemented in future
    LSCall(HandleConn, "palm://com.palm.telephony/isTelephonyReady", "{\"subscribe\":true}", ConnectionStateObserver::telephony_status_cb, this, NULL,
            NULL);
}

bool ConnectionStateObserver::wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx)
{
    g_print("response with payload %s\n", LSMessageGetPayload(message));
    return ((ConnectionStateObserver *) ctx)->_wifi_status_cb(sh, message);
}

bool ConnectionStateObserver::_wifi_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    const char *str;
    char *wifisrvEnable;
    json_object *root = 0;
    json_object *returnValueobj = 0;
    json_object *wifi_status_obj = 0;
    json_object *errorobj = 0;
    bool wifistatus = false;
    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);

    if (root == NULL || is_error(root)) {
        return true;
    }

    found = json_object_object_get_ex(root, "returnValue", &returnValueobj);

    if (found == false) {
        Notify_WifiStateChange(wifistatus);
        json_object_put(root);
        return true;
    }

    //passing NULL to json_object_get_boolean will return false
    ret = json_object_get_boolean(returnValueobj);

    if (ret == false) {
        int error;
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            Notify_WifiStateChange(wifistatus);
            register_wifi_status(sh);
        }

        json_object_put(root);
        return true;
    }

    found = json_object_object_get_ex(root, "status", &wifi_status_obj);

    if (found == false) {
        Notify_WifiStateChange(wifistatus);
        json_object_put(root);
        return true;
    }

    wifisrvEnable = json_object_get_string(wifi_status_obj);

    if ((strcmp(wifisrvEnable, "serviceEnabled") == 0)) {
        wifistatus = true;
        Notify_WifiStateChange(wifistatus);
    } else if (strcmp(wifisrvEnable, "serviceDisabled") == 0) {
        wifistatus = false;
        Notify_WifiStateChange(wifistatus);
    } else if (strcmp(wifisrvEnable, "connectionStateChanged") == 0) {
        Notify_WifiInternetStateChange(true);
    } else {
        Notify_WifiInternetStateChange(true);
        Notify_WifiStateChange(wifistatus);
    }

    json_object_put(root);
    return true;
}

/*luna-send -i palm://com.palm.connectionmanager/getstatus '{}'
 {"returnValue":true,"isInternetConnectionAvailable":false,"wired":{"state":"disconnected"},"wifi":{"state":"connected","interfaceName":"wlan0","ipAddress":"192.168.207.11
 9","netmask":"255.255.248.0","gateway":"192.168.200.1","dns1":"8.8.8.8","dns2":"4.2.2.2","method":"dhcp","ssid":"LGSI-TEST","isWakeOnWifiEnabled":false,"onInternet":"no"}
 }*/

bool ConnectionStateObserver::_connectivity_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    char *isInternetConnectionAvailable = NULL;
    const char *str;
    json_object *root = 0;
    json_object *returnValueobj = 0;
    json_object *isConnectedobj = 0;
    json_object *errorobj = 0;
    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);

    if (root == NULL || is_error(root)) {
        return true;
    }

    found = json_object_object_get_ex(root, "returnValue", &returnValueobj);

    if (found == false) {
        Notify_ConnectivityStateChange(isInternetConnectionAvailable);
        json_object_put(root);
        return true;
    }

    ret = json_object_get_boolean(returnValueobj);

    if (ret) {
        found = json_object_object_get_ex(root, "isInternetConnectionAvailable", &isConnectedobj);

        if (found == false) {
            Notify_ConnectivityStateChange(false);
            json_object_put(root);
            return true;
        }

        isInternetConnectionAvailable = json_object_get_string(isConnectedobj);
    } else {
        int error;
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            register_connectivity_status(sh);
        }

        json_object_put(root);
        return true;
    }
    LS_LOG_DEBUG("isInternetConnectionAvailable %s", isInternetConnectionAvailable);
    if (isInternetConnectionAvailable && (strcmp(isInternetConnectionAvailable, "yes") == 0))
        Notify_ConnectivityStateChange(true);
    else
        Notify_ConnectivityStateChange(false);
    json_object_put(root);
    return true;
}
bool ConnectionStateObserver::_telephony_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    bool isTelephonyAvailable = false;
    const char *str;
    json_object *root = 0;
    json_object *returnValueobj = 0;
    json_object *errorobj = 0;
    json_object *extendedobj = 0;
    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);

    if (root == NULL || is_error(root)) {
        return true;
    }

    found = json_object_object_get_ex(root, "returnValue", &returnValueobj);

    if (found == false) {
        Notify_TelephonyStateChange(isTelephonyAvailable);
        json_object_put(root);
        return true;
    }

    ret = json_object_get_boolean(returnValueobj);

    if (ret == false) {
        int error;
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            register_telephony_status(sh);
        }

        json_object_put(root);
        return true;
    } else {
        extendedobj = json_object_object_get(root, "extended");

        if (json_object_array_length(extendedobj) > 0) {
            LS_LOG_DEBUG("extendedobj reading power state %s", json_object_to_json_string(extendedobj));
            isTelephonyAvailable = json_object_object_get(extendedobj, "power");
        }
    }

    Notify_TelephonyStateChange(isTelephonyAvailable);
    json_object_put(root);
    return true;
}
