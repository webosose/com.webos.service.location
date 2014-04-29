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
#define SUBSC_GPS_ENGINE_STATUS "getGpsStatus"
#define SUBSC_GET_STATE_KEY "getState"
#define GPS_CRITERIA_KEY "gps/startTrackingCriteriaBased"
#define NW_CRITERIA_KEY "nw/startTrackingCriteriaBased"
#define CRITERIA_KEY "startTrackingCriteriaBased"

#define GPS "gps"
#define NETWORK "network"
#define START TRUE
#define STOP FALSE
#define MINIMAL_ACCURACY 100 // 100 meter as minimum accuracy for handler to reply
enum TrakingErrorCode {
    LOCATION_SUCCESS,
    LOCATION_TIME_OUT,
    LOCATION_POS_NOT_AVAILABLE,
    LOCATION_UNKNOWN_ERROR,
    LOCATION_GPS_UNAVAILABLE,
    LOCATION_LOCATION_OFF,
    LOCATION_PENDING_REQ,
    LOCATION_APP_BLACK_LISTED,
    LOCATION_START_FAILURE,
    LOCATION_STATE_UNKNOWN,
    LOCATION_INVALID_INPUT,
    LOCATION_DATA_CONNECTION_OFF,
    LOCATION_WIFI_CONNECTION_OFF,
    LOCATION_OUT_OF_MEM,
    LOCATION_TRACKING_MAXIMUM
};

/**
 * bitfield representing the
 * API progrees state
 */
enum RequestState {
    GET_CURRENT_POSITION = (1u << 0),
    START_TRACKING_REQ_STATE = (1u << 1),
    GET_NMEA_DATA = (1u << 2),
    GET_SATELLITE_DATA = (1u << 3),
};

// Mapped with HandlerTypes
enum HanlderRequestType {
    HANDLER_HYBRID_BIT = (1u << HANDLER_HYBRID),
    HANDLER_GPS_BIT = (1u << HANDLER_GPS),
    HANDLER_WIFI_BIT = (1u << HANDLER_WIFI),
    HANDLER_CELLID_BIT = (1u << HANDLER_CELLID)
};

enum maxAge {
    ALWAYS_READ_FROM_CACHE = -1,
    READ_FROM_CACHE
};

typedef struct structHandlerStatus {
    unsigned char gps;
    unsigned char wifi;
    unsigned char cellid;
} HandlerStatus;

enum HandlerStatusEnum {
    HANDLER_STATE_UNDEFINED = -1,
    HANDLER_STATE_DISABLED,
    HANDLER_STATE_ENABLED
};

#endif /* SERVICEAGENT_H_ */
