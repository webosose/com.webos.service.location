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

/*
 * Key values used to store Luna subscription list
 */
#define SUBSC_START_TRACK_KEY "startTracking"
#define SUBSC_GPS_GET_NMEA_KEY "/getNmeaData"
#define SUBSC_GPS_GET_CURR_POSITION_KEY "gps/getCurrentPosition"
#define SUBSC_WIFI_GET_CURR_POSITION_KEY "wifi/getCurrentPosition"
#define SUBSC_CELLID_GET_CURR_POSITION_KEY "cellid/getCurrentPosition"
#define SUBSC_GET_REV_LOC_KEY "/getReverseLocation"
#define SUBSC_GET_GEO_CODELOC_KEY "/getGeoCodeLocation"
#define SUBSC_GETALL_LOC_HANDLES_KEY "/getAllLocationHandlers"
#define SUBSC_SET_GPS_STATUS_KEY "/SetGpsStatus"
#define SUBSC_GET_GPS_STATUS "/GetGpsStatus"
#define SUBSC_SET_STATE_KEY "/setState"
#define SUBSC_GET_STATE_KEY "/getState"
#define SUBSC_SEND_XTRA_CMD_KEY "/sendExtraCommand"
#define SUBSC_GETLOC_HANDLER_DTLS_KEY "/getLocationHandlerDetails"
#define SUBSC_GET_GPS_STATE "/getGpsSatelliteData"
#define SUBSC_GET_TTFF_KEY "/getTimeToFirstFix"
#define SUBSC_GET_GPS_SATELLITE_DATA "/getGpsSatelliteData"

#define GPS "gps"
#define NETWORK "network"
#define START TRUE
#define STOP FALSE
enum TrakingErrorCode {
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
    STATE_UNKNOWN
};

enum methodsCode {
    METHOD_NONE,
    START_TRACK,
    GET_POS
};

/**
 * bitfield representing the
 * API progrees state
 */
enum RequestState {
    GET_CURRENT_POSITION = (1u << 0),
    START_TRACKING_REQ_STATE = (1u << 1),
    GET_NMEA_DATA= (1u << 2),
    GET_SATELLITE_DATA = (1u << 3),
};




/*
   myState |= MAXIMIZED;  // sets that bit
   myState &= ~MAXIMIZED; // resets that bit
 */


struct ServiceAgentProp{
    Handler *gps_handler;
    Handler *network_handler;
    Handler *hybrid_handler;
    Handler *wifi_handler;
    Handler *cell_handler; // testing
    int handlersToStart;
    gint api_progress_state ;
    gpointer ServiceObjRef;
    unsigned int REQUEST_STATE;
};

// Mapped with HandlerTypes
enum HanlderRequestType {
    REQUEST_HYBRID =(1u << HANDLER_HYBRID),
    REQUEST_GPS = (1u << HANDLER_GPS),
    REQUEST_WIFI = (1u << HANDLER_WIFI),
    REQUEST_CELLID = (1u << HANDLER_CELLID)
};

typedef struct _HandlerStatus {
    unsigned char gps;
    unsigned char wifi;
    unsigned char cellid;
} HandlerStatus;

enum HandlerStatusEnum {
    HANDLER_STATE_UNDEFINED =-1,
    HANDLER_STATE_DISABLED,
    HANDLER_STATE_ENABLED
};




#endif /* SERVICEAGENT_H_ */
