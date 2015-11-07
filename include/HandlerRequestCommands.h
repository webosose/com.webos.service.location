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

#ifndef HANDLERREQUESTCOMMANDS_H_
#define HANDLERREQUESTCOMMANDS_H_
enum HandlerRequestType{
  UNKNOWN_CMD=-1,
  POSITION_CMD,
  GPS_STATUS_CMD,
  NMEA_CMD,
  SATELITTE_CMD,
  SET_GPS_PARAMETERS_CMD,
  SEND_GPS_EXTRA_CMD,
  ADD_GEOFENCE_CMD,
  REMOVE_GEOFENCE_CMD,
  TIME_TO_FIRST_FIX,
  STOP_POSITION_CMD,
  STOP_NMEA_CMD,
  STOP_SATELITTE_CMD,
  STOP_GPS_CMD
};
#endif /* HANDLERREQUESTCOMMANDS_H_ */

