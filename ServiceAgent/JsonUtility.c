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
#include <Position.h>
#include <Address.h>
#include <glib.h>
#include <glib-object.h>
#include <geoclue/geoclue-position.h>
#include "LunaLocationServiceUtil.h"
#include "ServiceAgent.h"
#include "LocationService_Log.h"
#include "JsonUtility.h"

void location_util_add_pos_json(jvalue_ref serviceObject, Position *pos)
{
    if (jis_null(serviceObject) || (pos == NULL))
        return;

    jobject_put(serviceObject, J_CSTR_TO_JVAL("timestamp"), jnumber_create_i64(pos->timestamp));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("latitude"), jnumber_create_f64(pos->latitude));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("longitude"), jnumber_create_f64(pos->longitude));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("altitude"), jnumber_create_f64(pos->altitude));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("direction"), jnumber_create_f64(pos->direction));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("speed"), jnumber_create_f64(pos->speed));
}

void location_util_add_errorText_json(jvalue_ref serviceObject, char *errorText)
{
    if (jis_null(serviceObject) )
        return;

    jobject_put(serviceObject, J_CSTR_TO_JVAL("errorText"), jstring_create(errorText));
}

void location_util_form_json_reply(jvalue_ref serviceObject, bool returnValue, int errorCode)
{
    if (jis_null(serviceObject) )
        return;

    jobject_put(serviceObject, J_CSTR_TO_JVAL("returnValue"), jboolean_create(returnValue));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(errorCode));
}

void location_util_add_acc_json(jvalue_ref serviceObject, Accuracy *acc)
{
    if (jis_null(serviceObject) || (acc == NULL))
        return;

    jobject_put(serviceObject, J_CSTR_TO_JVAL("horizAccuracy"), jnumber_create_f64(acc->horizAccuracy));
    jobject_put(serviceObject, J_CSTR_TO_JVAL("vertAccuracy"), jnumber_create_f64(acc->vertAccuracy));
}

bool location_util_parsejsonAddress(jvalue_ref serviceObject, Address *addr)
{
    bool mRetVal;
    jvalue_ref m_JsonSubArgument = NULL;
    raw_buffer nameBuf;

    if (jis_null(serviceObject)  || addr == NULL) {
        return false;
    }

    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("street"), &m_JsonSubArgument);

    if (mRetVal == true) {
        raw_buffer nameBuf = jstring_get(m_JsonSubArgument);
        addr->street = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;

    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("country"), &m_JsonSubArgument);


    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->country = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;
    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("postcode"), &m_JsonSubArgument);

    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->postcode = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;
    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF( "countrycode"), &m_JsonSubArgument);

    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->countrycode = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;
    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("area"), &m_JsonSubArgument);


    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->area = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;
    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("locality"), &m_JsonSubArgument);

    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->locality = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    m_JsonSubArgument = NULL;
    mRetVal = jobject_get_exists(serviceObject, J_CSTR_TO_BUF("region"), &m_JsonSubArgument);

    if (mRetVal == true) {
        nameBuf = jstring_get(m_JsonSubArgument);
        addr->region = g_strdup(nameBuf.m_str);
        jstring_free_buffer(nameBuf);
    }

    return true;
}

