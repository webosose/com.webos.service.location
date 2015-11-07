// @@@LICENSE
//
//      Copyright (c) 2015 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef ILOCATIONCALLBACKS_H_
#define ILOCATIONCALLBACKS_H_

#include <glib.h>
#include <GeoLocation.h>
#include <location_errors.h>
#include <Location.h>

class ILocationCallbacks {
public:
    virtual void getLocationUpdateCb(GeoLocation location, ErrorCodes errCode,
            HandlerTypes type)=0;

    virtual void getNmeaDataCb(long long timestamp, char *data, int length)=0;

    virtual void getGpsStatusCb(int state)=0;

    virtual void getGpsSatelliteDataCb(Satellite *)=0;

    virtual void geofenceAddCb(int32_t geofence_id, int32_t status,
            gpointer user_data)=0;

    virtual void geofenceRemoveCb(int32_t geofence_id, int32_t status,
            gpointer user_data)=0;

    virtual void geofencePauseCb(int32_t geofence_id, int32_t status,
            gpointer user_data)=0;

    virtual void geofenceResumeCb(int32_t geofence_id, int32_t status,
            gpointer user_data)=0;

    virtual void geofenceBreachCb(int32_t geofence_id, int32_t status,
            int64_t timestamp, double latitude, double longitude,
            gpointer user_data)=0;

    virtual void geofenceStatusCb(int32_t status, Position *last_position,
            Accuracy *accuracy, gpointer user_data)=0;
    virtual ~ILocationCallbacks() {
    }
};
#endif /* ILOCATIONCALLBACKS_H_ */
