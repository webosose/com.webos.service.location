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

#ifndef NETWORKPROVIDER_H_
#define NETWORKPROVIDER_H_

#include <time.h>
#include <math.h>
#include <functional>
#include <map>
#include <string>
#include <loc_http.h>
#include <loc_log.h>
#include <pbnjson.h>
#include <lunaservice.h>
#include <NetworkData.h>
#include <HttpInterface.h>
#include <NetworkRequestManager.h>
#include <PositionProviderInterface.h>
#include <IConnectivityListener.h>

class NetworkDataClient;

typedef struct NetworkPositionData {
    const char *nwGeolocationKey;
    char *cellInfo;
    gboolean cellChanged;
    int32_t curCellid;
    double lastLatitude;
    double lastLongitude;
    double lastAccuracy;
    request_state_cellid_type requestState;
    GHashTable *wifiAccessPointsList;
    GHashTable *lastAps;
    int64_t lastTimeStamp;

    NetworkPositionData() {
        lastLatitude = 0;
        lastLongitude = 0;
        lastAccuracy = 0;
        lastTimeStamp = 0;
        lastAps = NULL;
        cellInfo = NULL;
        cellChanged = false;
        curCellid = 0;
        nwGeolocationKey = NULL;
        requestState = REQUEST_NONE;
        wifiAccessPointsList = NULL;
    }

    ~NetworkPositionData() {
        delete nwGeolocationKey;
        delete cellInfo;

        if (lastAps)
            g_hash_table_destroy(lastAps);

        if (wifiAccessPointsList)
            g_hash_table_destroy(wifiAccessPointsList);
    }

    void resetData() {
        lastLatitude = 0;
        lastLongitude = 0;
        lastAccuracy = 0;
        lastTimeStamp = 0;

        if (lastAps) {
            g_hash_table_destroy(lastAps);
            lastAps = NULL;
        }

        if (wifiAccessPointsList) {
            g_hash_table_destroy(wifiAccessPointsList);
            wifiAccessPointsList = NULL;
        }

        if (cellInfo) {
            g_free(cellInfo);
            cellInfo = NULL;
        }

        cellChanged = false;
        curCellid = 0;

        requestState = REQUEST_NONE;
    }
} _NetworkPositionData;

class NetworkPositionProvider final
        : public PositionProviderInterface,
          public HttpInterface,
          public NetworkDataClient,
          public IConnectivityListener {

public:

    NetworkPositionProvider(LSHandle *sh);

    ~NetworkPositionProvider() { }

    void enable();

    void disable();

    ErrorCodes processRequest(PositionRequest request);

    ErrorCodes getLastPosition(Position *position, Accuracy *accuracy);

    void Handle_WifiNotification(bool);

    void Handle_ConnectivityNotification(bool status);

    void Handle_TelephonyNotification(bool status);

    void Handle_WifiInternetNotification(bool status);

    void Handle_SuspendedNotification(bool status);

private:

    static bool serviceStatusCb(LSHandle *sh, const char *serviceName, bool connected, void *ctx);

    static bool scanCb(LSHandle *sh, LSMessage *reply, void *ctx);

    static gboolean networkUpdateCallback(void *obj);

    int parseHTTPResponse(char *body, double *latitude, double *longitude, double *accuracy);

    char *readApiKey();

    char *updateCellData();

    char *createCellWifiCombinedQuery();

    char *createCellQuery();

    char *createWifiQuery();

    bool networkPostQuery(char *post_data, const char *apikey, gboolean sync, LSMessage *message);

    void handleResponse(HttpReqTask *task);

    void onUpdateCellData(char *cellData, LSMessage *message);

    void onUpdateWifiData(GHashTable *wifiAccessPoints, LSMessage *message);

    bool registerServiceStatus(const char *service, void **cookie, LSServerStatusFunc cb);

    void wifiServiceStatusUpdate(bool connected);

    void telephonyServiceStatusUpdate(bool connected);

    bool unregisterServiceStatus(void *cookie);

    bool lunaServiceCall(const char *method, const char *payload, LSFilterFunc cb, LSMessageToken *token,
                         bool oneReply);

    bool triggerPostQuery(LSMessage *message);


private:

    NetworkData mNetworkData;
    _NetworkPositionData mPositionData;
    LSHandle *mLSHandle;
    guint mTimeoutId;
    bool mProcessRequestInProgress;
    void *mTelephonyCookie;
    void *mWifiCookie;
    LSMessageToken mScanToken;
    bool mtelephonyPowerd;
    bool mwifiStatus;
    bool mConnectivityStatus;
    uint32_t clientToken;
};

#endif // NETWORKPROVIDER_H_