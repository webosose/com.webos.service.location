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
 * Filename  : LocationService.h
 * Purpose  :
 * Platform  :
 * Author(s)  :
 * E-mail id. :
 * Creation date :
 *
 * Modifications:
 *
 * Sl No Modified by  Date  Version Description
 *
 **********************************************************/
#ifndef H_LocationService
#define H_LocationService

#include <lunaservice.h>
#include <ServiceAgent.h>
#include<IConnectivityListener.h>
#include <string.h>
#include <Location.h>
#include "boost/shared_ptr.hpp"
#include "boost/array.hpp"
#include <vector>
#include <pthread.h>
#include <LocationService_Log.h>
#include <LunaCriteriaCategoryHandler.h>
#include <pbnjson.h>

#define SHORT_RESPONSE_TIME                 10000
#define MEDIUM_RESPONSE_TIME                100000
#define LONG_RESPONSE_TIME                  150000
#define ACCURACY_LEVEL_HIGH 1
#define ACCURACY_LEVEL_MEDIUM 2
#define ACCURACY_LEVEL_LOW 3
#define RESPONSE_LEVEL_LOW 1
#define RESPONSE_LEVEL_MEDIUM 2
#define RESPONSE_LEVEL_HIGH 3
#define MAX_RESULT_LENGTH 400
#define MAX_GETSTATE_PARAM 32
#define TIME_SCALE_SEC 1000
#define LOCATION_SERVICE_NAME           "com.palm.location"
#define LOCATION_SERVICE_ALIAS_LGE_NAME "com.lge.location"

#define LSERROR_CHECK_AND_PRINT(ret)\
    do {                          \
        if (ret == false) {               \
            LSErrorPrintAndFree(&mLSError);\
            return false; \
        }                 \
    } while (0)

#define LOCATION_ASSERT(cond) \
    do {                    \
        if (!(cond)) {                      \
            LS_LOG_WARNING(#cond ": failed in %s, %s, %d", __FUNCTION__, __FILE__, __LINE__ );    \
            assert(!#cond);                 \
        }                                   \
    } while (0)

/**
 * @brief CONSTANT DEFINITIONS
 */

class LocationService: public IConnectivityListener
{
public:
    class TimerData
    {
    public:
        TimerData(LSMessage *message, LSHandle *sh, unsigned char handlerType) {
            m_message = message;
            m_sh = sh;
            m_handlerType = handlerType;
        }
        LSHandle *GetHandle() const {
            return m_sh;
        }

        LSMessage *GetMessage() const {
            return m_message;
        }

        unsigned char GetHandlerType() const {
            return m_handlerType;
        }

    private:
        LSMessage *m_message;
        LSHandle *m_sh;
        unsigned char m_handlerType;
    };

    class Request
    {
    public:
        Request(LSMessage *message, LSHandle *sh, guint timerID, unsigned char handlerType, TimerData *timerdata) {
            m_msg = message;
            m_sh = sh;
            m_handlerType = handlerType;
            m_timerID = timerID;
            m_timerdata = timerdata;
        }
        LSHandle *GetHandle() const {
            return m_sh;
        }

        LSMessage *GetMessage() const {
            return m_msg;
        }
        guint GetTimerID() const {
            return m_timerID;
        }

        TimerData *GetTimerData() {
            return m_timerdata;
        }

        unsigned char GetReqHandlerType() const {
            return m_handlerType;
        }

        //Multiple handler started, one failed then wait for other reply [START]
        void UpdateReqHandlerType(unsigned char tmp) {
            m_handlerType = tmp;
        }

        //Multiple handler started, one failed then wait for other reply [STOP]
        TimerData *m_timerdata;

    private:
        LSMessage *m_msg;
        LSHandle *m_sh;
        unsigned char m_handlerType;
        guint m_timerID;
    };

    virtual ~LocationService();
    bool init(GMainLoop *);
    bool locationServiceRegister(char *srvcname, GMainLoop *mainLoop, LSPalmService **mServiceHandle);
    static LocationService *getInstance();

    // /**Callback called from Handlers********/
    static void wrapper_getNmeaData_cb(gboolean enable_cb, int64_t timestamp, char *nmea, int length, gpointer privateIns);
    static void wrapper_getCurrentPosition_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int handlerType);
    static void wrapper_startTracking_cb(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error, gpointer privateIns, int type);
    static void wrappergetReverseLocation_cb(gboolean enable_cb, Address *address, gpointer privateIns);
    static void wrapper_getGeoCodeLocation_cb(gboolean enable_cb, Position *position, gpointer privateIns);
    static void wrapper_getGpsSatelliteData_cb(gboolean enable_cb, Satellite *satellite, gpointer privateIns);
    static void wrapper_sendExtraCommand_cb(gboolean enable_cb, int command, gpointer privateIns);
    static void wrapper_gpsStatus_cb(gboolean enable_cb, int state, gpointer data);

    static bool _getNmeaData(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getNmeaData(sh, message, NULL);
    }

    static bool _getCurrentPosition(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getCurrentPosition(sh, message, NULL);
    }

    static bool _startTracking(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->startTracking(sh, message, NULL);
    }

    static bool _getReverseLocation(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getReverseLocation(sh, message, NULL);
    }

    static bool _getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getGeoCodeLocation(sh, message, NULL);
    }

    static bool _getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getAllLocationHandlers(sh, message, NULL);
    }

    static bool _getGpsStatus(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getGpsStatus(sh, message, NULL);
    }

    static bool _setState(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->setState(sh, message, NULL);
    }

    static bool _getState(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getState(sh, message, NULL);
    }

    static bool _sendExtraCommand(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->sendExtraCommand(sh, message, NULL);
    }

    static bool _setGPSParameters(LSHandle *sh, LSMessage *message, void *data){
        return ((LocationService *) data)->setGPSParameters(sh, message, NULL);
    }

    static bool _stopGPS(LSHandle *sh, LSMessage *message, void *data){
        return ((LocationService *) data)->stopGPS(sh, message, NULL);
    }

    static bool _getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getLocationHandlerDetails(sh, message, NULL);
    }

    static bool _getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getGpsSatelliteData(sh, message, NULL);
    }

    static bool _getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getTimeToFirstFix(sh, message, NULL);
    }

    static bool _cancelSubscription(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->cancelSubscription(sh, message, NULL);
    }

    static gboolean TimerCallback(void *data) {
        return getInstance()->_TimerCallback(data);
    }

    bool getHandlerStatus(const char *);
    bool loadHandlerStatus(const char *handler);

    void setServiceState(int state) {
        m_service_state = state;
    }

    LSHandle *getPrivatehandle() {
        return LSPalmServiceGetPrivateConnection(mServiceHandle);
    }

    void setWifiState(bool state) {

        if (state == true) {
            LS_LOG_INFO("WIFI turned on");
        } else {
            LS_LOG_INFO("WIFI turned off");
        }

        wifistate = state;
    }
    bool getWifiState() {
        return wifistate;
    }
    void updateConnectionManagerState(bool state) {

        if (state == true) {
            LS_LOG_INFO("Internet connection available");
        } else {
            LS_LOG_INFO("Internet connection not available");
        }

        isInternetConnectionAvailable = state; //state; for TESTING
    }
    bool getConnectionManagerState() {
        return isInternetConnectionAvailable;
    }

    void updateTelephonyState(bool state) {

        if (state == true) {
            LS_LOG_INFO("Telephony connection available");
        } else {
            LS_LOG_INFO("Telephony connection not available");
        }

        isTelephonyAvailable = state; //state; for TESTING
    }

    bool getTelephonyState() {
        return isTelephonyAvailable;
    }

    void updateWifiInternetState(bool state) {

        if (state == true) {
            LS_LOG_INFO("WIfi Internet connetion available");
        } else {
            LS_LOG_INFO("WIfi Internet connection not available");
        }

        isWifiInternetAvailable = state; //state; for TESTING
    }

    bool getWifiInternetState() {
        return isWifiInternetAvailable;
    }

    void Handle_WifiNotification(bool wifi_state) {
        setWifiState(wifi_state);
    }

    void Handle_ConnectivityNotification(bool Conn_state) {
        updateConnectionManagerState(Conn_state);
    }

    void Handle_TelephonyNotification(bool Tele_state) {
        updateTelephonyState(Tele_state);
    }

    void Handle_WifiInternetNotification(bool Internet_state) {
        updateWifiInternetState(Internet_state);
    }

    bool LSSubscriptionNonSubscriptionRespond(LSPalmService *psh, const char *key, const char *payload, LSError *lserror);
    bool LSSubscriptionNonSubscriptionReply(LSHandle *sh, const char *key, const char *payload, LSError *lserror);
    bool isSubscribeTypeValid(LSHandle *sh, LSMessage *message, bool isMandatory, bool *isSubscription);
    void stopNonSubcription(const char *key);
    bool isSubscListFilled(LSHandle *sh, const char *key,bool cancelCase);

private:
    bool mGpsStatus;
    bool mNwStatus;
    bool mCachedGpsEngineStatus;
    bool mIsGetNmeaSubProgress;
    bool mIsGetSatSubProgress;
    bool mIsStartTrackProgress;
    bool mIsStartFirstReply;
    long long mlastTrackingReplyTime;
    typedef boost::shared_ptr<Request> Requestptr;
    std::vector<Requestptr> m_reqlist;
    bool wifistate;
    bool isInternetConnectionAvailable;
    bool isTelephonyAvailable;
    bool isWifiInternetAvailable;
    int m_service_state;
    static bool instanceFlag;
    static LocationService *locService;
    static LSMethod rootMethod[];
    static LSMethod prvMethod[];
    unsigned char trackhandlerstate;

    LSPalmService *mServiceHandle;
    LSPalmService *mlgeServiceHandle;

    // for calculating time to first fix
    static long mFixRequestTime;
    // time to first fix for most recent session
    static long mTTFF;
    // time we received our last fix
    static long mLastFixTime;
    pthread_mutex_t lbs_geocode_lock;
    pthread_mutex_t lbs_reverse_lock;
    pthread_mutex_t track_lock;
    pthread_mutex_t track_wifi_lock;
    pthread_mutex_t track_cell_lock;
    pthread_mutex_t pos_lock;
    pthread_mutex_t pos_nw_lock;
    pthread_mutex_t nmea_lock;
    pthread_mutex_t sat_lock;
    LunaCriteriaCategoryHandler *criteriaCatSvrc;
    //mapped with HandlerTypes
    Handler *handler_array[HANDLER_MAX];


    LocationService();
    bool getNmeaData(LSHandle *sh, LSMessage *message, void *data);
    bool getCurrentPosition(LSHandle *sh, LSMessage *message, void *data);
    bool startTracking(LSHandle *sh, LSMessage *message, void *data);
    bool getReverseLocation(LSHandle *sh, LSMessage *message, void *data);
    bool getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data);
    bool getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data);
    bool getGpsStatus(LSHandle *sh, LSMessage *message, void *data);
    bool getState(LSHandle *sh, LSMessage *message, void *data);
    bool setState(LSHandle *sh, LSMessage *message, void *data);
    bool sendExtraCommand(LSHandle *sh, LSMessage *message, void *data);
    bool setGPSParameters(LSHandle * sh, LSMessage * message, void * data);
    bool stopGPS(LSHandle * sh, LSMessage * message, void * data);
    bool getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data);
    bool getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data);
    bool getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data);
    bool cancelSubscription(LSHandle *sh, LSMessage *message, void *data);
    gboolean _TimerCallback(void *data);
    bool readLocationfromCache(LSHandle *sh, LSMessage *message, jvalue_ref serviceObject, int maxAge,int accuracylevel, unsigned char handlerstatus);
    bool handler_Selection(LSHandle *sh, LSMessage *message, void *data, int *, int *, unsigned char *,jvalue_ref );
    bool replyAndRemoveFromRequestList(LSHandle *sh, LSMessage *message, unsigned char);
    int  convertResponseTimeFromLevel(int accLevel, int responseTimeLevel);
    bool reqLocationToHandler(int handler_type, unsigned char *reqHandlerType, int subHandlerType, LSHandle *sh);
    bool getCachedDatafromHandler(Handler *hdl, Position *pos, Accuracy *acc, int type);

    void startTracking_reply(Position *pos, Accuracy *acc, int, int);
    void get_nmea_reply(long long timestamp, char *data, int length);
    void getCurrentPosition_reply(Position *, Accuracy *, int, int);
    void getGpsSatelliteData_reply(Satellite *);
    void getGpsStatus_reply(int);
    void stopSubcription(LSHandle *sh, const char *key);
    void LSErrorPrintAndFree(LSError *ptrLSError);
    void criteriaStopSubscription(LSHandle *sh, LSMessage *message);
};

#endif  //  H_LocationService
