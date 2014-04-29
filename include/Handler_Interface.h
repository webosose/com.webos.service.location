/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Handler_Interface.h
 * Purpose      : Common Interface for all Handlers (GPS, Hybrid, Network, LBS)
 * Platform     : RedRose
 * Author(s)    : Rajesh Gopu I.V, Abhishek Srivastava
 * Email ID.    : rajeshgopu.iv@lge.com, abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _HANDLER_INTERFACE_H_
#define _HANDLER_INTERFACE_H_

#include <glib-object.h>
#include <Location.h>
#include <Position.h>
#include <Address.h>
#include <Location_Plugin.h>
#include <lunaservice.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_INTERFACE                  (handler_interface_get_type ())
#define HANDLER_INTERFACE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HANDLER_TYPE_INTERFACE, Handler))
#define HANDLER_IS_INTERFACE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HANDLER_TYPE_INTERFACE))
#define HANDLER_INTERFACE_GET_INTERFACE(inst)   (G_TYPE_INSTANCE_GET_INTERFACE ((inst), HANDLER_TYPE_INTERFACE, HandlerInterface))

/**
 * Location Handler instance
 */
typedef struct _Handler Handler;
/**
 * Interface for all Location Handlers
 */
typedef struct _HandlerInterface HandlerInterface;

typedef int (*TYPE_START_FUNC)(Handler *self, int handler_type);
typedef int (*TYPE_STOP_FUNC)(Handler *self, int handler_type, gboolean forcestop);
typedef int (*TYPE_GET_POSITION)(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handlerobj, int handlertype, LSHandle *sh);
typedef void (*TYPE_START_TRACK)(Handler *self, gboolean enable, StartTrackingCallBack pos_cb, gpointer handlerobj, int handlertype, LSHandle *sh);
typedef int (*TYPE_GET_LAST_POSITION)(Handler *self, Position *position, Accuracy *accuracy, int handlertype);
typedef int (*TYPE_GET_TTFF)(Handler *self);
typedef int (*TYPE_GET_SAT)(Handler *self, gboolean enable_satellite, SatelliteCallback sat_cb);
typedef int (*TYPE_GET_NMEA)(Handler *self, gboolean enable_nmea, NmeaCallback nmea_cb, gpointer userdata);
typedef int (*TYPE_SEND_EXTRA)(Handler *self , char *command);
typedef int (*TYPE_GEO_CODE)(Handler *self, Address *address, Position *pos, Accuracy *ac);
typedef int (*TYPE_REV_GEO_CODE)(Handler *self, Position *pos, Address *address);
typedef int (*TYPE_GET_GPS_STATUS)(Handler *self, StatusCallback status_cb);
typedef int (*TYPE_SET_GPS_PARAMETERS)(Handler *self , char *command);
typedef void (*TYPE_START_TRACK_CRITERIA)(Handler *self, gboolean enable, StartTrackingCallBack pos_cb, gpointer handlerobj, int handlertype, LSHandle *sh);
/**
 * Interface for all Location Handlers
 */
struct _HandlerInterface {
    GTypeInterface parent_iface;

    TYPE_START_FUNC start;
    TYPE_STOP_FUNC stop;
    TYPE_GET_POSITION get_position;
    TYPE_START_TRACK start_tracking;
    TYPE_GET_LAST_POSITION get_last_position;
    TYPE_GET_TTFF get_ttfx;
    TYPE_GET_SAT get_sat_data;
    TYPE_GET_NMEA get_nmea_data;
    TYPE_SEND_EXTRA send_extra_cmd;
    TYPE_GEO_CODE get_geo_code;
    TYPE_REV_GEO_CODE get_rev_geocode;
    TYPE_GET_GPS_STATUS get_gps_status;
    TYPE_SET_GPS_PARAMETERS set_gps_params;
    TYPE_START_TRACK_CRITERIA start_tracking_criteria;
};

/*
 * Interface function signatures
 */
GType handler_interface_get_type(void);

int handler_start(Handler *self, int handler_type);

int handler_stop(Handler *self, int handler_type, gboolean forcestop);

int handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handler, int handlertype, LSHandle *sh);

void handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer handlerobj, int handlertype, LSHandle *sh);

int handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy, int handlertype);

guint64 handler_get_time_to_first_fix(Handler *self);

int handler_get_gps_satellite_data(Handler *self, gboolean enable_satellite, SatelliteCallback sat_cb);

int handler_get_nmea_data(Handler *self, gboolean enable_nmea, NmeaCallback nmea_cb, gpointer userdata);

int handler_send_extra_command(Handler *self, char *command);

int handler_get_geo_code(Handler *self, Address *address, Position *pos, Accuracy *ac);

int handler_get_reverse_geo_code(Handler *self, Position *pos, Address *address);

int handler_get_gps_status(Handler *self, StatusCallback gpsStatus_cb);

int handler_set_gps_parameters(Handler *self, char *command);
void handler_start_tracking_criteria(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer handlerobj, int handlertype, LSHandle *sh);
G_END_DECLS

#endif /* _HANDLER_INTERFACE_H_ */
