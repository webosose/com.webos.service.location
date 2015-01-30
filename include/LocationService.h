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
#include <loc_log.h>
#include <pbnjson.h>
#include <sys/time.h>
#include <LifeCycleMonitor.h>

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

#define GEOFENCE_ENTERED                    0
#define GEOFENCE_EXITED                     1
#define GEOFENCE_UNCERTAIN                  2
#define GEOFENCE_UNAVAILABLE                (1<<0L)
#define GEOFENCE_AVAILABLE                  (1<<1L)
#define GEOFENCE_OPERATION_SUCCESS          0
#define GEOFENCE_ERROR_TOO_MANY_GEOFENCES   -100
#define GEOFENCE_ERROR_ID_EXISTS            -101
#define GEOFENCE_ERROR_ID_UNKNOWN           -102
#define GEOFENCE_ERROR_INVALID_TRANSITION   -103
#define GEOFENCE_ERROR_GENERIC              -149

#define MAX_GEOFENCE_ID                     200
#define MIN_GEOFENCE_RANGE                  1000
#define MAX_GEOFENCE_RANGE                  1200
#define MAX_API_KEY_LENGTH                  256
#define KEY_MAX                             64
#define MATH_PI                             3.1415926535897932384626433832795

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
    static const int GETLOC_UPDATE_NW = 0;
    static const int GETLOC_UPDATE_GPS_NW = 1;
    static const int GETLOC_UPDATE_GPS = 2;
    static const int GETLOC_UPDATE_PASSIVE = 3;
    static const int GETLOC_UPDATE_INVALID =4;

    static const double INVALID_LAT = 0;
    static const double INVALID_LONG = 0;
    class TimerData
    {
    public:
        TimerData(LSMessage *message, LSHandle *sh, unsigned char handlerType, const char *argkey) {
            m_message = message;
            m_sh = sh;
            m_handlerType = handlerType;
            timerStart = true;
            if (argkey != NULL)
                memcpy(key, argkey, strlen(argkey)+1);
            else
                memset(key,0x00,KEY_MAX);
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
        void stopTimer() {
            timerStart = false;
        }
        bool getTimerStatus() {
            return timerStart;
        }
        const char *getKey() {
            return key;
        }

    private:
        LSMessage *m_message;
        LSHandle *m_sh;
        unsigned char m_handlerType;
        bool timerStart;
        char key[KEY_MAX];
    };

    class LocationUpdateRequest
    {
    public:
        LocationUpdateRequest(LSMessage *msg, long long reqTime,guint timerID,TimerData *timerData,double latitude, double longitude, int hander_type) {
            m_message = msg;
            m_requestTime = reqTime;
            m_lastlat = latitude;
            m_lastlong = longitude;
            m_isFirstReply = true;
            m_timerdata = timerData;
            m_timerID = timerID;
            m_handler_type = hander_type;
        }
        LSMessage *getMessage() {
            return m_message;
        }
        long long getRequestTime() {
            return m_requestTime;
        }
        void updateRequestTime(long long currentTime) {
            m_requestTime = currentTime;
        }
        double getLatitude() {
            return m_lastlat;
        }
        double getLongitude() {
            return m_lastlong;
        }

        guint getTimerID() const {
            return m_timerID;
        }
        void updateLatAndLong(double latitude,double longitude) {
            m_lastlat = latitude;
            m_lastlong = longitude;
        }
        bool getFirstReply() {
            return m_isFirstReply;
        }
        bool updateFirstReply(bool firstreply) {
            m_isFirstReply = firstreply;
        }

        TimerData *getTimerData() {
            return m_timerdata;
        }

        void setTimerData(TimerData *timerData) {
            m_timerdata = timerData;
        }
        int getHandlerType() {
            return m_handler_type;
        }
    private:
        LSMessage *m_message;
        long long m_requestTime;
        double m_lastlong;
        double m_lastlat;
        bool m_isFirstReply;
        guint m_timerID;
        TimerData *m_timerdata;
        int m_handler_type;
    };
    virtual ~LocationService();
    bool init(GMainLoop *);
    bool locationServiceRegister(const char *srvcname, GMainLoop *mainLoop, LSPalmService **mServiceHandle);
    static LocationService *getInstance();

    bool isLocationRequestEmpty() {
        bool ret = !(handler_get_handler_status(handler_array[HANDLER_NW], HANDLER_WIFI) ||
                     handler_get_handler_status(handler_array[HANDLER_GPS], HANDLER_GPS) ||
                     handler_get_handler_status(handler_array[HANDLER_NW], HANDLER_CELLID) ||
                     handler_get_handler_status(handler_array[HANDLER_NW], HANDLER_LBS));

        LS_LOG_INFO("isLocationRequestEmpty ret %d\n", ret);

        return ret;
    }

    // /**Callback called from Handlers********/
    static void wrapper_getNmeaData_cb(gboolean enable_cb, int64_t timestamp, char *nmea, int length, gpointer privateIns);
    static void wrapper_rev_geocoding_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type);
    static void wrapper_geocoding_cb(gboolean enable_cb, char *response, int error, gpointer userdata, int type);
    static void wrappergetReverseLocation_cb(gboolean enable_cb, Address *address, gpointer privateIns);
    static void wrapper_getGeoCodeLocation_cb(gboolean enable_cb, Position *position, gpointer privateIns);
    static void wrapper_getGpsSatelliteData_cb(gboolean enable_cb, Satellite *satellite, gpointer privateIns);
    static void wrapper_sendExtraCommand_cb(gboolean enable_cb, int command, gpointer privateIns);
    static void wrapper_gpsStatus_cb(gboolean enable_cb, int state, gpointer data);
    static void wrapper_geofence_add_cb(int32_t geofence_id, int32_t status, gpointer user_data);
    static void wrapper_geofence_remove_cb(int32_t geofence_id, int32_t status, gpointer user_data);
    static void wrapper_geofence_pause_cb(int32_t geofence_id, int32_t status, gpointer user_data);
    static void wrapper_geofence_resume_cb(int32_t geofence_id, int32_t status, gpointer user_data);
    static void wrapper_geofence_breach_cb(int32_t geofence_id, int32_t status, int64_t timestamp, double latitude, double longitude, gpointer user_data);
    static void wrapper_geofence_status_cb (int32_t status, Position *last_position, Accuracy *accuracy, gpointer user_data);
    static void wrapper_getLocationUpdate_cb(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error,gpointer privateIns, int type);

    static bool _getNmeaData(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getNmeaData(sh, message, NULL);
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

    static bool _exitLocation(LSHandle *sh, LSMessage *message, void *data){
        return ((LocationService *) data)->exitLocation(sh, message, NULL);
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

    static bool _getLocationUpdates(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getLocationUpdates(sh, message, NULL);
    }

    static bool _getCachedPosition(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getCachedPosition(sh, message, NULL);
    }

    static bool _cancelSubscription(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->cancelSubscription(sh, message, NULL);
    }

    static bool _addGeofenceArea(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->addGeofenceArea(sh, message, NULL);
    }

    static bool _getGeofenceStatus(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->getGeofenceStatus(sh, message, NULL);
    }

    static bool _pauseGeofence(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->pauseGeofence(sh, message, NULL);
    }

    static bool _resumeGeofence(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->resumeGeofence(sh, message, NULL);
    }

    static bool _removeGeofenceArea(LSHandle *sh, LSMessage *message, void *data) {
        return ((LocationService *) data)->removeGeofenceArea(sh, message, NULL);
    }

    static gboolean TimerCallbackLocationUpdate(void *data) {
        return getInstance()->_TimerCallbackLocationUpdate(data);
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

    void updateNwKey(const char *nwKey) {

        if(nwKey && (strlen(nwKey) < MAX_API_KEY_LENGTH))
           strcpy(nwGeolocationKey, nwKey);
    }

    void updateLbsKey(const char *lbsKey) {

        if(lbsKey && (strlen(lbsKey) < MAX_API_KEY_LENGTH))
           strcpy(lbsGeocodeKey, lbsKey);
    }
    // For deprecated API once depercated API removed this will also be removed
    char* getNwKey() {
        return nwGeolocationKey;
    }


    bool LSSubscriptionNonSubscriptionRespond(LSPalmService *psh, const char *key, const char *payload, LSError *lserror);
    bool LSSubscriptionNonSubscriptionReply(LSHandle *sh, const char *key, const char *payload, LSError *lserror);
    bool isSubscribeTypeValid(LSHandle *sh, LSMessage *message, bool isMandatory, bool *isSubscription);
    void stopNonSubcription(const char *key);
    bool isSubscListFilled(LSHandle *sh, LSMessage *message, const char *key,bool cancelCase);
    bool enableGpsHandler(unsigned char *startedHandlers);
    bool enableNwHandler(unsigned char *startedHandlers);
    int enableHandlers(int sel_handler, char *key, unsigned char *startedHandlers);
    bool LSSubNonSubRespondGetLocUpdateCase(Position *pos, Accuracy *acc, LSPalmService *psh, const char *key, const char *payload, LSError *lserror);
    bool LSSubNonSubReplyLocUpdateCase(Position *pos, Accuracy *acc, LSHandle *sh, const char *key, const char *payload, LSError *lserror);
    bool LSMessageReplyLocUpdateCase(LSMessage *msg,
                                     LSHandle *sh,
                                     const char *key,
                                     const char *payload,
                                     LSSubscriptionIter *iter,
                                     LSError *lserror);
    bool meetsCriteria(LSMessage *msg, Position *pos, Accuracy *acc, int minInterval,int minDist);
    void getLocRequestStopSubscription(LSHandle *sh, LSMessage *message);
    bool LSMessageRemoveReqList(LSMessage *message);
    bool removeTimer(LSMessage *message);

private:
    bool mGpsStatus;
    bool mNwStatus;
    bool mCachedGpsEngineStatus;
    bool mIsGetNmeaSubProgress;
    bool mIsGetSatSubProgress;
    bool mIsStartTrackProgress;
    bool mIsStartFirstReply;
    long long mlastTrackingReplyTime;
    typedef boost::shared_ptr<LocationUpdateRequest> LocationUpdateRequestPtr;
    std::vector<LocationUpdateRequestPtr> m_locUpdate_req_list;
    bool wifistate;
    bool isInternetConnectionAvailable;
    bool isTelephonyAvailable;
    bool isWifiInternetAvailable;
    int m_service_state;
    static bool instanceFlag;
    static LocationService *locService;
    static LSMethod rootMethod[];
    static LSMethod prvMethod[];
    static LSMethod geofenceMethod[];
    bool is_geofenceId_used[MAX_GEOFENCE_ID];
    unsigned char trackhandlerstate;
    GHashTable *htPseudoGeofence;

    LSPalmService *mServiceHandle;
    LSPalmService *mlgeServiceHandle;

    // for calculating time to first fix
    static long mFixRequestTime;
    // time to first fix for most recent session
    static long mTTFF;
    // time we received our last fix
    static long mLastFixTime;
    GMainLoop *mMainLoop;
    pthread_mutex_t lbs_geocode_lock;
    pthread_mutex_t lbs_reverse_lock;
    pthread_mutex_t nmea_lock;
    pthread_mutex_t sat_lock;
    pthread_mutex_t geofence_add_lock;
    pthread_mutex_t geofence_pause_lock;
    pthread_mutex_t geofence_remove_lock;
    pthread_mutex_t geofence_resume_lock;
    //mapped with HandlerTypes
    Handler *handler_array[HANDLER_MAX];

    static const char* geofenceStateText[GEOFENCE_MAXIMUM];

    LifeCycleMonitor *m_lifeCycleMonitor;
    bool m_enableSuspendBlocker;

    char nwGeolocationKey[MAX_API_KEY_LENGTH];
    char lbsGeocodeKey[MAX_API_KEY_LENGTH];
    LocationService();
    bool getNmeaData(LSHandle *sh, LSMessage *message, void *data);
    bool getCurrentPosition(LSHandle *sh, LSMessage *message, void *data);
    bool getReverseLocation(LSHandle *sh, LSMessage *message, void *data);
    bool getGeoCodeLocation(LSHandle *sh, LSMessage *message, void *data);
    bool get_nominatium_geocode(LSHandle *sh, LSMessage *message, void *data);
    bool get_nominatium_reverse_geocode(LSHandle *sh, LSMessage *message, void *data);
    void geocodeFreeAddress(Address *addr);
    bool getAllLocationHandlers(LSHandle *sh, LSMessage *message, void *data);
    bool getGpsStatus(LSHandle *sh, LSMessage *message, void *data);
    bool getState(LSHandle *sh, LSMessage *message, void *data);
    bool setState(LSHandle *sh, LSMessage *message, void *data);
    bool sendExtraCommand(LSHandle *sh, LSMessage *message, void *data);
    bool setGPSParameters(LSHandle * sh, LSMessage * message, void * data);
    bool stopGPS(LSHandle * sh, LSMessage * message, void * data);
    bool exitLocation(LSHandle * sh, LSMessage * message, void * data);
    bool getLocationHandlerDetails(LSHandle *sh, LSMessage *message, void *data);
    bool getGpsSatelliteData(LSHandle *sh, LSMessage *message, void *data);
    bool getTimeToFirstFix(LSHandle *sh, LSMessage *message, void *data);
    bool getLocationUpdates(LSHandle *sh, LSMessage *message, void *data);
    bool getCachedPosition(LSHandle *sh, LSMessage *message, void *data);
    bool cancelSubscription(LSHandle *sh, LSMessage *message, void *data);
    bool addGeofenceArea(LSHandle *sh, LSMessage *message, void *data);
    bool removeGeofenceArea(LSHandle *sh, LSMessage *message, void *data);
    bool pauseGeofence(LSHandle *sh, LSMessage *message, void *data);
    bool resumeGeofence(LSHandle *sh, LSMessage *message, void *data);
    bool getGeofenceStatus(LSHandle *sh, LSMessage *message, void *data);
    gboolean _TimerCallbackLocationUpdate (void *data);
    bool reqLocationToHandler(int handler_type, unsigned char *reqHandlerType, int subHandlerType, LSHandle *sh, const char *key);
    bool getCachedDatafromHandler(Handler *hdl, Position *pos, Accuracy *acc, int type);
    void geocoding_reply(char *response, int error, int type);
    void rev_geocoding_reply(char *response, int error, int type);
    void get_nmea_reply(long long timestamp, char *data, int length);
    void getGpsSatelliteData_reply(Satellite *);
    void getGpsStatus_reply(int);
    void geofence_add_reply(int32_t geofence_id, int32_t status);
    void geofence_breach_reply(int32_t geofence_id, int32_t status, int64_t timestamp, double latitude, double longitude);
    void geofence_remove_reply(int32_t geofence_id, int32_t status);
    void geofence_pause_reply(int32_t geofence_id, int32_t status);
    void geofence_resume_reply(int32_t geofence_id, int32_t status);
    void geofence_status_reply(int32_t status, Position *last_position, Accuracy *accuracy);
    void getLocationUpdate_reply(Position *pos, Accuracy *acc, int, int);
    void stopSubcription(LSHandle *sh, const char *key);
    void LSErrorPrintAndFree(LSError *ptrLSError);
    int getHandlerVal(char *handlerName);
    Position comparePositionTimeStamps(Position pos1, Position pos2, Position pos3, Accuracy acc1, Accuracy acc2, Accuracy acc3, Accuracy *retAcc);
    int getConnectionErrorCode();
    void replyHandlerState(HandlerTypes handler, bool state, char *subscription_key, jvalue_ref *serviceObject, jvalue_ref *getAllLocationHandlersReplyObject);
    void getAddressNominatiumData(jvalue_ref *serviceObject, Address *address);
    bool getNominatiumReverseGeocode(LSHandle *sh, LSMessage *message, void *data);
    void getReverseGeocodeData(jvalue_ref *parsedObj, GString **pos_data, Position *pos);
    bool getNominatiumGeocode(LSHandle *sh, LSMessage *message, void *data);
    void printMessageDetails(const char *usage, LSMessage *msg, LSHandle *sh);
    bool isListFilled(LSHandle *sh, LSMessage *message, const char *key, bool cancelCase);

};

#endif  //  H_LocationService
