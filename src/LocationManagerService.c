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
 * Filename  : LocationManagerService.c
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
#include <LocationManagerService.h>
#include <Handler_Interface.h>
#include<stdio.h>

/***Calls to Handlers***/
int getNmeaData(Handler *handler, gboolean enable, NmeaCallback nmea_cb, gpointer userdata)
{
    return handler_get_nmea_data(handler, enable, nmea_cb, userdata);
}

int getCurrentPosition(Handler *handler, gboolean enable, PositionCallback current_pos_cb, int handlertype,
                       LSHandle *sh)
{
    gpointer handlerobj = NULL;
    return handler_get_position(handler, enable, current_pos_cb, handlerobj, handlertype, sh);
}

void startTracking(Handler *handler, gboolean enable, StartTrackingCallBack track_cb, int handlertype, LSHandle *sh)
{
    gpointer handlerobj = NULL;
    handler_start_tracking(handler, enable, track_cb, handlerobj, handlertype, sh);
}

void getReverseLocation(Handler *handler, Position *pos, Address *address)
{
    handler_get_reverse_geo_code(handler, pos, address);
}
void getGeoCodeLocation(Handler *handler, Address *address, Position *pos, Accuracy *ac)
{
    handler_get_geo_code(handler, address, pos, ac);
}
void getAllLocationHandlers()
{
}
void SetGpsStatus()
{
}
void GetGpsStatus()
{
}
void getState()
{
}
void setState()
{
}
void sendExtraCommand()
{
}
void getLocationHandlerDetails()
{
}
int getGpsSatelliteData(Handler *handler, gboolean enable, SatelliteCallback cb)
{
    return handler_get_gps_satellite_data(handler, enable, cb);
}
long long getTimeToFirstFix(Handler *handler)
{
    return handler_get_time_to_first_fix(handler);
}

