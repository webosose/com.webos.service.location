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
 * Filename  : LocationManagerService.h
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
#ifndef H_LocationHandler
#define H_LocationHandler
#include <Location.h>
#include <Handler_Interface.h>
#include <Location_Plugin.h>
#define DEBUG 1
/**
 * @brief FUNCTION_PROTOTYPES
 */
#ifdef __cplusplus
extern "C" {
#endif

    /*****These functions will interact with Location Handlers *********/
    int getNmeaData (Handler *handler, gboolean enable, NmeaCallback , gpointer );
    int getCurrentPosition (Handler *handler , gboolean enable,PositionCallback pos_cb,int handlertype,LSHandle *sh);
    void startTracking (Handler *handler , gboolean enable, StartTrackingCallBack track_cb , int handlertype, LSHandle *sh);
    void getReverseLocation ();
    void getGeoCodeLocation ();
    void getAllLocationHandlers ();
    void SetGpsStatus ();
    void GetGpsStatus ();
    void getState ();
    void setState ();
    void sendExtraCommand ();
    void getLocationHandlerDetails ();
    void getGpsSatelliteData(Handler *handler, gboolean enable, SatelliteCallback cb);
    void getTimeToFirstFix ();



    // /**Callback called from Handlers********/
    void wrapper_getNmeaData_cb(gboolean ,int , char *, int , gpointer);
    void wrapper_getCurrentPosition_cb(gboolean ,Position *, Accuracy *,int ,gpointer, int);
    void wrapper_startTracking_cb(gboolean ,Position* ,Accuracy* , int  ,gpointer, int);
    void wrappergetReverseLocation_cb(gboolean enable_cb,Address* address, gpointer privateIns);
    void wrapper_getGeoCodeLocation_cb(gboolean enable_cb,Position* position, gpointer privateIns);
    void wrapper_getGpsSatelliteData_cb(gboolean enable_cb,Satellite *satellite,
            gpointer privateIns);
    void wrapper_sendExtraCommand_cb(gboolean enable_cb,int command, gpointer privateIns);
    //TODO
    void getTimeToFirstFix_cb();
    void setState_cb();
    void getAllLocationHandlers_cb();
    void setGpsStatus_cb();
    void getGpsStatus_cb();
    void getPositionFromLocationHandler_cb();
    void getState_cb();
    void proximityAlert_cb();
    void get_LocationHandlerDetails_cb();


    void stopTracking(Handler *handler);
    void stopNmeaData(Handler *handler);
    void stopSetelliteData(Handler *handler);
#ifdef __cplusplus


}
#endif
#endif





