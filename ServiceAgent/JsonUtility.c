/**********************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Red Rose Platform location
 * Team         : SWF Team
 * Security     : Confidential
 * ***********************************************************/

/*********************************************************
 * @file
 * Filename     : JsonUtility.c
 * Purpose      : Json Utility
 * Platform     : RedRose
 * Author(s)    : Mohammed Sameer Mulla
 * E-mail id.   : sameer.mulla@lge.com
 * Creation date:
 *
 * Modifications:
 *
 * Sl No        Modified by     Date        Version Description
 *
 **********************************************************/
#include <cjson/json.h>
#include <Position.h>
#include <Accuracy.h>
#include <Address.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>

void location_util_add_pos_json(struct json_object* serviceObject, Position* pos)
{
    if (!serviceObject || !pos) return;

    json_object_object_add(serviceObject, "latitude", json_object_new_double(pos->latitude));
    json_object_object_add(serviceObject, "longitude", json_object_new_double(pos->longitude));
    json_object_object_add(serviceObject, "altitude", json_object_new_double(pos->altitude));
    json_object_object_add(serviceObject, "timestamp", json_object_new_int(pos->timestamp));
    json_object_object_add(serviceObject, "heading", json_object_new_double(pos->direction));
    json_object_object_add(serviceObject, "velocity", json_object_new_double(pos->speed));
}

void location_util_add_returnValue_json(struct json_object* serviceObject, bool returnValue)
{
    if (!serviceObject) return;

    json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(returnValue));
}

void location_util_add_errorCode_json(struct json_object* serviceObject, int errorCode)
{
    if (!serviceObject) return;

    json_object_object_add(serviceObject, "errorCode", json_object_new_int(errorCode));
}

void location_util_add_acc_json(struct json_object* serviceObject, Accuracy* acc)
{
    if (!serviceObject || !acc) return;

    json_object_object_add(serviceObject, "horizAccuracy", json_object_new_double(acc->horizAccuracy));
    json_object_object_add(serviceObject, "vertAccuracy", json_object_new_double(acc->vertAccuracy));
}

void location_util_add_vel_json(struct json_object* serviceObject, Velocity* velocity)
{
    if (!serviceObject || !velocity) return;

    json_object_object_add(serviceObject, "heading", json_object_new_double(velocity->direction));
    json_object_object_add(serviceObject, "velocity", json_object_new_double(velocity->speed));
}

void testjson(struct json_object* serviceObject)
{
    if (!serviceObject) return;

    json_object_object_add(serviceObject, "altitude", json_object_new_double(0));
    json_object_object_add(serviceObject, "errorCode", json_object_new_int(0));
    json_object_object_add(serviceObject, "heading", json_object_new_double(0));
    json_object_object_add(serviceObject, "horizAccuracy", json_object_new_double(54));
    json_object_object_add(serviceObject, "latitude", json_object_new_double(12.936127));
    json_object_object_add(serviceObject, "longitude", json_object_new_double(77.693872));
    json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(true));
    json_object_object_add(serviceObject, "timestamp", json_object_new_double(1365568908));
    json_object_object_add(serviceObject, "velocity", json_object_new_double(0));
    json_object_object_add(serviceObject, "vertAccuracy", json_object_new_double(0));
}
void location_util_parsejsonAddress(struct json_object* m_JsonArgument, Address *addr)
{
    bool mRetVal;
    struct json_object *m_JsonSubArgument;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "street", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->street = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "country", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->country = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "postcode", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->postcode = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "countrycode", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->countrycode = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "area", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->area = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "locality", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->locality = json_object_get_string(m_JsonSubArgument);
    }
    mRetVal = json_object_object_get_ex(m_JsonArgument, "region", &m_JsonSubArgument);
    if (mRetVal == true) {
        addr->locality = json_object_get_string(m_JsonSubArgument);
    }
}
