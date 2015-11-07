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
#ifndef _ICONNECTIVITY_LISTENER_H_
#define _ICONNECTIVITY_LISTENER_H_


class IConnectivityListener {
public:
    virtual void Handle_WifiNotification(bool) = 0;

    virtual void Handle_ConnectivityNotification(bool) = 0;

    virtual void Handle_TelephonyNotification(bool) = 0;

    virtual void Handle_WifiInternetNotification(bool) = 0;

    virtual void Handle_SuspendedNotification(bool) = 0;
};

#endif
