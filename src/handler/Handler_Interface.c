/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Handler_Interface.c
 * Purpose      : Interface for all the handlers,all common function present here
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

#include <Handler_Interface.h>
#include <LocationService_Log.h>

static void handler_interface_base_init(gpointer g_class)
{
    static gboolean is_initialized = FALSE;
}

GType handler_interface_get_type(void)
{
    static GType interface_type = 0;

    if (interface_type == 0) {
        static const GTypeInfo info = { sizeof(HandlerInterface), handler_interface_base_init, NULL };
        interface_type = g_type_register_static(G_TYPE_INTERFACE, "Handler", &info, 0);
    }

    return interface_type;
}

/**
 * <Funciton>       handler_start
 * <Description>    start the handler
 * @param           <self> <In> <Handler GObject>
 * @throws
 * @return          int
 */
int handler_start(Handler *self, int handler_type)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->start, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_start\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->start(self, handler_type);
}

/**
 * <Funciton>       handler_stop
 * <Description>    stop the handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
int handler_stop(Handler *self, int handlertype, gboolean forcestop)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->stop, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_stop\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->stop(self, handlertype, forcestop);
}

/**
 * <Funciton>       handler_get_position
 * <Description>    get the position from the specified handler
 * @param           <self> <In> <Handler GObject>
 * @param           <pos_cb> <In> <callback function to get result>
 * @return          int
 */
int handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handlerobj, int handlertype, LSHandle *sh)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(pos_cb, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_position, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_position\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_position(self, enable, pos_cb, handlerobj, handlertype, sh);
}

/**
 * <Funciton>       handler_start_tracking
 * <Description>    Continous position updates
 * @param           <self> <In> <Handler GObject>
 * @param           <enable> <In> <true : enabel tracking, false : disable tracking, callback not required in false case>
 * @param           <track_cb> <In> <callback function to get result>
 * @return          void
 */
void handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer handlerobj, int handlertype, LSHandle *sh)
{
    g_return_if_fail(HANDLER_IS_INTERFACE(self));
    g_return_if_fail(track_cb);
    g_return_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->start_tracking);

    LS_LOG_DEBUG("handler_start_tracking\n");

    HANDLER_INTERFACE_GET_INTERFACE(self)->start_tracking(self, enable, track_cb, handlerobj, handlertype, sh);
}

/**
/**
 * <Funciton>       handler_start_tracking
 * <Description>    Continous position updates
 * @param           <self> <In> <Handler GObject>
 * @param           <enable> <In> <true : enabel tracking, false : disable tracking, callback not required in false case>
 * @param           <track_cb> <In> <callback function to get result>
 * @return          void
 */
void handler_start_tracking_criteria(Handler *self, gboolean enable, StartTrackingCallBack track_cb, gpointer handlerobj, int handlertype, LSHandle *sh)
{
    g_return_if_fail(HANDLER_IS_INTERFACE(self));
    g_return_if_fail(track_cb);
    g_return_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->start_tracking_criteria);
    LS_LOG_DEBUG("handler_start_tracking_criteria\n");
    HANDLER_INTERFACE_GET_INTERFACE(self)->start_tracking_criteria(self, enable, track_cb, handlerobj, handlertype, sh);
}
/**
 * <Funciton>       handler_get_last_position
 * <Description>    get the last position from the specified handler
 * @param           <self> <In> <Handler GObject>
 * @return          int
 */
int handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy, int handlertype)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(position, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(accuracy, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_last_position, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_last_position\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_last_position(self, position, accuracy, handlertype);
}

/**
 * <Funciton>       handler_get_time_to_first_fix
 * <Description>    gives the time to first fix value of GPS
 * @param           <self> <In> <Handler GObject>
 * @param           <ttff> <In> <calculated value of TTFF form GPS>
 * @return          int
 */
guint64 handler_get_time_to_first_fix(Handler *self)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_ttfx, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_time_to_first_fix\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_ttfx(self);
}

/**
 * <Funciton>       handler_get_gps_satellite_data
 * <Description>    get the satellite data from the GPS receiver
 * @param           <self> <In> <Handler GObject>
 * @param           <sat_cb> <In> <callback function to get result>
 * @return          int
 */
int handler_get_gps_satellite_data(Handler *self, gboolean enable_satellite, SatelliteCallback sat_cb)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(sat_cb, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_sat_data, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_gps_satellite_data\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_sat_data(self, enable_satellite, sat_cb);
}

/**
 * <Funciton>       handler_get_nmea_data
 * <Description>    get the nmea data from the GPS handler
 * @param           <self> <In> <Handler GObject>
 * @param           <nmea_cb> <In> <nmea callback function to get result>
 * @return          int
 */
int handler_get_nmea_data(Handler *self, gboolean enable_nmea, NmeaCallback nmea_cb, gpointer userdata)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(nmea_cb, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_nmea_data, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_nmea_data\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_nmea_data(self, enable_nmea, nmea_cb, userdata);
}
/**
 * <Funciton>       handler_send_extra_command
 * <Description>    send extra command to GPS ,applicable to GPS handler only
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <extra command id>
 * @param           <extra_cb> <In> <XTRA callback function to get result>
 * @return          int
 */
int handler_get_gps_status(Handler *self , StatusCallback gpsStatus_cb)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(gpsStatus_cb, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_gps_status, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_gps_status\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_gps_status(self, gpsStatus_cb);
}
/**
 * <Funciton>       handler_send_extra_command
 * <Description>    send extra command to GPS ,applicable to GPS handler only
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <extra command id>
 * @param           <extra_cb> <In> <XTRA callback function to get result>
 * @return          int
 */
int handler_send_extra_command(Handler *self , char *command)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->send_extra_cmd, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_send_extra_command\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->send_extra_cmd(self, command);
}
/**
 * <Funciton>       handler_set_gps_parameters
 * <Description>    set options for GPS,applicable to GPS handler only
 * @param           <self> <In> <Handler GObject>
 * @param           <command> <In> <command id>
 * @param           <extra_cb> <In> <XTRA callback function to get result>
 * @return          int
 */
int handler_set_gps_parameters(Handler *self , char *command)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->set_gps_params, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_set_gps_parameters\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->set_gps_params(self, command);
}

/**
 * <Funciton>       handler_get_geo_code
 * <Description>    Convert the given address to latitude & longitude
 * @param           <self> <In> <Handler GObject>
 * @param           <address> <In> <address info which need to convert>
 * @param           <geo_cb> <In> <callback function to get result>
 * @return          int
 */
int handler_get_geo_code(Handler *self, Address *address, Position *pos, Accuracy *ac)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_geo_code, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_geo_code\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_geo_code(self, address, pos, ac);
}

/**
 * <Funciton>       handler_get_reverse_geo_code
 * <Description>    convert given position info to Address
 * @param           <self> <In> <Handler GObject>
 * @param           <pos> <In> <Position structure>
 * @param           <rev_geo_cb> <In> <callback function for reverse geocode>
 * @return          int
 */
int handler_get_reverse_geo_code(Handler *self, Position *pos, Address *address)
{
    g_return_val_if_fail(HANDLER_IS_INTERFACE(self), ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(pos, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(address, ERROR_WRONG_PARAMETER);
    g_return_val_if_fail(HANDLER_INTERFACE_GET_INTERFACE(self)->get_rev_geocode, ERROR_NOT_AVAILABLE);

    LS_LOG_DEBUG("handler_get_reverse_geo_code\n");

    return HANDLER_INTERFACE_GET_INTERFACE(self)->get_rev_geocode(self, pos, address);
}

