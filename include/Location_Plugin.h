/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Location_Plugin.h
 * Purpose      : Structures & API's common to all plugins
 * Platform     : RedRose
 * Author(s)    : rajeshg gopu
 * Email ID.    : rajeshgopu.iv@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _LOCATION_PLUGIN_H_
#define _LOCATION_PLUGIN_H_

#include <Location.h>
#include <gmodule.h>

#define EXPORT_API __attribute__((visibility("default"))) G_MODULE_EXPORT

/**
 * <Funciton>       PositionCallback
 * <Description>    Callback to get position
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <Position> <In> <Position info>
 * @param           <Accuracy> <In> <Accuracy info>
 * @param           <error> <In> <Error>
 * @param           <userdata> <In> <Gobject private instance>
 * @param           <type> <In> <type>
 * @return          void
 */
typedef void (*PositionCallback)(gboolean enable_cb, Position *position, Accuracy *accuracy,
        int error, gpointer userdata, int type);

/**
 * <Funciton>       LastPositionCallback
 * <Description>    Callback to get Last position
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <Position> <In> <Position info>
 * @param           <Accuracy> <In> <Accuracy info>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*LastPositionCallback)(gboolean enable_cb, Position *lastPosition, Accuracy *accuracy,
        gpointer userdata);

/**
 * <Funciton >   VelocityCallback
 * <Description>  gCallback to get  Velocity
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Velocity> <In> <Velocity info>
 * @param     <Accuracy> <In> <Accuracy info>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*VelocityCallback)(gboolean enable_cb,Velocity *velocity, Accuracy *accuracy,
        gpointer userdata);

/**
 * <Funciton >   handler_stop
 * <Description>  geCallback to get last velocity
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Velocity> <In> <Velocity info>
 * @param     <Accuracy> <In> <Accuracy info>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*LastVelocityCallback)(gboolean enable_cb,Velocity *velocity, Accuracy *accuracy,
        gpointer userdata);

/**
 * <Funciton >   handler_stop
 * <Description>  Callback to get Accuracy
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Accuracy> <In> <Accuracy info>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*AccuracyCallback)(gboolean enable_cb,Accuracy *accuracy, gpointer userdata);

/**
 * <Funciton >   handler_stop
 * <Description>  Callback to get NMEA data
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Nmea> <In> <NMEA data>
 * @param     <Accuracy> <In> <Accuracy data>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*NmeaCallback)(gboolean enable_cb, int timestamp, char *data, int length, gpointer userdata);

/**
 * <Funciton >   StatusCallback
 * <Description>  Callback to get Status info
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Status> <In> <Status info>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*StatusCallback)(gboolean enable_cb,gint status, gpointer userdata);

/**
 * <Funciton >   SatelliteCallback
 * <Description>  Callback to get Satellite Info
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Satellite> <In> <Satellite data>
 * 
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*SatelliteCallback)(gboolean enable_cb,Satellite *satellite,gpointer userdata);
/**
 * <Funciton>       TimeoutCallback
 * <Description>    Callback for Timeout
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <error> <In> <Error code>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*TimeoutCallback)(gboolean enable_cb, int error, gpointer userdata);

/**
 * <Funciton>       GeoCodeCallback
 * <Description>    Callback to get GeoCode Data
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <Position> <In> <Position data
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*GeoCodeCallback)(gboolean enable_cb, Position *position, Accuracy *accuracy, gpointer userdata);

/**
 * <Funciton>       RevGeocodeCallback
 * <Description>    gCallback to get RevGeoCode
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <Address> <In> Address data>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*RevGeocodeCallback)(gboolean enable_cb, Address *address, gpointer userdata);

/**
 * <Funciton>       ExtraCmdCallback
 * <Description>    Callback to get response of XTRA commands
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <command> <In> <extra commad identifier>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*ExtraCmdCallback)(gboolean enable_cb, int command, gpointer userdata);

/**
 * <Funciton>       ExtraCmdCallback
 * <Description>    Callback to get response of XTRA commands
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <command> <In> <extra commad identifier>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*StartTrackingCallBack)(gboolean enable_cb, Position *pos, Accuracy *accuracy, int error, gpointer userdata, int type);

/**
 * <Funciton>       getTTFF_cb
 * <Description>    Callback to get time to first fix og GPS receiver
 * @param           <enable_cb> <In> <enable/disable the callback>
 * @param           <ttff> <In> <ttff value>
 * @param           <userdata> <In> <Gobject private instance>
 * @return          void
 */
typedef void (*getTTFF_cb)(gboolean enable_cb, double ttff, gpointer userdata);





/**
 * GPS plug-in APIS
 */
typedef struct {
    int (*start)(gpointer handle, StatusCallback status, gpointer userdata);
    int (*stop)(gpointer handle);
    int (*get_position)(gpointer handle, PositionCallback pos_cb);
    int (*get_gps_data)(gpointer handle, gboolean enable_data, gpointer gps_cb, int data_type);
} GpsPluginOps;

typedef struct {
    int (*start)(gpointer handle, gpointer userdata);
    int (*stop)(gpointer handle);
    int (*get_position)(gpointer handle, PositionCallback pos_cb);
    int (*start_tracking)(gpointer handle, gboolean enable, StartTrackingCallBack track_cb);
} WifiPluginOps;

typedef struct {
    int (*start)(gpointer handle, gpointer userdata);
    int (*stop)(gpointer handle);
    int (*get_position)(gpointer handle, PositionCallback pos_cb, gpointer celldata);
    int (*get_last_position)(gpointer handle, Position **position, Accuracy **accuracy);
    int (*start_tracking)(gpointer handle, gboolean enable, StartTrackingCallBack track_cb, gpointer celldata);
} CellPluginOps;

typedef struct {
    int (*start)(gpointer handle, gpointer userdata);
    int (*stop)(gpointer handle);
    int (*get_geocode)(gpointer handle, const Address *address, GList **position_list, GList **accuracy_list);
    int (*get_geocode_freetext)(gpointer handle, const gchar *address, GList **position_list, GList **accuracy_list);
    int (*get_reverse_geocode)(gpointer handle, Position *position, Address **address, Accuracy **accuracy);              
} LbsPluginOps;

#endif  /* _LOCATION_PLUGIN_H_ */
