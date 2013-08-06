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

#include <cjson/json.h>
#include <Position.h>
#include <Location.h>
#include <Address.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>

#ifdef __cplusplus
extern "C"
{
#endif
void location_util_add_pos_json(struct json_object *, Position *);
void location_util_add_acc_json(struct json_object *, Accuracy *);
void location_util_add_vel_json(struct json_object *, Velocity *);
void location_util_add_returnValue_json(struct json_object *serviceObject, bool returnValue);
void location_util_add_errorCode_json(struct json_object *serviceObject, int errorCode);
void testjson(struct json_object *serviceObject);
void location_util_parsejsonAddress(struct json_object *m_JsonArgument, Address *addr);
#ifdef __cplusplus
}
#endif

#endif
