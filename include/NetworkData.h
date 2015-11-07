// @@@LICENSE
//
//      Copyright (c) 2015 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef NETWORKDATA_H_
#define NETWORKDATA_H_

#include <lunaservice.h>
#include <glib.h>
#include <loc_log.h>

class NetworkDataClient {

public:

    NetworkDataClient() {
        LS_LOG_DEBUG("Ctor NetworkDataClient");
    }

    virtual ~NetworkDataClient() {
        LS_LOG_DEBUG("Dtor NetworkDataClient");
    }

    virtual void onUpdateCellData(char *cellData, LSMessage *message) = 0;

    virtual void onUpdateWifiData(GHashTable *wifiAccessPoints, LSMessage *message) = 0;
};

class NetworkData {

public:

    NetworkData(LSHandle *sh);

    ~NetworkData();

    void setNetworkDataClient(NetworkDataClient *client);

    bool registerForCellInfo();

    bool registerForWifiAccessPoints();

    bool unregisterForCellInfo();

    bool unregisterForWifiAccessPoints();

private:

    static bool cellDataCb(LSHandle *sh, LSMessage *message, void *ctx);

    static bool wifiAccessPointsCb(LSHandle *sh, LSMessage *message, void *ctx);

    bool cellDataCb(LSHandle *sh, LSMessage *message);

    bool wifiAccessPointsCb(LSHandle *sh, LSMessage *message);

private:

    LSHandle *mPrvHandle;
    LSMessageToken mCellInfoReq;
    LSMessageToken mWifiInfoReq;
    NetworkDataClient *mClient;
};

#endif // NETWORKDATA_H_
