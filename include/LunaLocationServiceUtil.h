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
// temproray so keeping hardcoded value
#define SECURE_PAYLOAD_NW_SET      "{\"keyname\":\"com.palm.location.geolocation\",\"pdata\":\"AIzaSyC3gspakPjzPZ-zctl0JBxKVtSlTBSL4BY\",\"cdata\":39}"
#define SECURE_PAYLOAD_LBS_SET     "{\"keyname\":\"com.palm.location.geocode\",\"pdata\":\"CDgvGzYI33BE0XvF54xrqaDmBrY=\",\"cdata\":28}"

#define SECURE_PAYLOAD_NW_GET      "{\"keyname\":\"com.palm.location.geolocation\"}"
#define SECURE_PAYLOAD_LBS_GET     "{\"keyname\":\"com.palm.location.geocode\"}"
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
#define JSCHEMA_GET_ALL_LOCATION_HANDLERS                   STRICT_SCHEMA(\
        PROPS_1(PROP(subscribe, boolean)))

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
 * JSON SCHEMA: getGeoCodeLocation (string address) - syntax 1
 */

#define JSCHEMA_GET_GOOGLE_GEOCODE_LOCATION                 STRICT_SCHEMA(\
        PROPS_5(\
            PROP(address, string), \
            OBJECT(components, STRICT_SCHEMA(\
                PROPS_5(\
                    PROP(route, string), \
                    PROP(locality, string), \
                    PROP(administrative_area, string), \
                    PROP(postal_code, string), \
                    PROP(country, string)\
                )\
            )), \
            OBJECT(bounds, STRICT_SCHEMA(\
                PROPS_4(\
                    PROP_WITH_OPT(southwestLat, number, "minimum":-90, "maximum":90), \
                    PROP_WITH_OPT(southwestLon, number, "minimum":-180, "maximum":180), \
                    PROP_WITH_OPT(northeastLat, number, "minimum":-90, "maximum":90), \
                    PROP_WITH_OPT(northeastLon, number, "minimum":-180, "maximum":180)\
                ) \
                REQUIRED_4(southwestLat, southwestLon, northeastLat, northeastLon)\
            )), \
            PROP(language, string), \
            PROP(region, string)\
        ))

/*
* JSON SCHEMA: getReverseLocation
*/
#define JSCHEMA_GET_GOOGLE_REVERSE_LOCATION                 STRICT_SCHEMA(\
        PROPS_5(\
            PROP_WITH_OPT(latitude, number, "minimum":-90, "maximum":90), \
            PROP_WITH_OPT(longitude, number, "minimum":-180, "maximum":180), \
            PROP(language, string), \
            STRICT_ENUM_ARRAY(result_type, string, \
                "street_address", \
                "route", \
                "intersection", \
                "political", \
                "country", \
                "administrative_area_level_1", \
                "administrative_area_level_2", \
                "administrative_area_level_3", \
                "administrative_area_level_4", \
                "administrative_area_level_5", \
                "colloquial_area", \
                "locality", \
                "ward", \
                "sublocality", \
                "neighborhood", \
                "premise", \
                "subpremise", \
                "postal_code", \
                "natural_feature", \
                "airport", \
                "park", \
                "point_of_interest", \
                "floor", \
                "establishment", \
                "parking", \
                "post_box", \
                "postal_town", \
                "room", \
                "street_number", \
                "bus_station", \
                "train_station", \
                "transit_station"\
            ), \
            STRICT_ENUM_ARRAY(location_type, string, \
                "ROOFTOP", \
                "RANGE_INTERPOLATED", \
                "GEOMETRIC_CENTER", \
                "APPROXIMATE"\
            )\
        ) \
        REQUIRED_2(latitude, longitude))

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
        PROPS_2(PROP_WITH_OPT(latitude, number, "minimum":-90, "maximum":90), \
        PROP_WITH_OPT(longitude, number, "minimum":-180, "maximum":180)) \
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
        PROPS_4(ENUM_PROP(posmode, integer, 0, 1, 2), PROP(supladdress, string), \
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

/*
 * JSON SCHEMA: addGeofenceArea (double latitude, double longitude, double radius, [bool subscribe])
 */
#define JSCHEMA_ADD_GEOFENCE_AREA                           STRICT_SCHEMA(\
        PROPS_4(PROP_WITH_OPT(latitude, number, "minimum":-90, "maximum":90), \
                PROP_WITH_OPT(longitude, number, "minimum":-180, "maximum":180), \
                PROP_WITH_OPT(radius, number, "minimum":0, "exclusiveMinimum":true), \
                PROP(subscribe, boolean)) \
        REQUIRED_3(latitude, longitude, radius))

/*
 * JSON SCHEMA: removeGeofenceArea (integer geofenceid)
 */
#define JSCHEMA_REMOVE_GEOFENCE_AREA                        STRICT_SCHEMA(\
        PROPS_1(PROP_WITH_OPT(geofenceid, integer, "minimum":0, "exclusiveMinimum":true)) \
        REQUIRED_1(geofenceid))

/*
 * JSON SCHEMA: pauseGeofenceArea (integer geofenceid)
 */
#define JSCHEMA_PAUSE_GEOFENCE_AREA                         STRICT_SCHEMA(\
        PROPS_1(PROP(geofenceid, integer)) \
        REQUIRED_1(geofenceid))

/*
 * JSON SCHEMA: resumeGeofenceArea (integer geofenceid)
 */
#define JSCHEMA_RESUME_GEOFENCE_AREA                        STRICT_SCHEMA(\
        PROPS_1(PROP(geofenceid, integer)) \
        REQUIRED_1(geofenceid))

/*
 * JSON SCHEMA: getLocationUpdates ([bool subscribe],
 *                                  [integer minimumInterval],
 *                                  [integer minimumDistance],
 *                                  [integer responseTimeout],
 *                                  [string Handler])
 */

#define JSCEHMA_GET_LOCATION_UPDATES                        STRICT_SCHEMA(\
        PROPS_5(\
            PROP(subscribe, boolean), \
            PROP_WITH_OPT(minimumInterval, integer, "minimum":0, "maximum":3600000), \
            PROP_WITH_OPT(minimumDistance, integer, "minimum":0, "maximum":60000), \
            PROP_WITH_OPT(responseTimeout, integer, "minimum":0, "maximum":720), \
            ENUM_PROP(Handler, string, "gps", "network", "passive")\
        ))


/*
 * JSON SCHEMA: getCachedPosition ([integer maximumAge], [string Handler])
 */


#define JSCHEMA_GET_CACHED_POSITION                         STRICT_SCHEMA(\
        PROPS_2(\
            PROP_WITH_OPT(maximumAge, integer, "minimum":0, "exclusiveMinimum": true), \
            ENUM_PROP(Handler, string, "gps", "network", "passive")))


bool LSMessageInitErrorReply();

void LSMessageReleaseErrorReply();

char *LSMessageGetErrorReply(int errorCode);

void LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode);
bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message);
void LSMessageReplySuccess(LSHandle *sh, LSMessage *message);
bool LSMessageValidateSchema(LSHandle *sh, LSMessage *message,const char *schema, jvalue_ref *parsedObj);
void securestorage_set(LSHandle *sh, void *ptr);
#endif
