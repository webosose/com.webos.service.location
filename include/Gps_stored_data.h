/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework Geoclue Service
 * Group  : CSP-2
 * Security  : Confidential
 ********************************************************************/

#ifndef _GPS_STORED_DATA_H_
#define _GPS_STORED_DATA_H_
#include <glib.h>
#include <Position.h>
#include <Location.h>

G_BEGIN_DECLS

void set_store_position(int64_t timestamp, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed,
                        gdouble direction, gdouble hor_accuracy, gdouble ver_accuracy , const char *path);
int get_stored_position(Position *position, Accuracy *accuracy, char *path);

G_END_DECLS

#endif /* _GPS_STORED_DATA_H_ */
