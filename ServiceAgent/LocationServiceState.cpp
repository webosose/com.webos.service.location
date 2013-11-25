#include<LocationServiceState.h>

void LocationServiceState::init()
{
    register_wifi_status();
    register_telephony_status();
    register_connectivity_status();
}

void LocationServiceState::register_wifi_status()
{
    LSCall(m_locSrvcPtr->getPrivatehandle(), "palm://com.palm.wifi/getstatus", "{\"subscribe\":true}",
           LocationServiceState::wifi_status_cb, this,
           NULL, NULL);
}

void LocationServiceState::register_connectivity_status()
{
    LSCall(m_locSrvcPtr->getPrivatehandle(), "palm://com.palm.connectionmanager/getstatus", "{\"subscribe\":true}",
           LocationServiceState::connectivity_status_cb, this, NULL, NULL);
}

void LocationServiceState::register_telephony_status()
{
    //Check modem status
    //Not implemented, will be implemented in future
    LSCall(m_locSrvcPtr->getPrivatehandle(), "palm://com.palm.telephony/isTelephonyReady", "{\"subscribe\":true}",
           LocationServiceState::telephony_status_cb, this, NULL, NULL);
}

bool LocationServiceState::wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx)
{
    LS_LOG_DEBUG("response with payload %s", LSMessageGetPayload(message));
    return ((LocationServiceState *) ctx)->_wifi_status_cb(sh, message);
}

bool LocationServiceState::_wifi_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    int error;
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
        this->m_locSrvcPtr->setWifiState(wifistatus);
        json_object_put(root);
        return true;
    }

    //passing NULL to json_object_get_boolean will return false
    ret = json_object_get_boolean(returnValueobj);

    if (ret == false) {
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            this->m_locSrvcPtr->setWifiState(wifistatus);
            register_wifi_status();
        }

        json_object_put(root);
        return true;
    }

    found = json_object_object_get_ex(root, "status", &wifi_status_obj);

    if (found == false) {
        this->m_locSrvcPtr->setWifiState(wifistatus);
        json_object_put(root);
        return true;
    }

    wifisrvEnable = json_object_get_string(wifi_status_obj);

    if ((strcmp(wifisrvEnable, "serviceEnabled") == 0) || (strcmp(wifisrvEnable, "connectionStateChanged") == 0)) {
        wifistatus = true;
    } else if (strcmp(wifisrvEnable, "serviceDisabled") == 0) {
        wifistatus = false;
    }

    this->m_locSrvcPtr->setWifiState(wifistatus);
    json_object_put(root);
    return true;
}

/*luna-send -i palm://com.palm.connectionmanager/getstatus '{}'
 {"returnValue":true,"isInternetConnectionAvailable":false,"wired":{"state":"disconnected"},"wifi":{"state":"connected","interfaceName":"wlan0","ipAddress":"192.168.207.11
 9","netmask":"255.255.248.0","gateway":"192.168.200.1","dns1":"8.8.8.8","dns2":"4.2.2.2","method":"dhcp","ssid":"LGSI-TEST","isWakeOnWifiEnabled":false,"onInternet":"no"}
 }*/

bool LocationServiceState::_connectivity_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    bool isInternetConnectionAvailable = false;
    int error;
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
        this->m_locSrvcPtr->updateConntionManagerState(isInternetConnectionAvailable);
        json_object_put(root);
        return true;
    }

    ret = json_object_get_boolean(returnValueobj);

    if (ret) {
        found = json_object_object_get_ex(root, "isInternetConnectionAvailable", &isConnectedobj);

        if (found == false) {
            this->m_locSrvcPtr->updateConntionManagerState(isInternetConnectionAvailable);
            json_object_put(root);
            return true;
        }

        isInternetConnectionAvailable = json_object_get_boolean(isConnectedobj);
    } else {
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            register_connectivity_status();
        }

        json_object_put(root);
    }

    this->m_locSrvcPtr->updateConntionManagerState(isInternetConnectionAvailable);
    json_object_put(root);
    return true;
}
bool LocationServiceState::_telephony_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool found;
    bool isTelephonyAvailable = false;
    bool powerstate;
    const char *str;
    int error;
    json_object *root = 0;
    json_object *returnValueobj = 0;
    json_object *isConnectedobj = 0;
    json_object *errorobj = 0;
    json_object *extendedobj = 0;
    json_object *powerobj = 0;
    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);

    if (root == NULL || is_error(root)) {
        return true;
    }

    found = json_object_object_get_ex(root, "returnValue", &returnValueobj);

    if (found == false) {
        this->m_locSrvcPtr->updateTelephonyState(isTelephonyAvailable);
        json_object_put(root);
        return true;
    }

    ret = json_object_get_boolean(returnValueobj);

    if (ret == false) {
        errorobj = json_object_object_get(root, "errorCode");
        error = json_object_get_int(errorobj);

        if (error == -1) {
            register_telephony_status();
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

    this->m_locSrvcPtr->updateTelephonyState(isTelephonyAvailable);
    json_object_put(root);
    return true;
}
