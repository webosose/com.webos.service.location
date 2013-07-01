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
#include "Gps_stored_data.h"
#include <Position.h>
#include <Accuracy.h>

void initializeLocationDB(){
    DBHandle* handle = NULL;


    xmlChar* result =NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));
    if(handle != NULL) {
        createPreference(LOCATION_DB_PREF_PATH , handle ,"Location\n", TRUE);
        char input[50];

        guint64 timestamp = 0;

        gdouble lat = 0.0;
        gdouble lon  = 0.0;
        gdouble alt = 0.0;
        gdouble spd  = 0.0;
        gdouble dir = 0.0;
        gdouble h_acc = 0.0;
        gdouble v_acc = 0.0;

        sprintf(input, "%lld", timestamp);
        put(handle , "timestamp" ,input);

        sprintf(input,"%lf",lat);
        put(handle , "latitude" ,input);

        sprintf(input,"%lf",lon);
        put(handle , "longitude" ,input);

        sprintf(input,"%lf",alt);
        put(handle , "altitude" ,input);

        sprintf(input,"%lf",spd);
        put(handle , "speed" ,input);

        sprintf(input,"%lf",dir);
        put(handle , "direction" ,input);

        sprintf(input,"%lf",h_acc);
        put(handle , "hor_accuracy" ,input);
        sprintf(input,"%lf",v_acc);
        put(handle , "ver_accuracy" ,input);

        commit(handle);
        free(handle);
    }



}
/**
 * <Funciton >   gps_service_get_stored_position
 * <Description>   will be called for getting the stored position
 *      .
 * @param     <last_pos> <OUT> <get Stored position data>
 * @throws
 * @return     void
 */


void gps_service_get_stored_position(position_data * last_pos)
{
    DBHandle* handle = NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));

    xmlChar *result =NULL;

    if(handle == NULL){
        if(DEBUG)
            g_print("gps_service_get_stored_position ");


        last_pos->latitude = 0.0;
        last_pos->longitude = 0.0;
        last_pos->altitude = 0.0;
        last_pos->speed = 0;
        last_pos->bearing = 0;
        last_pos->hor_accuracy = 0;
        last_pos->ver_accuracy = 0;
        last_pos->timestamp =0;
    }
    else {


        handle->fileName= LOCATION_DB_PREF_PATH;

        get(handle , "latitude", &result);

        if(DEBUG)
            g_print("latitud %f",atof ( result ));
        last_pos->latitude = atof ( result );

        get(handle , "longitude", &result);
        last_pos->longitude = atof ( result );

        get(handle, "altitude", &result);
        last_pos->altitude = atof ( result );
        get(handle , "speed", &result);
        last_pos->speed = atof ( result );
        get(handle , "direction", &result);
        last_pos->bearing = atof ( result );
        get(handle , "hor_accuracy", &result);
        last_pos->hor_accuracy  = atof ( result );
        get(handle , "ver_accuracy", &result);
        last_pos->ver_accuracy =  atof ( result );
        get(handle , "timestamp", &result);
        last_pos->timestamp = atoi ( result );
        free(handle);
    }





}



/**
 * <Funciton >   gps_service_get_stored_position
 * <Description>   will be called for getting the stored position
 *      .
 * @param     <last_pos> <In> <Stored the position data>
 * @throws
 * @return     Void
 */


void gps_service_store_position(position_data * last_pos)
{
    DBHandle* handle = NULL;

    xmlChar* result =NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));
    if(handle != NULL) {
        createPreference(LOCATION_DB_PREF_PATH , handle ,"Location\n", FALSE);
        char input[50];

        guint64 timestamp = last_pos->timestamp/1000;

        gdouble lat = last_pos->latitude;
        gdouble lon  = last_pos->longitude;
        gdouble alt = last_pos->altitude;
        gdouble spd  = last_pos->speed;
        gdouble dir = last_pos->bearing;
        gdouble h_acc = last_pos->hor_accuracy;
        gdouble v_acc = last_pos->ver_accuracy;

        sprintf(input, "%lld", timestamp);
        put(handle , "timestamp" ,input);

        sprintf(input,"%lf",lat);
        put(handle , "latitude" ,input);

        sprintf(input,"%lf",lon);
        put(handle , "longitude" ,input);

        sprintf(input,"%lf",alt);
        put(handle , "altitude" ,input);

        sprintf(input,"%lf",spd);
        put(handle , "speed" ,input);

        sprintf(input,"%lf",dir);
        put(handle , "direction" ,input);

        sprintf(input,"%lf",h_acc);
        put(handle , "hor_accuracy" ,input);
        sprintf(input,"%lf",v_acc);
        put(handle , "ver_accuracy" ,input);

        commit(handle);
        free(handle);
    }
    // TO DO: How to store vale in DB need to check

}

int gps_plugin_get_stored_position(Position *position, Accuracy *accuracy )
{
    DBHandle* handle = NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));

    xmlChar *result =NULL;
    int ret;
    if(handle == NULL)
        return ERROR_NOT_AVAILABLE;
    else {

        if(isFileExists() == 0)
            initializeLocationDB();
        handle->fileName= LOCATION_DB_PREF_PATH;
        ret = get(handle , "timestamp", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->timestamp = atoll ( result );

        ret = get(handle , "latitude", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->latitude = atof ( result );

        ret = get(handle , "longitude", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->longitude = atof ( result );

        ret = get(handle, "altitude", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->altitude = atof ( result );

        ret = get(handle , "hor_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            accuracy->horizAccuracy  = atof ( result );

        ret = get(handle , "ver_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            accuracy->vertAccuracy =  atof ( result );

        free(handle);
    }
    return ERROR_NONE;


}


int gps_plugin_get_stored_velocity(guint64 *timestamp, gdouble *speed,  gdouble *direction , gdouble * climb, gdouble *hor_accuracy, gdouble *ver_accuracy){


    DBHandle* handle = NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));
    int ret = UNKNOWN_ERROR;
    xmlChar *result =NULL;

    if(handle == NULL)
        return ERROR_NOT_AVAILABLE;
    else {


        handle->fileName= LOCATION_DB_PREF_PATH;
        ret = get(handle , "timestamp", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *timestamp = atoll ( result );

        ret = get(handle , "speed", &result);

        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *speed = atof ( result );

        ret = get(handle , "direction", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *direction = atof ( result );

        ret = get(handle, "climb", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *climb = atof ( result );

        ret = get(handle , "hor_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *hor_accuracy  = atof ( result );
        ret = get(handle , "ver_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            *ver_accuracy =  atof ( result );

        free(handle);
    }
    return ERROR_NONE;
}


int wifi_plugin_get_stored_position(Position *position, Accuracy *accuracy)
{
    DBHandle* handle = NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));
    int ret = UNKNOWN_ERROR;
    xmlChar *result =NULL;

    if(handle == NULL)
        return ERROR_NOT_AVAILABLE;
    else {


        handle->fileName= LOCATION_DB_PREF_PATH;
        ret = get(handle , "wifi_timestamp", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->timestamp = atoll ( result );
        ret = get(handle , "wifi_latitude", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->latitude = atof ( result );

        ret = get(handle , "wifi_longitude", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            position->longitude = atof ( result );


        ret = get(handle , "wifi_hor_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            accuracy->horizAccuracy  = atof ( result );

        ret = get(handle , "wifi_ver_accuracy", &result);
        if(ret == KEY_NOT_FOUND)
            return ERROR_NOT_AVAILABLE;
        else
            accuracy->vertAccuracy =  atof ( result );

        free(handle);
    }
    return ERROR_NONE;
}

void wifi_plugin_set_stored_position(guint64 timestamp, gdouble longitude, gdouble latitude , gdouble hor_accuracy, gdouble ver_accuracy)
{
    DBHandle* handle = NULL;

    xmlChar* result =NULL;
    handle = (DBHandle*) malloc(sizeof(DBHandle));
    if(handle != NULL) {
        createPreference(LOCATION_DB_PREF_PATH , handle ,"Location\n", FALSE);
        char input[50];


        sprintf(input, "%lld", timestamp);
        put(handle , "timestamp" ,input);

        sprintf(input,"%lf",latitude);
        put(handle , "wifi_latitude" ,input);

        sprintf(input,"%lf",longitude);
        put(handle , "wifi_longitude" ,input);

        sprintf(input,"%lf",hor_accuracy);
        put(handle , "wifi_hor_accuracy" ,input);
        sprintf(input,"%lf",ver_accuracy);
        put(handle , "wifi_ver_accuracy" ,input);

        commit(handle);
        free(handle);
    }
}
