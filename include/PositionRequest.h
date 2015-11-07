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

#ifndef POSITIONREQUEST_H_
#define POSITIONREQUEST_H_

#include <string>
#include <location_errors.h>
#include <GeoLocation.h>
#include <HandlerRequestCommands.h>

#define printf_debug LS_LOG_DEBUG
#define printf_error LS_LOG_ERROR
#define printf_info LS_LOG_INFO
#define printf_warning LS_LOG_WARNING

using namespace std;

class PositionRequest {

private:


    typedef std::string ProviderID_t;
    typedef int PositionInterval_t;
    typedef string RequestParams_t;




    int mGeofenceID;
    double mGeofenceLatitude;
    double mGeofenceRadius;
    double mGeofenceLongitude;
    gint64 mTimeToFirstFix;
    ProviderID_t mProviderId;
    HandlerRequestType mRequestType;
    RequestParams_t mRequestParams;
    PositionInterval_t mDefaultInterval = 100;
    PositionInterval_t mPositionInterval;
    void* mpUserData;

public:

    //PositionRequest():
        //mProviderId(""), mRequestType(UNKNOWN_CMD), mPositionInterval(mDefaultInterval) {
    //}

    PositionRequest():mProviderId(""), mRequestType(UNKNOWN_CMD), mPositionInterval(mDefaultInterval),
                mRequestParams(""),
                            mpUserData(nullptr),
                            mGeofenceID(0),
                            mGeofenceLatitude(0.0),
                            mGeofenceRadius(0.0),
                            mGeofenceLongitude(0.0),
                            mTimeToFirstFix(0){
    }


    PositionRequest(ProviderID_t providerId, HandlerRequestType requestType):
                  mProviderId(providerId), mRequestType(requestType), mPositionInterval(mDefaultInterval) {
    }

    const std::string& getProviderId() const {
    return mProviderId;
    }

    void setProviderId(const std::string& providerId) {
    mProviderId = providerId;
    }

    ~PositionRequest() {};

    const string& getRequestParams() const {
        return mRequestParams;
    }

    void setRequestParams(const string requestParams) {
        mRequestParams = requestParams;
    }


    PositionInterval_t getPositionInterval() const {
    return mPositionInterval;
    }

    void setPositionInterval(PositionInterval_t positionInterval) {
    mPositionInterval = positionInterval;
    }

 void printRequest() {
    printf_info("===>position-request: \n");
    printf_info("[mProvider_id]: %s\n", mProviderId.c_str());
    printf_info("[mPosition_interval]: %d\n", mPositionInterval);
 }
    HandlerRequestType getRequestType() const {
    return mRequestType;
    }

    void setRequestType(HandlerRequestType requestType) {
        mRequestType = requestType;
    }

#if 0
    bool getEnableGPSData() const {
        return mEnableGPSData;
    }

    void setEnableGPSData(bool enable) {
        mEnableGPSData = enable;
    }

    bool getGPSForceStop() const {
        return mGPSForceStop;
    }

    void setGPSForceStop(bool bForceStop) {
        mGPSForceStop = bForceStop;
    }
#endif
    void* getUserData() const {
        return mpUserData;
    }

    void setUserData(void* userData) {
        mpUserData = userData;
    }


    void setGeofenceID (int geofenceId){
        mGeofenceID = geofenceId;
        }

    int getGeofenceID (){
        return mGeofenceID;
    }

    void setGeofenceCoordinates (double longitude, double latitude, double radius){
        this->mGeofenceLongitude = longitude;
        this->mGeofenceLatitude = latitude;
        this->mGeofenceRadius = radius;
    }

    double getGeofenceLongitude (){
        return mGeofenceLongitude;
    }

    double getGeofenceLatitude (){
        return mGeofenceLatitude;
    }

    double getGeofenceRadius (){
        return mGeofenceRadius;
    }




};

#endif /* POSITIONREQUEST_H_ */
