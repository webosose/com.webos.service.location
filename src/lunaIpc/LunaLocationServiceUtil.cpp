/**
 @file      LunaLocationService_Util.cpp
 @brief     Utility File to handle error conditions while parsing JSON objects recieved
 Null Checks on memory allocations, variables recieved in callbacks

 @author    Sameer
 @date      2013-03-22
 @version   TDB
 @todo      TDB

 @copyright Copyright (c) 2012 LG Electronics.  All rights reserved.
 */

#include "LunaLocationServiceUtil.h"
#include "ServiceAgent.h"
#include <LocationService.h>

#define GEOLOCKEY_CONFIG_PATH       "/etc/geolocation.conf"
#define GEOCODEKEY_CONFIG_PATH      "/etc/geocode.conf"


LocationService *loc_svc_ptr = NULL;

typedef struct locErrorTextPair {
    LocationErrorCode code;
    const char *text;
} locErrorTextPair_t;

static const locErrorTextPair_t mapLocErrorText[LOCATION_ERROR_MAX] = {
        {LOCATION_SUCCESS,                     "Success"},
        {LOCATION_TIME_OUT,                    "Time out"},
        {LOCATION_POS_NOT_AVAILABLE,           "Position not available"},
        {LOCATION_UNKNOWN_ERROR,               "Unknown error"},
        {LOCATION_GPS_UNAVAILABLE,             "Gps not supported"},
        {LOCATION_LOCATION_OFF,                "Location source is off"},
        {LOCATION_PENDING_REQ,                 "Request pending"},
        {LOCATION_APP_BLACK_LISTED,            "Application is blacklisted"},
        {LOCATION_START_FAILURE,               "Handler start failure"},
        {LOCATION_STATE_UNKNOWN,               "State unknown"},
        {LOCATION_INVALID_INPUT,               "Invalid input"},
        {LOCATION_DATA_CONNECTION_OFF,         "No internet connection"},
        {LOCATION_WIFI_CONNECTION_OFF,         "Wifi is not turned on"},
        {LOCATION_OUT_OF_MEM,                  "Out of memory"},
        {LOCATION_GEOFENCE_TOO_MANY_GEOFENCE,  "Too many geofences are added"},
        {LOCATION_GEOFENCE_ID_EXIST,           "Geofence id already exists"},
        {LOCATION_GEOFENCE_ID_UNKNOWN,         "Geofecne id unknown"},
        {LOCATION_GEOFENCE_INVALID_TRANSITION, "Geofence invalid transition"}
};

char *locationErrorReply[LOCATION_ERROR_MAX] = {0};


bool LSMessageInitErrorReply() {
    int i, j;

    LSMessageReleaseErrorReply();

    for (i = 0; i < LOCATION_ERROR_MAX; i++) {
        locationErrorReply[i] = g_strdup_printf("{\"returnValue\":true,\"errorCode\":%d,\"errorText\":\"%s\"}",
                                                mapLocErrorText[i].code,
                                                mapLocErrorText[i].text);

        if (locationErrorReply[i] == NULL) {
            //release all previous allocated memory in array
            for (j = i - 1; j >= 0; j--) {
                g_free(locationErrorReply[j]);
                locationErrorReply[j] = NULL;
            }

            return false;
        }
    }

    return true;
}

void LSMessageReleaseErrorReply() {
    int i;

    for (i = 0; i < LOCATION_ERROR_MAX; i++) {
        if (locationErrorReply[i]) {
            g_free(locationErrorReply[i]);
            locationErrorReply[i] = NULL;
        }
    }
}

char *LSMessageGetErrorReply(int errorCode) {
    if (errorCode < 0 || errorCode >= LOCATION_ERROR_MAX)
        return NULL;

    return locationErrorReply[errorCode];
}

void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode) {
    LSError lserror;

    if (errorCode < 0 || errorCode >= LOCATION_ERROR_MAX)
        return;

    if (locationErrorReply[errorCode] == NULL)
       return;

    LSErrorInit(&lserror);

    if (!LSMessageReply(sh, message, locationErrorReply[errorCode], &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message) {
    const char *subscribe_answer = "{\"returnValue\":true, \"subscribed\":true}";
    LSError lsError;
    LSErrorInit(&lsError);

    if (LSMessageReply(sh, message, subscribe_answer, &lsError) == false) {
        LSErrorPrint(&lsError, stderr);
        LSErrorFree(&lsError);
        return false;
    }

    return true;
}

void LSMessageReplySuccess(LSHandle *sh, LSMessage *message) {
    LSError lserror;
    LSErrorInit(&lserror);
    bool retVal = LSMessageReply(sh, message, "{\"returnValue\":true}", &lserror);

    if (retVal == false) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool LSMessageValidateSchemaReplyOnError(LSHandle *sh, LSMessage *message, const char *schema, jvalue_ref *parsedObj) {
    bool ret = false;
    jschema_ref input_schema = jschema_parse(j_cstr_to_buffer(schema), DOMOPT_NOOPT, NULL);

    if (!input_schema)
        return false;

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    *parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&input_schema);

    if (jis_null(*parsedObj)) {
        input_schema = jschema_parse(j_cstr_to_buffer(SCHEMA_ANY), DOMOPT_NOOPT, NULL);
        jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
        *parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

        if (!jis_null(*parsedObj))
            j_release(parsedObj);

        jschema_release(&input_schema);

        LSMessageReplyError(sh, message, LOCATION_INVALID_INPUT);
    } else {
        ret = true;
    }

    return ret;
}

bool secure_storage_get_cb(LSHandle *sh, LSMessage *reply, void *ctx) {
    jvalue_ref parsedObj = NULL;
    jvalue_ref jsonSubObject = NULL;
    raw_buffer nameBuf;

    if (ctx == NULL) {
        return true;
    }

    if (!LSMessageValidateSchemaReplyOnError(sh, reply, SCHEMA_ANY, &parsedObj)) {
        return true;
    }

    if (strcmp("nwkey", (char *) ctx) == 0) {
        //store key
        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("buff"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);

            if (loc_svc_ptr) {
                loc_svc_ptr->updateNwKey(nameBuf.m_str);
            }

            jstring_free_buffer(nameBuf);
        }
    } else if (strcmp("lbskey", (char *) ctx) == 0) {

        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("buff"), &jsonSubObject)) {
            nameBuf = jstring_get(jsonSubObject);

            if (loc_svc_ptr) {
                loc_svc_ptr->updateLbsKey(nameBuf.m_str);
            }
            jstring_free_buffer(nameBuf);
        }
    }

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

bool securestorage_get(LSHandle *sh, void *ptr) {
    LSError lsError;
    LSErrorInit(&lsError);
    loc_svc_ptr = (LocationService *) ptr;

    if (!LSCall(sh,
                SECURE_STORAGE_KEY_STORAGE_GET,
                SECURE_PAYLOAD_NW_GET,
                secure_storage_get_cb,
                (char *) "nwkey",
                NULL,
                &lsError)) {
        LSErrorPrint(&lsError, stderr);
        LSErrorFree(&lsError);

        return false;
    }

    if (!LSCall(sh,
                SECURE_STORAGE_KEY_STORAGE_GET,
                SECURE_PAYLOAD_LBS_GET,
                secure_storage_get_cb,
                (char *) "lbskey",
                NULL,
                &lsError)) {
        LSErrorPrint(&lsError, stderr);
        LSErrorFree(&lsError);

        return false;
    }

    return true;
}
Position *position_create(gint64 timestamp,
                          gdouble latitude,
                          gdouble longitude,
                          gdouble altitude,
                          gdouble speed,
                          gdouble direction,
                          gdouble climb,
                          int flags)
{
    if (latitude < -90 || latitude > 90)
        return NULL;

    if (longitude < -180 || longitude > 180)
        return NULL;

    Position *position = g_slice_new0(Position);
    g_return_val_if_fail(position, NULL);
    position->timestamp = timestamp;

    if (flags & POSITION_FIELDS_NONE)
        return position;

    if (flags & POSITION_FIELDS_LATITUDE)
        position->latitude = latitude;

    if (flags & POSITION_FIELDS_LONGITUDE)
        position->longitude = longitude;

    if (flags & POSITION_FIELDS_ALTITUDE)
        position->altitude = altitude;

    if (flags & VELOCITY_FIELDS_SPEED)
        position->speed = speed;

    if (flags & VELOCITY_FIELDS_DIRECTION)
        position->direction = direction;

    if (flags & VELOCITY_FIELDS_CLIMB)
        position->climb = climb;

    return position;
}