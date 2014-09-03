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
 * Author(s)    : rajeshg gopu, Abhishek Srivastava
 * Email ID.    : rajeshgopu.iv@lge.com , abhishek.srivastava@lge.com
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
typedef void (*PositionCallback)(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer userdata, int type);

/**
 * <Funciton >   handler_stop
 * <Description>  Callback to get NMEA data
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Nmea> <In> <NMEA data>
 * @param     <Accuracy> <In> <Accuracy data>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*NmeaCallback)(gboolean enable_cb, int64_t timestamp, char *data, int length, gpointer userdata);

/**
 * <Funciton >   StatusCallback
 * <Description>  Callback to get Status info
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Status> <In> <Status info>
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*StatusCallback)(gboolean enable_cb, gint status, gpointer userdata);

/**
 * <Funciton >   SatelliteCallback
 * <Description>  Callback to get Satellite Info
 * @param     <enable_cb> <In> <enable/disable the callback>
 * @param     <Satellite> <In> <Satellite data>
 *
 * @param     <userdata> <In> <Gobject private instance>
 * @return    int
 */
typedef void (*SatelliteCallback)(gboolean enable_cb, Satellite *satellite, gpointer userdata);
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


typedef void (*GeofenceAddCallBack)(int32_t geofence_id, int32_t status, gpointer user_data);

typedef void (*GeofenceResumeCallback) (int32_t geofence_id, int32_t status, gpointer user_data);

typedef void (*GeofencePauseCallback) (int32_t geofence_id, int32_t status, gpointer user_data);

typedef void (*GeofenceRemoveCallback) (int32_t geofence_id, int32_t status, gpointer user_data);

typedef void (*GeofenceBreachCallback) (int32_t geofence_id, int32_t status, int64_t timestamp, double latitude, double longitude, gpointer user_data);

typedef void (*GeofenceStatusCallback) (int32_t status, Position *last_position, Accuracy *accuracy, gpointer user_data);

typedef void (*GeoCodeCallback)(gboolean enable_cb, char *response, int error, gpointer userdata, int type);
/**
 * GPS plug-in APIS
 */
typedef struct {
    int (*start)(gpointer handle, StatusCallback status, gpointer userdata);
    int (*stop)(gpointer handle);
    int (*get_position)(gpointer handle, PositionCallback pos_cb);
    int (*get_gps_data)(gpointer handle, gboolean enable_data, gpointer gps_cb, int data_type);
    int (*send_extra_command)(gpointer handle , char *command);
    int (*set_gps_parameters)(gpointer handle , char *command);
    int (*geofence_add_area)(gpointer handle,
                             gboolean enable,
                             int32_t *geofence_id,
                             gdouble *latitude,
                             gdouble *longitude,
                             gdouble *radius_meters,
                             GeofenceAddCallBack add_cb,
                             GeofenceBreachCallback breach_cb,
                             GeofenceStatusCallback status_cb);
    int (*geofence_process_request)(gpointer handle, gboolean enable, int32_t *geofence_id, int *monitor, gpointer callback, int type);
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
    int (*start_tracking)(gpointer handle, gboolean enable, StartTrackingCallBack track_cb, gpointer celldata);
} CellPluginOps;

typedef struct {
    int (*start)(gpointer handle, gpointer userdata);
    int (*stop)(gpointer handle);
#ifdef NOMINATIUM_LBS
    int (*get_reverse_geocode)(gpointer handle, Position *pos, Address *address);
    int (*get_geocode)(gpointer handle, const Address *address, Position *pos, Accuracy *ac);
    int (*get_geocode_freetext)(gpointer handle, const gchar *addrress, Position *pos, Accuracy *ac);
#else
    int (*get_google_geocode)(gpointer handle, const char *addrress, GeoCodeCallback geocode_cb);
    int (*get_reverse_google_geocode)(gpointer handle, const char *pos, GeoCodeCallback geocode_cb);
#endif
} LbsPluginOps;

#endif  /* _LOCATION_PLUGIN_H_ */
