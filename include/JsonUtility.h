/**********************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Red Rose Platform location
 * Team   : SWF Team
 * Security  : Confidential
 * ***********************************************************/

/*********************************************************
 * @file
 * Filename  : JsonUtility.h
 * Purpose  : Provides location related API to application
 * Platform  : RedRose
 * Author(s)  : Mohammed Sameer Mulla
 * E-mail id. : sameer.mulla@lge.com
 * Creation date :
 *
 * Modifications:
 *
 * Sl No Modified by  Date  Version Description
 *
 **********************************************************/
#ifndef _JSON_UTILITY_H_
#define _JSON_UTILITY_H_

#include <Position.h>
#include <Location.h>
#include <Address.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>
#include <pbnjson.h>
#include <sys/time.h>

#define SCHEMA_ANY                          "{}"
#define SCHEMA_NONE                         "{\"additionalProperties\":false}"

#define PROPS_1(p1)                         ",\"properties\":{" p1 "}"
#define PROPS_2(p1, p2)                     ",\"properties\":{" p1 "," p2 "}"
#define PROPS_3(p1, p2, p3)                 ",\"properties\":{" p1 "," p2 "," p3 "}"
#define PROPS_4(p1, p2, p3, p4)             ",\"properties\":{" p1 "," p2 "," p3 "," p4 "}"
#define PROPS_5(p1, p2, p3, p4, p5)         ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "}"
#define PROPS_6(p1, p2, p3, p4, p5, p6)     ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "}"
#define PROPS_7(p1, p2, p3, p4, p5, p6, p7) ",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "}"
#define REQUIRED_1(p1)                      ",\"required\":[\"" #p1 "\"]"
#define REQUIRED_2(p1, p2)                  ",\"required\":[\"" #p1 "\",\"" #p2 "\"]"
#define REQUIRED_3(p1, p2, p3)              ",\"required\":[\"" #p1 "\",\"" #p2 "\",\"" #p3 "\"]"
#define REQUIRED_4(p1, p2, p3, p4)          ",\"required\":[\"" #p1 "\",\"" #p2 "\",\"" #p3 "\",\"" #p4 "\"]"
#define STRICT_SCHEMA(attributes)           "{\"type\":\"object\"" attributes ",\"additionalProperties\":false}"
#define RELAXED_SCHEMA(attributes)          "{\"type\":\"object\"" attributes ",\"additionalProperties\":true}"

#define PROP(name, type)                    "\"" #name "\":{\"type\":\"" #type "\"}"
#define PROP_WITH_OPT(name, type, ...)      "\"" #name "\":{\"type\":\"" #type "\"," #__VA_ARGS__ "}"
#define ENUM_PROP(name, type, ...)          "\"" #name "\":{\"type\":\"" #type "\",\"enum\":[" #__VA_ARGS__ "]}"
#define ARRAY(name, type)                   "\"" #name "\":{\"type\":\"array\", \"items\":{\"type\":\"" #type "\"}}"
#define OBJSCHEMA_1(param)                  "{\"type\":\"object\",\"properties\":{" param "}}"
#define OBJSCHEMA_2(p1, p2)                 "{\"type\":\"object\",\"properties\":{" p1 "," p2 "}}"
#define OBJSCHEMA_3(p1, p2, p3)             "{\"type\":\"object\",\"properties\":{" p1 "," p2 ", " p3 "}}"
#define OBJSCHEMA_4(p1, p2, p3, p4)         "{\"type\":\"object\",\"properties\":{" p1 "," p2 ", " p3 ", " p4 "}}"
#define OBJECT(name, objschema)             "\"" #name "\":" objschema
#define STRICT_ENUM_ARRAY(name, type, ...)  "\"" #name "\":{\"type\":\"array\", \"items\":{\"type\":\"" #type "\",\"enum\":[" #__VA_ARGS__ "]}, \"minItems\": 1, \"additionalItems\": false, \"uniqueItems\": true }"


#ifdef __cplusplus
extern "C"
{
#endif

void location_util_add_pos_json(jvalue_ref serviceObject, Position *pos);
void location_util_add_acc_json(jvalue_ref serviceObject, Accuracy *acc);
void location_util_add_errorText_json(jvalue_ref serviceObject, char *errorText);
bool location_util_parsejsonAddress(jvalue_ref serviceObject, Address *addr);
void location_util_form_json_reply(jvalue_ref serviceObject, bool returnValue, int errorCode);
bool location_util_req_has_wakeup(LSMessage *msg);

#ifdef __cplusplus
}
#endif

#endif
