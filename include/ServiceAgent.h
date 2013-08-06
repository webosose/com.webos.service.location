/*
 * ServiceAgent.h
 *
 *  Created on: Feb 27, 2013
 *      Author: rajeshgopu.iv
 */

#ifndef SERVICEAGENT_H_
#define SERVICEAGENT_H_

#include <Handler_Interface.h>
#include <Location.h>

#define REQ_HANDLER_TYPE(type) (1u << type)
#define MAX_LIMIT 64
/*
 * Key values used to store Luna subscription list
 */
#define SUBSC_START_TRACK_KEY "startTracking"
#define SUBSC_GPS_GET_NMEA_KEY "getNmeaData"
#define SUBSC_GPS_GET_CURR_POSITION_KEY "getCurrentPosition"
#define SUBSC_SEND_XTRA_CMD_KEY "sendExtraCommand"
#define SUBSC_GET_TTFF_KEY "getTimeToFirstFix"
#define SUBSC_GET_GPS_SATELLITE_DATA "getGpsSatelliteData"

#define GPS "gps"
#define NETWORK "network"
#define START TRUE
#define STOP FALSE
enum TrakingErrorCode
{
    TRACKING_SUCCESS,
    TRACKING_TIMEOUT,
    TRACKING_POS_NOT_AVAILABLE,
    TRACKING_UNKNOWN_ERROR,
    TRACKING_GPS_UNAVAILABLE,
    TRACKING_LOCATION_OFF,
    TRACKING_PENDING_REQ,
    TRACKING_APP_BLACK_LISTED,
    TRACKING_NOT_SUBSC_CALL,
    TRACKING_GPS_HANDLER_START_FAILED,
    HANDLER_START_FAILURE,
    STATE_UNKNOWN
};
enum getStateErrorCode
{
    GET_STATE_SUCCESS,
    GET_STATE_TIME_OUT,
    GET_STATE_HANDLER_NOT_AVAILABLE,
    GET_STATE_HANDLER_INVALID_INPUT,
    GET_STATE_UNKNOWN_ERROR,
    GET_STATE_OUT_OF_MEM
};

enum setStateErrorCode
{
    SET_STATE_SUCCESS,
    SET_STATE_TIME_OUT,
    SET_STATE_HANDLER_NOT_AVAILABLE,
    SET_STATE_HANDLER_INVALID_INPUT,
    SET_STATE_UNKNOWN_ERROR
};
enum handlerDetailErrorCode
{
    GET_HANDLER_DETAILS_SUCCESS,
    GET_HANDLER_DETAILS_INVALID_PARAMETER,
    GET_HANDLER_DETAILS_NOT_SUPPORTED
};
enum getGpsSatelliteDataErrorCode
{
    GET_SAT_DATA_SUCCESS,
    GET_SAT_DATA_TIME_OUT,
    GET_SAT_DATA_HANDLER_NOT_AVAILABLE,
    GET_SAT_DATA_UNKNOWN_ERROR,
    GET_SAT_DATA_GPS_UNAVAILABLE,
    GET_SAT_DATA_LOCATION_OFF
};

enum getReverseLocationErrorCode
{
    GET_REV_LOCATION_SUCCESS,
    GET_REV_LOCATION_TIME_OUT,
    GET_REV_LOCATION_INVALID_INPUT,
    GET_REV_LOCATION_UNKNOWN_ERROR
};

enum getGeoCodeLocationErrorCode
{
    GEO_CODE_LOCATION_SUCCESS,
    GEO_CODE_LOCATION_TIME_OUT,
    GEO_CODE_LOCATION_INVALID_INPUT,
    GEO_CODE_LOCATION_UNKNOWN_ERROR
};
enum methodsCode
{
    METHOD_NONE,
    START_TRACK,
    GET_POS
};

/**
 * bitfield representing the
 * API progrees state
 */
enum RequestState
{
    GET_CURRENT_POSITION = (1u << 0),
    START_TRACKING_REQ_STATE = (1u << 1),
    GET_NMEA_DATA = (1u << 2),
    GET_SATELLITE_DATA = (1u << 3),
};

/*
 myState |= MAXIMIZED;  // sets that bit
 myState &= ~MAXIMIZED; // resets that bit
 */

struct ServiceAgentProp
{
    Handler *gps_handler;
    Handler *network_handler;
    Handler *hybrid_handler;
    Handler *wifi_handler;
    Handler *cell_handler; // testing
    int handlersToStart;
    gint api_progress_state;
    gpointer ServiceObjRef;
    unsigned int REQUEST_STATE;
};

// Mapped with HandlerTypes
enum HanlderRequestType
{
    HANDLER_HYBRID_BIT = (1u << HANDLER_HYBRID),
    HANDLER_GPS_BIT = (1u << HANDLER_GPS),
    HANDLER_WIFI_BIT = (1u << HANDLER_WIFI),
    HANDLER_CELLID_BIT = (1u << HANDLER_CELLID)
};

typedef struct structHandlerStatus
{
    unsigned char gps;
    unsigned char wifi;
    unsigned char cellid;
} HandlerStatus;

enum HandlerStatusEnum
{
    HANDLER_STATE_UNDEFINED = -1,
    HANDLER_STATE_DISABLED,
    HANDLER_STATE_ENABLED
};

#endif /* SERVICEAGENT_H_ */
