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
#include <cjson/json.h>
#include <pthread.h>
#include <LocationService_Log.h>
#define SHORT_RESPONSE_TIME  10000;
#define MEDIUM_RESPONSE_TIME  100000;
#define LONG_RESPONSE_TIME  150000;
#define ACCURACY_LEVEL_HIGH 1
#define ACCURACY_LEVEL_MEDIUM 2
#define ACCURACY_LEVEL_LOW 3
#define RESPONSE_LEVEL_LOW 1
#define RESPONSE_LEVEL_MEDIUM 2
#define RESPONSE_LEVEL_HIGH 3
#define MAX_RESULT_LENGTH 400
#define LSERROR_CHECK_AND_PRINT(ret)\
    if(ret == false) \
    { \
        LSErrorPrintAndFree(&mLSError);\
        return false; \
    }
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
        ;
        LSMessage *GetMessage() const {
            return m_message;
        }
        ;
        unsigned char GetHandlerType() const {
            return m_handlerType;
        }
        ;
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
        ;
        LSMessage *GetMessage() const {
            return m_msg;
        }
        ;
        guint GetTimerID() const {
            return m_timerID;
        }
        ;
        TimerData *GetTimerData() {
            return m_timerdata;
        }
        ;
        unsigned char GetReqHandlerType() const {
            return m_handlerType;
        }
        ;
        //Multiple handler started, one failed then wait for other reply [START]
        void UpdateReqHandlerType(unsigned char tmp) {
            m_handlerType = tmp;
        }
        ;
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
    static int method;

    enum SERVICE_STATE {
        SERVICE_NOT_RUNNING = 0,
        SERVICE_GETTING_READY,
        SERVICE_READY
    };

    void startCellIDHandler();
    static bool _getNmeaData(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getNmeaData(sh, message, NULL);
    }
    static bool _getCurrentPosition(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getCurrentPosition(sh, message, NULL);
    }
    static bool _startTracking(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->startTracking(sh, message, NULL);
    }
    static bool _startTrackingBestPosition(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->startTrackingBestPosition(sh, message, NULL);
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
    static bool _SetGpsStatus(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->SetGpsStatus(sh, message, NULL);
    }
    static bool _GetGpsStatus(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->GetGpsStatus(sh, message, NULL);
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

    static bool _getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getLocationHandlerDetails(sh, message, NULL);
    }

    static bool _getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getGpsSatelliteData(sh, message, NULL);
    }
    static bool _getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getTimeToFirstFix(sh, message, NULL);
    }
    static bool _test(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->test(sh, message, NULL);
    }
    static bool _cancelSubscription(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->cancelSubscription(sh, message, NULL);
    }

    /**** Callbacks from C********************************/

    static void start_tracking_cb(Position *pos, Accuracy *accuracy, int error, gpointer privateIns, int type) {
        getInstance()->startTracking_reply(pos, accuracy, error, type);
    }
    static void get_nmea_cb(gboolean enable_cb, int timestamp, char *data, int length, gpointer privateIns) {
        //((LocationService*)privateIns)->get_nmea_reply(timestamp,data,length);
        getInstance()->get_nmea_reply(timestamp, data, length);
    }

    static void getCurrentPosition_cb(Position *pos, Accuracy *accuracy, int error, gpointer privateIns, int handlerType) {
        getInstance()->getCurrentPosition_reply(pos, accuracy, error, handlerType);
    }

    static void getGpsSatelliteData_cb(gboolean enable_cb, Satellite *satellite, gpointer privateIns) {
        getInstance()->getGpsSatelliteData_reply(satellite);
    }

    static void getGpsStatus_cb(int state) {
        getInstance()->getGpsStatus_reply(state);
    }

    static int CreateSharedPref();
    static bool GetWifiApsList_cb(LSHandle *sh, LSMessage *reply, void *ctx) {
        return getInstance()->_GetWifiApsList_cb(sh, reply, ctx);
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
            LS_LOG_DEBUG("WIFI turned on");
        } else {
            LS_LOG_DEBUG("WIFI turned off");
        }

        wifistate = state;
    }
    void updateConntionManagerState(bool state) {
        if (state == true) {
            LS_LOG_DEBUG("Internet connetion available");
        } else {
            LS_LOG_DEBUG("Internet connection not available");
        }

        isInternetConnectionAvailable = state; //state; for TESTING
        //TODO START HANDLER WHEN INTERNET CONNECTION IS ENABLED
        /*if(isInternetConnectionAvailable && getHandlerStatus(NETWORK))
         startCellIDHandler();*/
    }
    void updateTelephonyState(bool state) {
        if (state == true) {
            LS_LOG_DEBUG("Telephony connetion available");
        } else {
            LS_LOG_DEBUG("Telephony connection not available");
        }

        isTelephonyAvailable = state; //state; for TESTING
    }

    void LocationService::Handle_WifiNotification(bool wifi_state) {
        setWifiState(wifi_state);
    }
    void LocationService::Handle_ConnectivityNotification(bool Conn_state) {
        updateConntionManagerState(Conn_state);
    }
    void LocationService::Handle_TelephonyNotification(bool Tele_state) {
        updateTelephonyState(Tele_state);
    }

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
    LocationService();
    //mapped with HandlerTypes
    Handler *handler_array[HANDLER_MAX];

    char* locationErrorText[LOCATION_MAXIMUM] {
    "Success" , //LOCATION_SUCCESS
    "TimeOut", //LOCATION_TIMEOUT,
    "Handler Not Available", //GET_STATE_HANDLER_NOT_AVAILABLE,
    "Invalid input" , //GET_STATE_HANDLER_INVALID_INPUT,
    "Unknown error", //GET_STATE_UNKNOWN_ERROR
    "Out of Memory" //GET_STATE_OUT_OF_MEM
    };

    char* trakingErrorText[TRACKING_MAXIMUM] {
    "Success" , //TRACKING_SUCCESS
    "TimeOut", //TRACKING_TIMEOUT,
    "Position Not Available", //TRACKING_POS_NOT_AVAILABLE,
    "Unknown error" , //TRACKING_UNKNOWN_ERROR,
    "Gps Not supported", //TRACKING_GPS_UNAVAILABLE,
    "Location Source is off", //TRACKING_LOCATION_OFF,
    "Request Pending" , //TRACKING_PENDING_REQ,
    "App is blacklisted", //TRACKING_APP_BLACK_LISTED,
    "Handler start failure", //HANDLER_START_FAILURE,
    "state unknown",//STATE_UNKNOWN
    "Invalid Input"
    };


    //ServiceAgentProp *service_agent_prop;
    bool getNmeaData(LSHandle *sh, LSMessage *message, void *data);
    bool getCurrentPosition(LSHandle *sh, LSMessage *message, void *data);
    bool startTracking(LSHandle *sh, LSMessage *message, void *data);
    bool startTrackingBestPosition(LSHandle *sh, LSMessage *message, void *data);
    bool getReverseLocation(LSHandle *sh, LSMessage *message, void *data);
    bool getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data);
    bool getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data);
    bool SetGpsStatus(LSHandle *sh, LSMessage *message, void *data);
    bool GetGpsStatus(LSHandle *sh, LSMessage *message, void *data);
    bool getState(LSHandle *sh, LSMessage *message, void *data);
    bool setState(LSHandle *sh, LSMessage *message, void *data);
    bool sendExtraCommand(LSHandle *sh, LSMessage *message, void *data);
    bool getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data);
    bool getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data);
    bool getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data);
    bool test(LSHandle *sh, LSMessage *message, void *data);
    bool cancelSubscription(LSHandle *sh, LSMessage *message, void *data);
    bool _GetWifiApsList_cb(LSHandle *sh, LSMessage *reply, void *ctx);
    gboolean _TimerCallback(void *data);
    bool readLocationfromCache(LSHandle *sh, LSMessage *message, json_object *serviceObject, int maxAge, int accuracy, unsigned char handlerstatus);
    bool handler_Selection(LSHandle *sh, LSMessage *message, void *data, int *, int *, unsigned char *);
    bool replyAndRemoveFromRequestList(LSHandle *sh, LSMessage *message, unsigned char);
    int ConvetResponseTimeFromLevel(int accLevel, int responseTimeLevel);
    bool reqLocationToHandler(int handler_type, unsigned char *reqHandlerType, int subHandlerType, LSHandle *sh);
    bool getCachedDatafromHandler(Handler *hdl, Position *pos, Accuracy *acc, int type);
    void getServiceListString(char *);
    int geocluecall(double *, double *);

    void startTracking_reply(Position *pos, Accuracy *acc, int, int);
    void startTrackingBestPosition_reply(struct json_object *, Accuracy *acc);
    void get_nmea_reply(int timestamp, char *data, int length);
    void getCurrentPosition_reply(Position *, Accuracy *, int, int);
    void getGpsSatelliteData_reply(Satellite *);
    void getGpsStatus_reply(int);
    void stopSubcription(LSHandle *sh, const char *key);
    void LocationService::stopNonSubcription(const char *key);
    bool isSubscListEmpty(LSHandle *sh, const char *key);
    bool LSSubscriptionNonSubscriptionRespond(LSPalmService *psh, const char *key, const char *payload, LSError *lserror);
    bool LSSubscriptionNonSubscriptionReply(LSHandle *sh, const char *key, const char *payload, LSError *lserror);
    void LSErrorPrintAndFree(LSError *ptrLSError);
    void startWifiHandler(void);
    bool isSubscribeTypeValid(LSHandle *sh, LSMessage *message);

    int m_service_state;
    static bool instanceFlag;
    static LocationService *locService;
    static LSMethod rootMethod[];
    static LSMethod jsMethod[];
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
};

#endif  //  H_LocationService
