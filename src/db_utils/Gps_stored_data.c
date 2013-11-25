/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework Geoclue Service
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Gps_stored_data.c
 * Purpose  : Stroing of GPS data in DB
 * Platform  : RedRose
 * Author(s) : Abhishek Srivastava
 * Email ID. : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/

#include <stdio.h>
#include <glib.h>
#include "db_util.h"
#include <Position.h>
#include <Accuracy.h>
#include <time.h>
#include <Location.h>
#define MAX_LEN 50
/**
 * <Funciton >   gps_service_get_stored_position
 * <Description>   will be called for getting the stored position
 *      .
 * @param     <last_pos> <In> <Stored the position data>
 * @throws
 * @return     Void
 */

void set_store_position(gdouble latitude,
                        gdouble longitude, gdouble altitude, gdouble speed,
                        gdouble direction, gdouble hor_accuracy,
                        gdouble ver_accuracy , char *path)
{
    DBHandle *handle = NULL;
    char input[MAX_LEN];
    handle = (DBHandle *) malloc(sizeof(DBHandle));

    if (handle != NULL) {
        createPreference(path, handle, "Location\n", FALSE);
        sprintf(input, "%lld", time(0));
        put(handle, "timestamp", input);
        sprintf(input, "%lf", latitude);
        put(handle, "latitude", input);
        sprintf(input, "%lf", longitude);
        put(handle, "longitude", input);
        sprintf(input, "%lf", altitude);
        put(handle, "altitude", input);
        sprintf(input, "%lf", speed);
        put(handle, "speed", input);
        sprintf(input, "%lf", direction);
        put(handle, "direction", input);
        sprintf(input, "%lf", hor_accuracy);
        put(handle, "hor_accuracy", input);
        sprintf(input, "%lf", ver_accuracy);
        put(handle, "ver_accuracy", input);
        commit(handle);
        free(handle);
    }
}

int get_stored_position(Position *position, Accuracy *accuracy, char *path)
{
    DBHandle *handle = NULL;
    handle = (DBHandle *) malloc(sizeof(DBHandle));
    xmlChar *result = NULL;
    int ret;

    if (position == NULL || accuracy == NULL) {
        if (handle != NULL)
            free(handle);

        return ERROR_NOT_AVAILABLE;
    }

    if (handle == NULL)
        return ERROR_NOT_AVAILABLE;
    else {
        if (isFileExists(path) == 0) return ERROR_NOT_AVAILABLE;

        handle->fileName = path;
        ret = get(handle, "timestamp", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else position->timestamp = atoll(result);

        ret = get(handle, "latitude", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else position->latitude = atof(result);

        ret = get(handle, "longitude", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else position->longitude = atof(result);

        ret = get(handle, "altitude", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else position->altitude = atof(result);

        ret = get(handle, "hor_accuracy", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else accuracy->horizAccuracy = atof(result);

        ret = get(handle, "ver_accuracy", &result);

        if (ret == KEY_NOT_FOUND) return ERROR_NOT_AVAILABLE;
        else accuracy->vertAccuracy = atof(result);

        free(handle);
    }

    return ERROR_NONE;
}

