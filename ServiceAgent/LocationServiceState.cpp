#include<LocationServiceState.h>


void LocationServiceState::init()
{
    register_wifi_status();
    register_modem_status();
    register_connectivity_status();

}

void LocationServiceState::register_wifi_status()
{
    LSCall(m_locSrvcPtr->getPrivatehandle(),"palm://com.palm.wifi/getstatus","{\"subscribe\":true}",LocationServiceState::wifi_status_cb,this,NULL,NULL);
}



void LocationServiceState::register_connectivity_status()
{
    LSCall(m_locSrvcPtr->getPrivatehandle(),"palm://com.palm.connectionmanager/getstatus","{\"subscribe\":true}",LocationServiceState::connectivity_status_cb,this,NULL,NULL);
}


void LocationServiceState::register_modem_status()
{
    //Check modem status
    //Not implemented, will be implemented in future
}

bool LocationServiceState::wifi_status_cb(LSHandle *sh, LSMessage *message, void *ctx)
{
    LS_LOG_DEBUG("response with payload %s",LSMessageGetPayload(message));
    return ((LocationServiceState *)ctx)->_wifi_status_cb(sh,message);
}

bool LocationServiceState::_wifi_status_cb(LSHandle *sh, LSMessage *message)
{
    int ret;
    const char* str;
    char* wifisrvEnable;
    json_object* root = 0;
    json_object* returnValueobj = 0;
    json_object*  wifi_status_obj= 0;
    bool wifistatus = false;

    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);
    returnValueobj = json_object_object_get(root,"returnValue");

    //passing NULL to json_object_get_boolean will return false
    ret = json_object_get_boolean(returnValueobj);
    if(ret == false)
    {
        LS_LOG_DEBUG("Wifi is OFF\n");
        wifistatus = false;
        this->m_locSrvcPtr->setWifiState(wifistatus);
        return true;
    }
    wifi_status_obj = json_object_object_get(root, "status");

    if(wifi_status_obj == NULL)
    {
        this->m_locSrvcPtr->setWifiState(wifistatus);
        return true;
    }

    wifisrvEnable = json_object_get_string(wifi_status_obj);
    if(wifisrvEnable != NULL && (strcmp(wifisrvEnable,"serviceEnabled") == 0))
    {
        LS_LOG_DEBUG("wifi is TURNED ON");
        wifistatus = true;
    }
    else
    {
        wifistatus = false;
    }
    this->m_locSrvcPtr->setWifiState(wifistatus);
    return true;
}

/*luna-send -i palm://com.palm.connectionmanager/getstatus '{}'
  {"returnValue":true,"isInternetConnectionAvailable":false,"wired":{"state":"disconnected"},"wifi":{"state":"connected","interfaceName":"wlan0","ipAddress":"192.168.207.11
  9","netmask":"255.255.248.0","gateway":"192.168.200.1","dns1":"8.8.8.8","dns2":"4.2.2.2","method":"dhcp","ssid":"LGSI-TEST","isWakeOnWifiEnabled":false,"onInternet":"no"}
  }*/

bool LocationServiceState::_connectivity_status_cb(LSHandle *sh, LSMessage *message)
{
    bool ret;
    bool isInternetConnectionAvailable = false;
    const char* str;
    json_object* root = 0;
    json_object* returnValueobj = 0;
    json_object* isConnectedobj = 0;

    str = LSMessageGetPayload(message);
    root = json_tokener_parse(str);
    returnValueobj = json_object_object_get(root,"returnValue");

    ret = json_object_get_boolean(returnValueobj);
    if(ret)
    {
        isConnectedobj = json_object_object_get(root,"isInternetConnectionAvailable");
        isInternetConnectionAvailable = json_object_get_boolean(isConnectedobj);
    }
    this->m_locSrvcPtr->updateConntionManagerState(isInternetConnectionAvailable);
    return true;
}
