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

#ifndef GPSSERVICECONFIG_H_
#define GPSSERVICECONFIG_H_

using namespace std;

#include <string>
#include <gps_cfg.h>

#define    GPS_CONF_FILE    "/etc/gps.conf"

class GPSServiceConfig {
public:
    GPSServiceConfig() {
        loadDefaults();
        loadConfigFile(GPS_CONF_FILE);
    };

    GPSServiceConfig(std::string configFileName) {
        loadDefaults();
        loadConfigFile(configFileName);
    };

    void loadDefaults();

private:
    void loadConfigFile(std::string ConfigFileName);

public:
    unsigned long mSUPLVer;
    char mSUPLHost[GPS_MAX_PARAM_STRING];
    unsigned long mSUPLPort;
    char mXtraServer1[GPS_MAX_PARAM_STRING];
    char mXtraServer2[GPS_MAX_PARAM_STRING];
    char mXtraServer3[GPS_MAX_PARAM_STRING];
    char mNTPServer1[GPS_MAX_PARAM_STRING];
    char mNTPServer2[GPS_MAX_PARAM_STRING];
    char mNTPServer3[GPS_MAX_PARAM_STRING];
    char mVENDOR[GPS_MAX_PARAM_STRING];
    unsigned long mLgeTlsMode;
    unsigned long mLgeGPSPositionMode;
    char mChipsetID[GPS_MAX_PARAM_STRING];
};

#endif /* GPSSERVICECONFIG_H_ */
