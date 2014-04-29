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
#include <Address.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>

void location_util_add_pos_json(struct json_object *serviceObject, Position *pos)
{
    if (!serviceObject || !pos)
        return;

    json_object_object_add(serviceObject, "timestamp", json_object_new_double(pos->timestamp));
    json_object_object_add(serviceObject, "latitude", json_object_new_double(pos->latitude));
    json_object_object_add(serviceObject, "longitude", json_object_new_double(pos->longitude));
    json_object_object_add(serviceObject, "altitude", json_object_new_double(pos->altitude));
    json_object_object_add(serviceObject, "heading", json_object_new_double(pos->direction));
    json_object_object_add(serviceObject, "velocity", json_object_new_double(pos->speed));
}

void location_util_add_errorText_json(struct json_object *serviceObject, char *errorText)
{
    if (!serviceObject)
        return;

    json_object_object_add(serviceObject, "errorText", json_object_new_string(errorText));
}

void location_util_add_returnValue_json(struct json_object *serviceObject, bool returnValue)
{
    if (!serviceObject)
        return;

    json_object_object_add(serviceObject, "returnValue", json_object_new_boolean(returnValue));
}

void location_util_add_errorCode_json(struct json_object *serviceObject, int errorCode)
{
    if (!serviceObject)
        return;

    json_object_object_add(serviceObject, "errorCode", json_object_new_int(errorCode));
}

void location_util_add_acc_json(struct json_object *serviceObject, Accuracy *acc)
{
    if (!serviceObject || !acc)
        return;

    json_object_object_add(serviceObject, "horizAccuracy", json_object_new_double(acc->horizAccuracy));
    json_object_object_add(serviceObject, "vertAccuracy", json_object_new_double(acc->vertAccuracy));
}

bool location_util_parsejsonAddress(struct json_object *m_JsonArgument, Address *addr)
{
    bool mRetVal;
    struct json_object *m_JsonSubArgument = NULL;

    if (m_JsonArgument == NULL || addr == NULL)
        return false;

    mRetVal = json_object_object_get_ex(m_JsonArgument, "street", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->street = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "country", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->country = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "postcode", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->postcode = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "countrycode", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->countrycode = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "area", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->area = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "locality", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->locality = json_object_get_string(m_JsonSubArgument);
    }

    m_JsonSubArgument = NULL;
    mRetVal = json_object_object_get_ex(m_JsonArgument, "region", &m_JsonSubArgument);

    if (m_JsonSubArgument == NULL || (mRetVal && !json_object_is_type(m_JsonSubArgument, json_type_string)))
        return false;

    if (mRetVal == true) {
        addr->region = json_object_get_string(m_JsonSubArgument);
    }

    return true;
}
