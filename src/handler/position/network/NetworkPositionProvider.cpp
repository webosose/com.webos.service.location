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

#include <loc-utils/loc_security.h>
#include "NetworkPositionProvider.h"
#include "Gps_stored_data.h"

#define NETWORK_URL                            "https://www.googleapis.com/geolocation/v1/geolocate?key=%s"
#define SIGNAL_CHANGE_THRESHOLD                20
#define SCHEMA_ANY                             "{}"
#define URL_LENGTH                             256
#define WIFI_SERVICE                           "com.webos.service.wifi"
#define TELEPHONY_SERVICE                      "com.webos.service.telephony"
#define GEOLOCKEY_CONFIG_PATH                  "/etc/geolocation.conf"
#define SCAN_METHOD                            "luna://com.webos.service.wifi/scan"
#define SCAN_PAYLOAD                           "{}"

NetworkPositionProvider::NetworkPositionProvider(LSHandle *sh) : PositionProviderInterface("Network"),
                                                                 mNetworkData(sh) {
    mwifiStatus = false;
    mtelephonyPowerd = false;
    mEnabled = false;
    mLSHandle = sh;
    mProcessRequestInProgress = false;
    mNetworkData.setNetworkDataClient(this);
    mTelephonyCookie = nullptr;
    mWifiCookie = nullptr;
    mConnectivityStatus = false;
    mTimeoutId = 0;
}

char *NetworkPositionProvider::readApiKey() {
    unsigned char *geoLocAPIKey = NULL;
    int retGeoLocAPIKey = LOC_SECURITY_ERROR_FAILURE;

    retGeoLocAPIKey = locSecurityBase64Decode(GEOLOCKEY_CONFIG_PATH,
                                              &geoLocAPIKey);

    if (LOC_SECURITY_ERROR_SUCCESS == retGeoLocAPIKey) {
        return (char *) geoLocAPIKey;
    }

    LS_LOG_INFO("readApiKey failed");
    return (char *) geoLocAPIKey;
}

void NetworkPositionProvider::enable() {
    LS_LOG_INFO("#NetworkPositionProvider::enable");

    if (mEnabled) {
        LS_LOG_INFO("#NetworkPositionProvider::already enabled");
        return;
    }

    mPositionData.nwGeolocationKey = readApiKey();

    mEnabled = true;
}

void NetworkPositionProvider::disable() {
    LS_LOG_INFO("#NetworkPositionProvider::disable ");

    if (mEnabled) {
        g_source_remove(mTimeoutId);
        setCallback(nullptr);
        mEnabled = false;
        mTimeoutId = 0;
    } else {
        LS_LOG_INFO("#NetworkPositionProvider::already disabled ");
    }

}

void NetworkPositionProvider::onUpdateCellData(char *cellData, LSMessage *message) {
    LS_LOG_INFO("############NetworkPositionProvider::onUpdateCellData = %s", cellData);

    if (mPositionData.cellInfo) {
        g_free(mPositionData.cellInfo);
        mPositionData.cellInfo = NULL;
    }

    mPositionData.cellInfo = g_strdup(cellData);
    triggerPostQuery(message);
}

void NetworkPositionProvider::onUpdateWifiData(GHashTable *wifiAccessPoints, LSMessage *message) {
    GHashTableIter iter;
    gpointer key = NULL;
    gpointer newValue = NULL;
    gchar *updateKey = NULL;
    int updateValue = 0;

    if (mPositionData.wifiAccessPointsList == NULL)
        mPositionData.wifiAccessPointsList = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                                   (GDestroyNotify) g_free, NULL);
    else
        g_hash_table_remove_all(mPositionData.wifiAccessPointsList);

    if (wifiAccessPoints != NULL) {
        g_hash_table_iter_init(&iter, wifiAccessPoints);

        while (g_hash_table_iter_next(&iter, &key, &newValue)) {
            updateKey = g_strdup((const gchar *) key);
            updateValue = GPOINTER_TO_INT(newValue);
            g_hash_table_insert(mPositionData.wifiAccessPointsList, updateKey, GINT_TO_POINTER(updateValue));
            LS_LOG_INFO("###### onUpdateWifiData New key/value: %s, %d", (char *) updateKey,
                        GPOINTER_TO_INT(updateValue));
        }

    }

    triggerPostQuery(message);

}

bool NetworkPositionProvider::triggerPostQuery(LSMessage *message) {
    char *postData = NULL;

    if (!mwifiStatus && mtelephonyPowerd) {
        LS_LOG_DEBUG("createCellQuery");
        postData = createCellQuery();
    }

    if (!mtelephonyPowerd && mwifiStatus) {
        LS_LOG_DEBUG("createWifiQuery");
        postData = createWifiQuery();
    }

    if (mwifiStatus && mtelephonyPowerd) {
        LS_LOG_DEBUG("createCellWifiCombinedQuery");
        postData = createCellWifiCombinedQuery();
    }

    // for tracking, no update means no need to emit signal
    if (!postData) {
        LS_LOG_ERROR("triggerPostQuery: no data to post");
        return false;
    }

    if (!networkPostQuery(postData, mPositionData.nwGeolocationKey, FALSE, message)) {
        LS_LOG_ERROR("Failed to post query!");
        g_free(postData);
        return false;
    }

    g_free(postData);

    return true;
}

ErrorCodes NetworkPositionProvider::getLastPosition(Position *position, Accuracy *accuracy) {
    if (get_stored_position(position, accuracy, const_cast<char *>(LOCATION_DB_PREF_PATH_NETWORK)) ==
        ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("getLastPosition Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

gboolean NetworkPositionProvider::networkUpdateCallback(void *obj) {

    NetworkPositionProvider *pThis = static_cast <NetworkPositionProvider*> (obj);

    if (!pThis) {
        LS_LOG_ERROR("networkUpdateCallback pThis is NULL");
        return true;
    }

    pThis->lunaServiceCall(SCAN_METHOD, SCAN_PAYLOAD, scanCb, &(pThis->mScanToken), true);

    return true;
}

ErrorCodes  NetworkPositionProvider::processRequest(PositionRequest request) {
    LS_LOG_INFO("processRequest");

    if (!mEnabled) {
        LS_LOG_INFO("processRequest::NetworkPositionProvider is not enabled!!!");
        return ERROR_NOT_STARTED;
    }

    switch (request.getRequestType()) {
        case POSITION_CMD:

            LS_LOG_INFO("processRequest:POSITION_CMD received");

            if (mProcessRequestInProgress) {
            if (getCallback())
                    getCallback()->getLocationUpdateCb(GeoLocation(mPositionData.lastLatitude, mPositionData.lastLongitude, -1.0,
                                          mPositionData.lastAccuracy, mPositionData.lastTimeStamp, -1.0, -1.0, -1.0,
                                          -1.0),
                              ERROR_NONE, HANDLER_NETWORK);
                break;
            }

            if (NULL == mPositionData.nwGeolocationKey) {
                LS_LOG_INFO("geolocation API key is invalid, re-trying to read ...");
                /*Read again*/
                mPositionData.nwGeolocationKey = readApiKey();

                if (!mPositionData.nwGeolocationKey) {
                    LS_LOG_ERROR("Failed to get API keys");
                    return ERROR_LICENSE_KEY_INVALID;
                }
            }

            lunaServiceCall(SCAN_METHOD, SCAN_PAYLOAD, scanCb, &mScanToken, true);
            mProcessRequestInProgress = true;
            registerServiceStatus(TELEPHONY_SERVICE, &mTelephonyCookie, serviceStatusCb);
            registerServiceStatus(WIFI_SERVICE, &mWifiCookie, serviceStatusCb);
            mTimeoutId = g_timeout_add(60000, &networkUpdateCallback, (gpointer) this);
            break;

        case STOP_POSITION_CMD:

            LS_LOG_INFO("processRequest:STOP_POSITION_CMD received");
            mProcessRequestInProgress = false;
            mNetworkData.unregisterForWifiAccessPoints();
            mNetworkData.unregisterForCellInfo();
            unregisterServiceStatus(mWifiCookie);
            unregisterServiceStatus(mTelephonyCookie);

            if (mTimeoutId) {
                LS_LOG_INFO("stopping scan timer");
                g_source_remove(mTimeoutId);
                mTimeoutId = 0;
            }

            mPositionData.resetData();
            break;

        default:
            LS_LOG_ERROR("unknown PositionRequest received");
    }

    return ERROR_NONE;
}

char *NetworkPositionProvider::updateCellData() {
    jvalue_ref parsedObj = NULL;
    jvalue_ref cellObj = NULL;
    jvalue_ref cellTower = NULL;
    jvalue_ref subObj = NULL;
    bool state = FALSE;
    int32_t cellID = 0;
    char *cellData = NULL;

    if (mPositionData.cellInfo) {
        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
        parsedObj = jdom_parse(j_cstr_to_buffer(mPositionData.cellInfo), DOMOPT_NOOPT, &schemaInfo);

        if (jis_null(parsedObj)) {
            LS_LOG_INFO("cellInfo parse failed");
            return NULL;
        }

        //CheckIf cellID changed, query should be posted only if cellID changed
        if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("cellTowers"), &cellObj)) {
            if (!jis_array(cellObj))
                goto EXIT;

            for (int i = 0; i < jarray_size(cellObj); i++) {
                cellTower = jarray_get(cellObj, i);
                if (jobject_get_exists(cellTower, J_CSTR_TO_BUF("registered"), &subObj)) {
                    jboolean_get(subObj, &state);

                    if (state && jobject_get_exists(cellTower, J_CSTR_TO_BUF("cellId"), &subObj)) {
                        jnumber_get_i32(subObj, &cellID);

                        if (cellID != mPositionData.curCellid) {
                            mPositionData.curCellid = cellID;
                            mPositionData.cellChanged = TRUE;
                        } else {
                            mPositionData.cellChanged = FALSE;
                        }
                    }
                }

                if (state) //exit if registered cell tower found
                    break;
            }
        }
    }
    //if cell changed and new cell info is available
    if ((mPositionData.cellInfo) && mPositionData.cellChanged) {
        cellData = g_strdup(jvalue_tostring_simple(cellObj));
        LS_LOG_DEBUG("cellData %s", cellData);
    }

    EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return cellData;

}

char *NetworkPositionProvider::createCellWifiCombinedQuery() {
    gchar *updateKey = NULL;
    char *postData = NULL;
    gboolean isFirst = FALSE;
    int vanishedCount = 0;
    int signalChange = 0;
    guint oldSize = 0;
    guint newSize = 0;
    int signalChangeSum = 0;
    int updateValue = 0;
    double avgSignalChange = 0;
    GHashTableIter iter;
    gpointer key = NULL;
    gpointer oldValue = NULL;
    gpointer newValue = NULL;
    char *cellData = NULL;
    jvalue_ref wifiCellIdObject = NULL;
    jvalue_ref wifiAccessPointsList = NULL;
    jvalue_ref wifiAccessPointsListItem = NULL;
    jvalue_ref parsedObj = NULL;
    jvalue_ref cellObj = NULL;

    wifiCellIdObject = jobject_create();

    if (jis_null(wifiCellIdObject)) {
        return NULL;
    }

    LS_LOG_INFO("NetworkPositionProvider::createCellWifiCombinedQuery ");

    if (mPositionData.cellInfo != NULL) {
        cellData = updateCellData();

        if (cellData) {

            JSchemaInfo schemaInfo;
            jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
            parsedObj = jdom_parse(j_cstr_to_buffer(mPositionData.cellInfo), DOMOPT_NOOPT, &schemaInfo);

            if (!jis_null(parsedObj)) {
                LS_LOG_INFO("cellInfo parsed");
                if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("cellTowers"), &cellObj))
                    jobject_put(wifiCellIdObject, J_CSTR_TO_JVAL("cellTowers"), cellObj);
                LS_LOG_INFO("==%s====", jvalue_tostring_simple(wifiCellIdObject));
            }

            if (!mPositionData.wifiAccessPointsList || (g_hash_table_size(mPositionData.wifiAccessPointsList) < 2)) {
                postData = g_strdup(jvalue_tostring_simple(wifiCellIdObject));
                LS_LOG_DEBUG("WiFi AP <2, updated with only cell data %s", postData);
                goto EXIT;
            }

        }
    }

    LS_LOG_INFO("Checking wifi AP list...");

    if (!mPositionData.wifiAccessPointsList || (g_hash_table_size(mPositionData.wifiAccessPointsList) < 2)) {
        LS_LOG_DEBUG("wifiAccessPointsList is less than 2");
        goto EXIT;
    }

    if (!mPositionData.lastAps) {
        LS_LOG_INFO("Checking mPositionData.lastAps null...");
        isFirst = TRUE;
        mPositionData.lastAps = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                      (GDestroyNotify) g_free, NULL);
    }

    LS_LOG_INFO("Checking mPositionData.last_aps not null...");
    oldSize = g_hash_table_size(mPositionData.lastAps);
    LS_LOG_INFO("Existing table size = %d", oldSize);

    newSize = g_hash_table_size(mPositionData.wifiAccessPointsList);

    LS_LOG_INFO("New table size = %d", newSize);

    if (!isFirst) {
        g_hash_table_iter_init(&iter, mPositionData.wifiAccessPointsList);

        while (g_hash_table_iter_next(&iter, &key, &newValue)) {
            LS_LOG_INFO("New key/value: %s, %d", (char *) key, GPOINTER_TO_INT(newValue));
            if ((oldValue = g_hash_table_lookup(mPositionData.lastAps, (gconstpointer) key))) {
                LS_LOG_INFO("Found key in existing table");

                signalChange = abs(GPOINTER_TO_INT(newValue) - GPOINTER_TO_INT(oldValue));
            } else {
                LS_LOG_INFO("Not found key");

                signalChange = abs(GPOINTER_TO_INT(newValue));
            }

            LS_LOG_INFO("signal change of %s = %d", (char *) key, signalChange);
            signalChangeSum += signalChange;
        }

        g_hash_table_iter_init(&iter, mPositionData.lastAps);

        while (g_hash_table_iter_next(&iter, &key, &oldValue)) {
            if (!(newValue = g_hash_table_lookup(mPositionData.wifiAccessPointsList, (gconstpointer) key))) {
                LS_LOG_INFO("Existing key is vanished in new table");

                signalChange = abs(GPOINTER_TO_INT(oldValue));
                LS_LOG_INFO("signal change of %s = %d", (char *) key, signalChange);

                signalChangeSum += signalChange;
                vanishedCount++;
            }
        }

        avgSignalChange = (double) signalChangeSum / (double) (newSize + vanishedCount);
        LS_LOG_INFO("Average signal change = %f", avgSignalChange);
    }

    if (isFirst || (!isFirst && avgSignalChange > SIGNAL_CHANGE_THRESHOLD)) {
        g_hash_table_remove_all(mPositionData.lastAps);

        wifiAccessPointsList = jarray_create(NULL);
        if (jis_null(wifiAccessPointsList))
            goto EXIT;

        if (jis_null(wifiCellIdObject)) {
            j_release(&wifiCellIdObject);
            goto EXIT;
        }

        g_hash_table_iter_init(&iter, mPositionData.wifiAccessPointsList);
        while (g_hash_table_iter_next(&iter, &key, &newValue)) {
            updateKey = g_strdup((const gchar *) key);
            updateValue = GPOINTER_TO_INT(newValue);

            LS_LOG_INFO("inserting key/value: %s, %d", updateKey, updateValue);
            g_hash_table_insert(mPositionData.lastAps, updateKey, GINT_TO_POINTER(updateValue));

            wifiAccessPointsListItem = jobject_create();
            if (!jis_null(wifiAccessPointsListItem)) {
                jobject_put(wifiAccessPointsListItem, J_CSTR_TO_JVAL("macAddress"), jstring_create((char *) key));
                jobject_put(wifiAccessPointsListItem, J_CSTR_TO_JVAL("signalStrength"),
                            jnumber_create_i32(GPOINTER_TO_INT(newValue)));
                jarray_append(wifiAccessPointsList, wifiAccessPointsListItem);
            }
        }

        jobject_put(wifiCellIdObject, J_CSTR_TO_JVAL("wifiAccessPoints"), wifiAccessPointsList);
    } else {
        LS_LOG_INFO("No update in wifi AP list");
        if (!(mPositionData.cellInfo && mPositionData.cellChanged)) {
            goto EXIT;
        }
    }

    postData = g_strdup(jvalue_tostring_simple(wifiCellIdObject));
    LS_LOG_INFO("data posted for query %s", postData);

    EXIT:
    if (!jis_null(wifiCellIdObject))
        j_release(&wifiCellIdObject);

    if (cellData)
        g_free(cellData);

    return postData;
}

char *NetworkPositionProvider::createCellQuery() {
    char *postData = NULL;
    char *cellData = NULL;
    jvalue_ref cellIdObject = NULL;
    jvalue_ref parsedObj = NULL;
    jvalue_ref cellObj = NULL;

    cellIdObject = jobject_create();

    if (jis_null(cellIdObject)) {
        return NULL;
    }

    LS_LOG_INFO("NetworkPositionProvider::createCellQuery ");

    if (mPositionData.cellInfo != NULL) {
        cellData = updateCellData();

        if (cellData) {

            JSchemaInfo schemaInfo;
            jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
            parsedObj = jdom_parse(j_cstr_to_buffer(mPositionData.cellInfo), DOMOPT_NOOPT, &schemaInfo);

            if (!jis_null(parsedObj)) {
                LS_LOG_INFO("cellInfo parsed");

                if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("cellTowers"), &cellObj))
                    jobject_put(cellIdObject, J_CSTR_TO_JVAL("cellTowers"), cellObj);
                LS_LOG_INFO("cellIdObject ==%s====", jvalue_tostring_simple(cellIdObject));
            }

            postData = g_strdup(jvalue_tostring_simple(cellIdObject));
            LS_LOG_DEBUG("list updated with cell data only %s", postData);
            goto EXIT;
        }

    } else {
        LS_LOG_INFO("NetworkPositionProvider::createCellQuery no cell info");
    }

    EXIT:
    if (!jis_null(cellIdObject))
        j_release(&cellIdObject);

    if (cellData)
        g_free(cellData);

    return postData;
}

char *NetworkPositionProvider::createWifiQuery() {
    gchar *updateKey = NULL;
    char *postData = NULL;
    gboolean isFirst = FALSE;
    int vanishedCount = 0;
    int signalChange = 0;
    guint oldSize = 0;
    guint newSize = 0;
    int signalChangeSum = 0;
    int updateValue = 0;
    double avgSignalChange = 0;
    GHashTableIter iter;
    gpointer key = NULL;
    gpointer oldValue = NULL;
    gpointer newValue = NULL;
    jvalue_ref wifiObject = NULL;
    jvalue_ref wifiAccessPointsList = NULL;
    jvalue_ref wifiAccessPointsListItem = NULL;

    wifiObject = jobject_create();

    if (jis_null(wifiObject)) {
        return NULL;
    }

    LS_LOG_INFO("NetworkPositionProvider::createWifiQuery ");

    if (!mPositionData.wifiAccessPointsList || (g_hash_table_size(mPositionData.wifiAccessPointsList) < 2)) {
        LS_LOG_DEBUG("Access point empty or less than 2");
        goto EXIT;
    }

    LS_LOG_INFO("Checking wifi AP list...");

    if (!mPositionData.lastAps) {
        LS_LOG_INFO("Checking mPositionData.lastAps null...");
        isFirst = TRUE;
        mPositionData.lastAps = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                      (GDestroyNotify) g_free, NULL);
    }
    LS_LOG_INFO("Checking mPositionData.last aps not null...");
    oldSize = g_hash_table_size(mPositionData.lastAps);
    LS_LOG_INFO("Existing table size = %d", oldSize);

    newSize = g_hash_table_size(mPositionData.wifiAccessPointsList);

    LS_LOG_INFO("New table size = %d", newSize);

    if (!isFirst) {
        g_hash_table_iter_init(&iter, mPositionData.wifiAccessPointsList);

        while (g_hash_table_iter_next(&iter, &key, &newValue)) {
            LS_LOG_INFO("New key/value: %s, %d", (char *) key, GPOINTER_TO_INT(newValue));

            if ((oldValue = g_hash_table_lookup(mPositionData.lastAps, (gconstpointer) key))) {
                LS_LOG_INFO("Found key in existing table");
                signalChange = abs(GPOINTER_TO_INT(newValue) - GPOINTER_TO_INT(oldValue));
            } else {
                LS_LOG_INFO("Not found key");
                signalChange = abs(GPOINTER_TO_INT(newValue));
            }

            LS_LOG_INFO("signal change of %s = %d", (char *) key, signalChange);
            signalChangeSum += signalChange;
        }

        g_hash_table_iter_init(&iter, mPositionData.lastAps);

        while (g_hash_table_iter_next(&iter, &key, &oldValue)) {
            if (!(newValue = g_hash_table_lookup(mPositionData.wifiAccessPointsList, (gconstpointer) key))) {
                LS_LOG_INFO("Existing key is vanished in new table");

                signalChange = abs(GPOINTER_TO_INT(oldValue));
                LS_LOG_INFO("signal change of %s = %d", (char *) key, signalChange);

                signalChangeSum += signalChange;
                vanishedCount++;
            }
        }

        avgSignalChange = (double) signalChangeSum / (double) (newSize + vanishedCount);
        LS_LOG_INFO("Average signal change = %f", avgSignalChange);
    }

    if (isFirst || (!isFirst && avgSignalChange > SIGNAL_CHANGE_THRESHOLD)) {
        g_hash_table_remove_all(mPositionData.lastAps);

        wifiAccessPointsList = jarray_create(NULL);
        if (jis_null(wifiAccessPointsList))
            goto EXIT;

        if (jis_null(wifiObject)) {
            j_release(&wifiObject);
            goto EXIT;
        }

        g_hash_table_iter_init(&iter, mPositionData.wifiAccessPointsList);
        while (g_hash_table_iter_next(&iter, &key, &newValue)) {
            updateKey = g_strdup((const gchar *) key);
            updateValue = GPOINTER_TO_INT(newValue);

            LS_LOG_INFO("inserting key/value: %s, %d", updateKey, updateValue);
            g_hash_table_insert(mPositionData.lastAps, updateKey, GINT_TO_POINTER(updateValue));

            wifiAccessPointsListItem = jobject_create();
            if (!jis_null(wifiAccessPointsListItem)) {
                jobject_put(wifiAccessPointsListItem, J_CSTR_TO_JVAL("macAddress"), jstring_create((char *) key));
                jobject_put(wifiAccessPointsListItem, J_CSTR_TO_JVAL("signalStrength"),
                            jnumber_create_i32(GPOINTER_TO_INT(newValue)));
                jarray_append(wifiAccessPointsList, wifiAccessPointsListItem);
            }
        }

        jobject_put(wifiObject, J_CSTR_TO_JVAL("wifiAccessPoints"), wifiAccessPointsList);
    } else {
        LS_LOG_INFO("No update in wifi AP list");
        goto EXIT;
    }

    postData = g_strdup(jvalue_tostring_simple(wifiObject));
    LS_LOG_INFO("data posted for query %s", postData);

    EXIT:
    if (!jis_null(wifiObject))
        j_release(&wifiObject);

    return postData;
}

bool NetworkPositionProvider::networkPostQuery(char *postData, const char *APIKey, gboolean sync, LSMessage *message) {
    char url[URL_LENGTH] = {0};
    sprintf(url, NETWORK_URL, APIKey);
    LS_LOG_DEBUG("networkPostQuery %s", url);

    int errorCode = NetworkRequestManager::getInstance()->initiateTransaction(NULL, 0, url, sync, message, this,
                                                                              postData);

    if (ERROR_NONE == errorCode) {
        return true;
    }

    return false;
}

void NetworkPositionProvider::handleResponse(HttpReqTask *task) {
    int response = 0;
    ErrorCodes error = ERROR_NONE;
    int64_t currentTime = 0;
    double latitude, longitude, accuracy;
    struct timeval tv;

    if (!task) {
        LS_LOG_ERROR("handleResponse::invalid http data received");
        return;
    }

    if (HTTP_STATUS_CODE_SUCCESS == task->curlDesc.httpResponseCode) {
        LS_LOG_INFO("curlResultCode: %d, httpResponseCode: %ld, httpConnectCode: %ld, desc: %s",
                    task->curlDesc.curlResultCode,
                    task->curlDesc.httpResponseCode,
                    task->curlDesc.httpConnectCode,
                    (task->curlDesc.curlResultErrorStr) ? task->curlDesc.curlResultErrorStr : "");

        LS_LOG_DEBUG("responseData %s", task->responseData);

        latitude = longitude = accuracy = 0;
        response = parseHTTPResponse(task->responseData,
                                     &latitude,
                                     &longitude,
                                     &accuracy);

        NetworkRequestManager::getInstance()->clearTransaction(task);

        if (response != ERROR_NONE) {
            LS_LOG_ERROR("parseHTTPResponse failed");
            return;
        }

        LS_LOG_DEBUG("parseHTTPResponse succeeded");

        // for tracking, if responded coordinates are same as the last coordinates,
        // no need to emit signal.
        LS_LOG_INFO("latitude/longitude change: %f, %f", fabs(mPositionData.lastLatitude - latitude),
                    fabs(mPositionData.lastLongitude - longitude));

        if (latitude == mPositionData.lastLatitude &&
            longitude == mPositionData.lastLongitude &&
            accuracy == mPositionData.lastAccuracy) {

            LS_LOG_DEBUG("no position change in tracking");
            return;
        }

        LS_LOG_INFO("position is changed, emitting position changed signal...");

        // position is changed
        mPositionData.lastLatitude = latitude;
        mPositionData.lastLongitude = longitude;
        mPositionData.lastAccuracy = accuracy;
        gettimeofday(&tv, (struct timezone *) NULL);
        currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

        set_store_position(currentTime, latitude, longitude, INVALID_PARAM, INVALID_PARAM, INVALID_PARAM,
                           accuracy, INVALID_PARAM, const_cast<char *>(LOCATION_DB_PREF_PATH_NETWORK));

        if (getCallback())
            getCallback()->getLocationUpdateCb(GeoLocation(latitude, longitude, -1.0, accuracy, currentTime, -1.0, -1.0, -1.0, -1.0), error,
                      HANDLER_NETWORK);
    } else {
        //error case handling
        LS_LOG_INFO("network error: task->curlDesc.curlResultErrorStr %s", task->curlDesc.curlResultErrorStr);

        NetworkRequestManager::getInstance()->clearTransaction(task);

        error = ERROR_NETWORK_ERROR;

        gettimeofday(&tv, (struct timezone *) NULL);
        currentTime = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

        if (getCallback())
            getCallback()->getLocationUpdateCb(GeoLocation(0, 0, -1.0, 0, currentTime, -1.0, -1.0, -1.0, -1.0), error, HANDLER_NETWORK);
    }
}

int NetworkPositionProvider::parseHTTPResponse(char *body, double *latitude, double *longitude, double *accuracy) {
    jschema_ref inputSchema = NULL;
    jvalue_ref parsedObj = NULL;
    jvalue_ref locObj = NULL;
    jvalue_ref errorObj = NULL;
    JSchemaInfo schemaInfo;
    int error = ERROR_NO_DATA;

    if (!body)
        return error;

    inputSchema = jschema_parse(j_cstr_to_buffer(SCHEMA_ANY), DOMOPT_NOOPT, NULL);

    if (!inputSchema)
        goto EXIT;

    jschema_info_init(&schemaInfo, inputSchema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(body), DOMOPT_NOOPT, &schemaInfo);
    jschema_release(&inputSchema);

    if (jis_null(parsedObj))
        goto EXIT;

    if (jobject_get_exists(parsedObj, J_CSTR_TO_BUF("error"), &errorObj)) {
        jnumber_get_i32(jobject_get(errorObj, J_CSTR_TO_BUF("code")), &error);
        LS_LOG_ERROR("Error %d:", error);
        goto EXIT;
    }

    jnumber_get_f64(jobject_get(parsedObj, J_CSTR_TO_BUF("accuracy")), accuracy);

    locObj = jobject_get(parsedObj, J_CSTR_TO_BUF("location"));
    jnumber_get_f64(jobject_get(locObj, J_CSTR_TO_BUF("lat")), latitude);
    jnumber_get_f64(jobject_get(locObj, J_CSTR_TO_BUF("lng")), longitude);
    error = ERROR_NONE;

    LS_LOG_DEBUG("parsed accuracy=%f, latitude=%f, longitude=%f", *accuracy, *latitude, *longitude);

    EXIT:
    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    return error;
}

bool NetworkPositionProvider::registerServiceStatus(const char *service, void **cookie, LSServerStatusFunc cb) {
    LSError lserror;
    LSErrorInit(&lserror);

    bool result = LSRegisterServerStatusEx(mLSHandle,
                                           service,
                                           cb,
                                           this,
                                           cookie,
                                           &lserror);
    if (!result) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    if (result)
        LS_LOG_INFO("Registered to service %s with cookie value %p", service, cookie);

    return result;
}

bool NetworkPositionProvider::unregisterServiceStatus(void *cookie) {
    LSError lserror;
    LSErrorInit(&lserror);

    if (!cookie) {
        LS_LOG_INFO("unregisterServiceStatus: invalid cookie received %p", cookie);
        return false;
    }
    bool result = LSCancelServerStatus(mLSHandle, cookie, &lserror);

    if (!result) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    if (result)
        LS_LOG_INFO("Unregistered successfully for cookie %p", cookie);

    return result;
}

bool NetworkPositionProvider::serviceStatusCb(LSHandle *sh,
                                              const char *serviceName,
                                              bool connected,
                                              void *ctx) {
    if ((!serviceName)||(!ctx))
        return false;

    LS_LOG_INFO("service %s %d ", serviceName, connected);

    if (!strcmp(serviceName, WIFI_SERVICE))
        ((NetworkPositionProvider *) ctx)->wifiServiceStatusUpdate(connected);
    else if (!strcmp(serviceName, TELEPHONY_SERVICE))
        ((NetworkPositionProvider *) ctx)->telephonyServiceStatusUpdate(connected);

    return true;
}

void NetworkPositionProvider::wifiServiceStatusUpdate(bool connected) {
    if (connected && mProcessRequestInProgress) {
        mNetworkData.registerForWifiAccessPoints();
    }
}

void NetworkPositionProvider::telephonyServiceStatusUpdate(bool connected) {
    if (connected && mProcessRequestInProgress) {
        mNetworkData.registerForCellInfo();
    }
}

bool NetworkPositionProvider::lunaServiceCall(const char *uri, const char *payload, LSFilterFunc cb,
                                              LSMessageToken *token, bool oneReply) {

    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    if (uri && payload)
        LS_LOG_INFO("LSCall %s paylod %s", uri, payload);

    if (oneReply)
        result = LSCallOneReply(mLSHandle, uri, payload, cb, this, token, &lserror);
    else
        result = LSCall(mLSHandle, uri, payload, cb, this, token, &lserror);

    if (!result) {
        LS_LOG_ERROR("LSCall failed");
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return result;
}

bool NetworkPositionProvider::scanCb(LSHandle *sh, LSMessage *reply, void *ctx) {
    LS_LOG_DEBUG("scanCb reply %s", LSMessageGetPayload(reply));
    return true;
}


void NetworkPositionProvider::Handle_WifiNotification(bool status) {
    LS_LOG_INFO("Handle_WifiNotification status %d", status);
    mwifiStatus = status;
}

void NetworkPositionProvider::Handle_ConnectivityNotification(bool status) {
    LS_LOG_INFO("Handle_ConnectivityNotification status %d", status);
    mConnectivityStatus = status;
}

void NetworkPositionProvider::Handle_TelephonyNotification(bool status) {
    LS_LOG_INFO("Handle_TelephonyNotification status %d", status);
    mtelephonyPowerd = status;
}

void NetworkPositionProvider::Handle_WifiInternetNotification(bool status) {

}

void NetworkPositionProvider::Handle_SuspendedNotification(bool status) {

}