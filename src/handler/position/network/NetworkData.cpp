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

#include <NetworkData.h>

#include <lunaprefs.h>
#include <loc_security.h>
#include <Location.h>
#include <string>
#include <loc_http.h>
#include <loc_log.h>
#include <pbnjson.h>
#include <functional>
#include <map>


#define GET_NETWORKS    "luna://com.webos.service.wifi/getNetworks"
#define CELL_INFO       "luna://com.webos.service.telephony/getCellInfo"
#define SUBSCRIBE       "{\"subscribe\":true}"

using namespace std;

NetworkData::NetworkData(LSHandle *sh) : mPrvHandle(sh) {
    LS_LOG_INFO("## NetworkData:: mPrvHandle ## = %p", mPrvHandle);
}

NetworkData::~NetworkData() {
    LSError lserror;
    LSErrorInit(&lserror);

    if (mCellInfoReq) {
        if (!LSCallCancel(mPrvHandle, mCellInfoReq, &lserror)) {
            LS_LOG_ERROR("Failed to cancel getCellInfo subscription\n");
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
        mCellInfoReq = DEFAULT_VALUE;
    }

}

bool NetworkData::wifiAccessPointsCb(LSHandle *lshandle, LSMessage *msg) {
    jvalue_ref parsedObj = NULL;
    jvalue_ref parsednetworkInfoObj = NULL;
    jvalue_ref parsedbssObj = NULL;
    jvalue_ref bssIdobj = NULL;
    jvalue_ref signalObj = NULL;
    jvalue_ref foundNetworks = NULL;
    jvalue_ref bssInfoObjects = NULL;
    jvalue_ref nwInfoObj = NULL;
    jvalue_ref rangeAvailableObj = NULL;
    char *bssId = NULL;
    int signalLevel = 0;
    bool range = false;
    GHashTable *wifiAccesspoints;

    wifiAccesspoints = g_hash_table_new_full(g_str_hash, g_str_equal,
                                             (GDestroyNotify) g_free, NULL);

    jschema_ref inputSchema = jschema_parse(j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
    if (!inputSchema)
        return true;

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, inputSchema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(msg)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(parsedObj)) {
        jschema_release(&inputSchema);
        return true;
    }
    jschema_release(&inputSchema);

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("foundNetworks"), &foundNetworks)) {
        LS_LOG_INFO("network handler:networkInfo no data available\n");
        goto CLEANUP;
    }

    for (int i = 0; i < jarray_size(foundNetworks); i++) {
        parsednetworkInfoObj = jarray_get(foundNetworks, i);

        if (jobject_get_exists(parsednetworkInfoObj, J_CSTR_TO_BUF("networkInfo"), &nwInfoObj)) {

            //check range availability
            if (jobject_get_exists(nwInfoObj, J_CSTR_TO_BUF("available"), &rangeAvailableObj)) {
                jboolean_get(rangeAvailableObj, &range);
                if (!range) {
                    LS_LOG_INFO("out of range networkInfo, not considered,skip");
                    continue;
                }
            }
            else {
                LS_LOG_ERROR("network Data:no field for range data , skip");
                continue;
            }
            //parsing bssInfo array
            if (!jobject_get_exists(nwInfoObj, J_CSTR_TO_BUF("bssInfo"), &bssInfoObjects)) {
                LS_LOG_INFO("network Data:bssInfo no data available, skip");
                continue;//goto CLEANUP;
            }

            for (int j = 0; j < jarray_size(bssInfoObjects); j++) {
                parsedbssObj = jarray_get(bssInfoObjects, j);

                if (jobject_get_exists(parsedbssObj, J_CSTR_TO_BUF("bssid"), &bssIdobj)) {
                    raw_buffer buf = jstring_get(bssIdobj);
                    bssId = strdup(buf.m_str);
                    jstring_free_buffer(buf);
                }

                if (jobject_get_exists(parsedbssObj, J_CSTR_TO_BUF("signal"), &signalObj)) {
                    jnumber_get_i32(signalObj, &signalLevel);
                }

                LS_LOG_INFO("network handler:bssInfo %s and signalLevel %d gathered \n", bssId, signalLevel);
                g_hash_table_insert(wifiAccesspoints, bssId, GINT_TO_POINTER(signalLevel));
            }
        }
        else {
            LS_LOG_ERROR("!!no NetworkInfo object found in json data");
        }
    }

    mClient->onUpdateWifiData(wifiAccesspoints, msg);

    CLEANUP:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return true;
}

bool NetworkData::registerForWifiAccessPoints() {
    LSError lserror;
    LSErrorInit(&lserror);
    gboolean result;

    result = LSCall(mPrvHandle,
                    GET_NETWORKS,
                    SUBSCRIBE,
                    &NetworkData::wifiAccessPointsCb,
                    this, &mWifiInfoReq, &lserror);
    if (!result) {
        LS_LOG_ERROR("Failed to register findnetworks subscription");
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return FALSE;
    }

    return TRUE;
}

bool NetworkData::registerForCellInfo() {

    LSError lserror;
    LSErrorInit(&lserror);
    gboolean result;

    result = LSCall(mPrvHandle, CELL_INFO, SUBSCRIBE, &NetworkData::cellDataCb, this, &mCellInfoReq, &lserror);

    if (!result) {
        LS_LOG_ERROR("Failed to register getCellInfo subscription");
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return result;
}

bool NetworkData::unregisterForCellInfo() {
    LSError lserror;
    LSErrorInit(&lserror);

    if (mCellInfoReq) {
        if (!LSCallCancel(mPrvHandle, mCellInfoReq, &lserror)) {
            LS_LOG_ERROR("Failed to cancel getCellInfo subscription");
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            return FALSE;
        }
    }

    return TRUE;
}

bool NetworkData::unregisterForWifiAccessPoints() {

    LSError lserror;
    LSErrorInit(&lserror);

    if (mWifiInfoReq) {
        if (!LSCallCancel(mPrvHandle, mWifiInfoReq, &lserror)) {
            LS_LOG_ERROR("Failed to cancel findnetworks subscription");
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            return FALSE;
        }
    }

    return TRUE;

}

void NetworkData::setNetworkDataClient(NetworkDataClient *client) {

    LS_LOG_INFO("setNetworkDataClient registered");
    mClient = client;
}

bool NetworkData::cellDataCb(LSHandle *sh, LSMessage *message) {

    jvalue_ref parsedObj = NULL;
    jvalue_ref errorObj = NULL;
    int error = ERROR_NONE;
    jvalue_ref cellDataObj = NULL;

    LS_LOG_DEBUG("message=%s \n \n", LSMessageGetPayload(message));

    jschema_ref inputSchema = jschema_parse(j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);

    if (!inputSchema) {
        LS_LOG_ERROR("schema invalid");
        return true;
    }

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, inputSchema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jis_null(parsedObj)) {
        LS_LOG_INFO("parsing failed");
        jschema_release(&inputSchema);
        return true;
    }

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("errorCode"), &errorObj)) {
        LS_LOG_INFO("errorCode doesnt exist");
        goto CLEANUP;
    }

    jnumber_get_i32(errorObj, &error);

    if (error != ERROR_NONE) { // O i Sucess case
        LS_LOG_ERROR("network handler:Telephony error Happened %d\n", error);
        goto CLEANUP;
    }

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("data"), &cellDataObj)) {
        LS_LOG_ERROR("network handler:Telephony no data available\n");
        goto CLEANUP;
    }

    mClient->onUpdateCellData(g_strdup(jvalue_tostring_simple(cellDataObj)), message);

    CLEANUP:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    jschema_release(&inputSchema);

    return true;
}

bool NetworkData::cellDataCb(LSHandle *sh, LSMessage *message, void *ctx) {

    return ((NetworkData *) ctx)->cellDataCb(sh, message);
}

bool NetworkData::wifiAccessPointsCb(LSHandle *sh, LSMessage *message, void *ctx) {

    return ((NetworkData *) ctx)->wifiAccessPointsCb(sh, message);
}
