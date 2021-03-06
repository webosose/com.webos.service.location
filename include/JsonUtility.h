// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#ifndef _JSON_UTILITY_H_
#define _JSON_UTILITY_H_

#include <Position.h>
#include <Location.h>
#include <glib.h>
#include <glib-object.h>
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
