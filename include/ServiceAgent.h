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
#define SUBSC_GPS_GET_NMEA_KEY "getNmeaData"
#define SUBSC_SEND_XTRA_CMD_KEY "sendExtraCommand"
#define SUBSC_GET_TTFF_KEY "getTimeToFirstFix"
#define SUBSC_GET_GPS_SATELLITE_DATA "getGpsSatelliteData"
#define SUBSC_GPS_ENGINE_STATUS "getGpsStatus"
#define SUBSC_GET_GEOCODE_KEY "getGeoCodeLocation"
#define SUBSC_GET_REVGEOCODE_KEY "getReverseLocation"
#define SUBSC_GET_STATE_KEY "getState"
#define SUBSC_GETALLLOCATIONHANDLERS "getAllLocationHandlers"
#define SUBSC_GEOFENCE_STATUS_KEY "geofenceStatus"
#define SUBSC_GEOFENCE_PAUSE_AREA_KEY "pauseGeofenceArea"
#define SUBSC_GEOFENCE_RESUME_AREA_KEY "resumeGeofenceArea"
#define SUBSC_GEOFENCE_REMOVE_AREA_KEY "removeGeofenceArea"
#define SUBSC_GEOFENCE_ADD_AREA_KEY "addGeofenceArea"
#define GET_LOC_UPDATE_KEY "getLocationUpdates"
#define SUBSC_GET_LOC_UPDATES_GPS_KEY "gps/getLocationUpdate"
#define SUBSC_GET_LOC_UPDATES_NW_KEY  "nw/getLocationUpdate"
#define SUBSC_GET_LOC_UPDATES_HYBRID_KEY   "gpsnw/getLocationUpdate"
#define SUBSC_GET_LOC_UPDATES_PASSIVE_KEY "passive/getLocationUpdate"

#define GPS "gps"
#define NETWORK "network"
#define HYBRID "hybrid"
#define PASSIVE "passive"
#define START TRUE
#define STOP FALSE
#define MINIMAL_ACCURACY 100 // 100 meter as minimum accuracy for handler to reply
enum LocationErrorCode {
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
    LOCATION_GEOFENCE_TOO_MANY_GEOFENCE,
    LOCATION_GEOFENCE_ID_EXIST,
    LOCATION_GEOFENCE_ID_UNKNOWN,
    LOCATION_GEOFENCE_INVALID_TRANSITION,
    LOCATION_ERROR_MAX
};

enum GeofenceStateCode {
    GEOFENCE_ADD_SUCCESS = 0,
    GEOFENCE_REMOVE_SUCCESS,
    GEOFENCE_PAUSE_SUCCESS ,
    GEOFENCE_RESUME_SUCCESS ,
    GEOFENCE_TRANSITION_ENTERED,
    GEOFENCE_TRANSITION_EXITED,
    GEOFENCE_TRANSITION_UNCERTAIN,
    GEOFENCE_MAXIMUM
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
