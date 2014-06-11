/* @@@LICENSE
 *
 *      Copyright (c) 2007-2012 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * LICENSE@@@ */

#ifndef _LUNA_LOCATION_SERVICE_UTIL_H_
#define _LUNA_LOCATION_SERVICE_UTIL_H_

#include <lunaservice.h>
#include "JsonUtility.h"

/*
 * JSON SCHEMA: criteria/getLocationCriteriaHandlerDetails (string Handler)
 */
#define JSCHEMA_GET_LOCATION_CRITERIA_HANDLER_DETAILS       STRICT_SCHEMA(\
        PROPS_1(ENUM_PROP(Handler, string, "gps", "network")) \
        REQUIRED_1(Handler))

/*
 * JSON SCHEMA: criteria/startTrackingCriteriaBased ([bool subscribe],
 *                                                   [int minimumInterval],
 *                                                   [int minimumDistance],
 *                                                   [string Handler],
 *                                                   [int accuracyLevel],
 *                                                   [int powerLevel])
 */
#define JSCHEMA_START_TRACKING_CRITERIA_BASED               STRICT_SCHEMA(\
        PROPS_6(PROP(subscribe, boolean), PROP(minimumInterval, integer), \
        PROP(minimumDistance, integer), ENUM_PROP(Handler, string, "gps", "network"), \
        ENUM_PROP(accuracyLevel, integer, 1, 2), ENUM_PROP(powerLevel, integer, 1, 2)))

/*
 * JSON SCHEMA: getAllLocationHandlers ()
 */
#define JSCHEMA_GET_ALL_LOCATION_HANDLERS                   SCHEMA_ANY

/*
 * JSON SCHEMA: getCurrentPosition ([int accuracy], [int maximumAge], [int responseTime])
 */
#define JSCHEMA_GET_CURRENT_POSITION                        STRICT_SCHEMA(\
        PROPS_3(ENUM_PROP(accuracy, integer, 1, 2, 3), PROP(maximumAge, integer), \
        ENUM_PROP(responseTime, integer, 1, 2, 3)))

/*
 * JSON SCHEMA: getGeoCodeLocation (string address) - syntax 1
 */
#define JSCHEMA_GET_GEOCODE_LOCATION_1                      STRICT_SCHEMA(\
        PROPS_1(PROP(address, string)) \
        REQUIRED_1(address))

/*
 * JSON SCHEMA: getGeoCodeLocation ([string street], [string locality],
 *                                  [string area], [string region],
 *                                  [string country], [string countrycode],
 *                                  [string postcode]) - syntax 2
 */
#define JSCHEMA_GET_GEOCODE_LOCATION_2                      STRICT_SCHEMA(\
        PROPS_7(PROP(street, string), PROP(locality, string), \
        PROP(area, string), PROP(region, string), \
        PROP(country, string), PROP(countrycode, string), \
        PROP(postcode, string)))

/*
 * JSON SCHEMA: getGpsSatelliteData ([bool subscribe])
 */
#define JSCHEMA_GET_GPS_SATELLITE_DATA                      STRICT_SCHEMA(\
        PROPS_1(PROP(subscribe, boolean)))

/*
 * JSON SCHEMA: getGpsStatus ([bool subscribe])
 */
#define JSCHEMA_GET_GPS_STATUS                              STRICT_SCHEMA(\
        PROPS_1(PROP(subscribe, boolean)))

/*
 * JSON SCHEMA: getLocationHandlerDetails (string Handler)
 */
#define JSCHEMA_GET_LOCATION_HANDLER_DETAILS                STRICT_SCHEMA(\
        PROPS_1(ENUM_PROP(Handler, string, "gps", "network")) \
        REQUIRED_1(Handler))

/*
 * JSON SCHEMA: getNmeaData ([bool subscribe])
 */
#define JSCHEMA_GET_NMEA_DATA                               STRICT_SCHEMA(\
        PROPS_1(PROP(subscribe, boolean)))

/*
 * JSON SCHEMA: getReverseLocation (double latitude, double longitude)
 */
#define JSCHEMA_GET_REVERSE_LOCATION                        STRICT_SCHEMA(\
        PROPS_2(PROP(latitude, number), PROP(longitude, number)) \
        REQUIRED_2(latitude, longitude))

/*
 * JSON SCHEMA: getState (string Handler, [bool subscribe])
 */
#define JSCHEMA_GET_STATE                                   STRICT_SCHEMA(\
        PROPS_2(ENUM_PROP(Handler, string, "gps", "network"), PROP(subscribe, boolean)) \
        REQUIRED_1(Handler))

/*
 * JSON SCHEMA: getTimeToFirstFix ()
 */
#define JSCHEMA_GET_TIME_TO_FIRST_FIX                       SCHEMA_ANY

/*
 * JSON SCHEMA: sendExtraCommand (string command)
 */
#define JSCHEMA_SEND_EXTRA_COMMAND                          STRICT_SCHEMA(\
        PROPS_1(PROP(command, string)) \
        REQUIRED_1(command))

/*
 * JSON SCHEMA: setGPSParameters ([int posmode], [string supladdress],
 *                                [int suplport], [int fixinterval])
 */
#define JSCHEMA_SET_GPS_PARAMETERS                          STRICT_SCHEMA(\
        PROPS_4(ENUM_PROP(posmode, integer, 1, 2, 3), PROP(supladdress, string), \
        PROP(suplport, integer), PROP(fixinterval, integer)))

/*
 * JSON SCHEMA: setState (string Handler, bool state)
 */
#define JSCHEMA_SET_STATE                                   STRICT_SCHEMA(\
        PROPS_2(ENUM_PROP(Handler, string, "gps", "network"), PROP(state, boolean)) \
        REQUIRED_2(Handler, state))

/*
 * JSON SCHEMA: startTracking ([bool subscribe])
 */
#define JSCHEMA_START_TRACKING                              STRICT_SCHEMA(\
        PROPS_1(PROP(subscribe, boolean)))

/*
 * JSON SCHEMA: stopGPS ()
 */
#define JSCHEMA_STOP_GPS                                    SCHEMA_ANY

void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode, char *errorText);
bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message);
void LSMessageReplySuccess(LSHandle *sh, LSMessage *message);
bool LSMessageValidateSchema(LSHandle *sh, LSMessage *message,const char *schema, jvalue_ref *parsedObj);
#endif
