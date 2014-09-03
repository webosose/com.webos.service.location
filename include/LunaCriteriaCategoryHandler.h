/**********************************************************
 * (c) Copyright 2014. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Red Rose Platform location
 * Team   : CSP Team
 * Security  : Confidential
 * ***********************************************************/

/*********************************************************
 * @file
 * Filename  : LunaCriteriaCategoryHandler.h
 * Purpose   : Provides location related API to application
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
#ifndef _CRITERIA_CATEGORY_LUNA_SERVICE_H_
#define _CRITERIA_CATEGORY_LUNA_SERVICE_H_

#include <Handler_Interface.h>

#define MATH_PI 3.1415926535897932384626433832795
#define SUCCESSFUL 0
#define KEY_MAX  64

class LunaCriteriaCategoryHandler
{
public:
    static const int NO_REQUIREMENT = 0;
    static const int ACCURACY_FINE = 1;
    static const int ACCURACY_COARSE = 2;
    static const int CRITERIA_NW = 0;
    static const int CRITERIA_GPS_NW = 1;
    static const int CRITERIA_GPS = 2;
    static const int CRITERIA_PASSIVE = 3;
    static const int CRITERIA_INVALID =4;

    static const int POWER_HIGH = 1;
    static const int POWER_LOW = 2;

    static const int MIN_RANGE = 1000;
    static const int MAX_RANGE = 60000;

    static const double INVALID_LAT = 0;
    static const double INVALID_LONG = 0;

    LunaCriteriaCategoryHandler();
    ~LunaCriteriaCategoryHandler();
    bool init(LSPalmService *sh, LSPalmService *sh_lge, Handler **handler);
    void LSErrorPrintAndFree(LSError *ptrLSError);
    int handlerSelection(int accLevel, int powLevel);
    int enableHandlers(int sel_handler, char *sub_key_gps, char *sub_key_nw, char *sub_key_gpsnw, unsigned char *startedHandlers);
    bool enableGpsHandler(unsigned char *startedHandlers);
    bool enableNwHandler(unsigned char *startedHandlers);
    void startTrackingCriteriaBased_reply(Position *pos, Accuracy *accuracy, int error, int type);
    bool LSSubscriptionNonSubMeetsCriteriaRespond(Position *pos,
                                                  LSPalmService *psh,
                                                  const char *key,
                                                  const char *payload,
                                                  LSError *lserror);
    bool LSSubscriptionNonMeetsCriteriaReply(Position *pos,
                                             LSHandle *sh,
                                             const char *key,
                                             const char *payload,
                                             LSError *lserror);
    bool minDistance(int minDistance, double latitude1, double longitude1, double latitude2 , double longitude2);
    void removeMessageFromCriteriaReqList(LSMessage *message);

    static bool _startTrackingCriteriaBased(LSHandle *sh, LSMessage *message, void *data) {
        return ((LunaCriteriaCategoryHandler *) data)->startTrackingCriteriaBased(sh, message);
    }

    static bool _getLocationCriteriaHandlerDetails(LSHandle *sh, LSMessage *message, void *data) {
        return ((LunaCriteriaCategoryHandler *) data)->getLocationCriteriaHandlerDetails(sh, message);
    }

    static void start_tracking_criteria_cb(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error,
                                           gpointer privateIns, int type) {
        getInstance()->startTrackingCriteriaBased_reply(pos, accuracy, error, type);
    }

    static LunaCriteriaCategoryHandler *getInstance();
    bool LSMessageReplyCriteria(LSMessage *msg,
                                Position *pos,
                                LSHandle *sh,
                                const char *key,
                                const char *payload,
                                LSSubscriptionIter *iter,
                                LSError *lserror);

    bool meetsCriteria(LSMessage *,Position *,int,int,bool,bool);

    class CriteriaRequest
    {
    public:
        CriteriaRequest(LSMessage *msg, long long reqTime,double latitude, double longitude) {
            message = msg;
            requestTime = reqTime;
            lastlat = latitude;
            lastlong = longitude;
            isFirstReply = true;
        }
        LSMessage *getMessage() {
            return message;
        }
        long long getRequestTime() {
            return requestTime;
        }
        void updateRequestTime(long long currentTime) {
            requestTime = currentTime;
        }
        double getLatitude(){
            return lastlat;
        }
        double getLongitude(){
            return lastlong;
        }
        void updateLatAndLong(double latitude,double longitude) {
            lastlat = latitude;
            lastlong = longitude;
        }
        bool getFirstReply() {
            return isFirstReply;
        }
        bool updateFirstReply(bool firstreply) {
            isFirstReply = firstreply;
        }

    private:
        LSMessage *message;
        long long requestTime;
        double lastlong;
        double lastlat;
        bool isFirstReply;
    };

private:
    typedef boost::shared_ptr<CriteriaRequest> CriteriaRequestPtr;
    std::vector<CriteriaRequestPtr> m_criteria_req_list;
    double mlastLat;
    double mlastLong;
    LSPalmService *mpalmSrvHandle;
    LSPalmService *mpalmLgeSrvHandle;
    Handler **handler_array;
    unsigned char trackhandlerstate;

    static LSMethod criteriaCategoryMethods[];
    bool startTrackingCriteriaBased(LSHandle *sh, LSMessage *message);
    bool getLocationCriteriaHandlerDetails(LSHandle *sh, LSMessage *message);
    static LunaCriteriaCategoryHandler *CriteriaCatSvc;

};

#endif
