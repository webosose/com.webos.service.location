/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Gps_Handler.h
 * Purpose      : Gobject GPS handler header file
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

#ifndef _GPS_HANDLER_H_
#define _GPS_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_GPS                  (gps_handler_get_type ())
#define GPS_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HANDLER_TYPE_GPS, GpsHandler))
#define GPS_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HANDLER_TYPE_GPS))
#define GPS_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), HANDLER_TYPE_GPS, GpsHandlerClass))
#define GPS_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), HANDLER_TYPE_GPS))
#define GPS_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), HANDLER_TYPE_GPS, GpsHandlerClass))

typedef struct _GpsHandler GpsHandler;
typedef struct _GpsHandlerClass GpsHandlerClass;

struct _GpsHandler {
    GObject parent_instance;
};

struct _GpsHandlerClass {
    GObjectClass parent_class;
};

GType gps_handler_get_type(void);

typedef enum {
    PROGRESS_NONE = 0,
    GET_POSITION_ON = 1 << 0,
    START_TRACKING_ON = 1 << 1,
    NMEA_GET_DATA_ON = 1 << 2,
    SATELLITE_GET_DATA_ON = 1 << 3,
    START_TRACKING_CRITERIA_ON = 1 << 4,
    LOCATION_UPDATES_ON = 1 << 5
} HandlerStateFlags;

G_END_DECLS

#endif  /* _GPS_HANDLER_H_ */
