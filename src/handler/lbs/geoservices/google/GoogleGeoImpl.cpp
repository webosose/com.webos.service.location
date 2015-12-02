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


#include <GoogleGeoImpl.h>

using namespace std;

#define GEOCODEMETHOD    "getGeoCodeLocation"
#define REVGEOMETHOD       "getReverseLocation"

void GoogleGeoImpl::handleResponse(HttpReqTask *task) {
    int error = ERROR_NONE;
    char *response = NULL;
    LSMessage *message = NULL;

    if (!task) {
        return;
    }

    message = (LSMessage *) task->message;

    if (HTTP_STATUS_CODE_SUCCESS == task->curlDesc.httpResponseCode) {
        response = g_strdup(task->responseData);
        LS_LOG_DEBUG("cbHttpResponsee %s", response);
    } else {
        LS_LOG_INFO("task->curlDesc.curlResultErrorStr %s", task->curlDesc.curlResultErrorStr);
        error = ERROR_NETWORK_ERROR;
        response = g_strdup("HTTP Connection failed");
    }

    NetworkRequestManager::getInstance()->clearTransaction(task);

    LS_LOG_INFO("cbHttpResponse : %d %p", error, message);

    if (!strcmp(LSMessageGetMethod(message), GEOCODEMETHOD)) {
        mGeoCodeCb(GeoLocation(response), error, message);
    } else if (!strcmp(LSMessageGetMethod(message), REVGEOMETHOD)) {
        mRevGeoCodeCb(GeoAddress(response), error, message);
    }

    if (response)
        free(response);

}

GoogleGeoImpl::GoogleGeoImpl() {
    char* key = readApiKey();
    if(key)
        mGoogleGeoCodeApiKey = key;//assign only if non-null
    free(key);
}

GoogleGeoImpl::~GoogleGeoImpl() {
    LS_LOG_DEBUG("===GoogleGeoImpl Dtor====");
}

ErrorCodes GoogleGeoImpl::geoCode(GeoAddress address, GeoCodeCb geoCodeCb, bool isSync, LSMessage *message) {
    LS_LOG_DEBUG("GoogleGeoImpl Geocode %s %p", address.toString().c_str(), message);

    if (nullptr == geoCodeCb) {
        LS_LOG_ERROR("geoCodeCb is NULL");
        return ERROR_NOT_AVAILABLE;
    }

    if (mGoogleGeoCodeApiKey.empty()) {
        LS_LOG_ERROR("License Key not present");
        return ERROR_LICENSE_KEY_INVALID;
    }

    this->mGeoCodeCb = geoCodeCb;

    string strFormattedUrl = formatUrl(address.toString(), mGoogleGeoCodeApiKey.c_str());
    return lbsPostQuery(strFormattedUrl, isSync, message);
}

ErrorCodes GoogleGeoImpl::reverseGeoCode(GeoLocation geolocation, ReverseGeoCodeCb revGeocodeCallback, bool isSync,
                                         LSMessage *message) {
    LS_LOG_DEBUG("GoogleGeoImpl reverseGeoCode %s %p", geolocation.toString().c_str(), message);

    if (nullptr == revGeocodeCallback) {
        LS_LOG_ERROR("revGeocodeCallback is NULL");
        return ERROR_NOT_AVAILABLE;
    }

    if (mGoogleGeoCodeApiKey.empty()) {
        LS_LOG_ERROR("License Key not present");
        return ERROR_LICENSE_KEY_INVALID;
    }

    this->mRevGeoCodeCb = revGeocodeCallback;

    string strFormattedUrl = formatUrl(geolocation.toString(), mGoogleGeoCodeApiKey.c_str());
    return lbsPostQuery(strFormattedUrl, isSync, message);
}

string GoogleGeoImpl::formatUrl(string geoData, const char *key) {
    guchar *decodedKey = NULL;
    gsize size = 0;
    gsize len;
    GHmac *hmac = NULL;
    gchar *encodedSignature = NULL;
    guint8 *buffer = NULL;
    string finalURL;
    string encodedSignatureStr;
    string urlToSign = TOSTRING(GOOGLE_SUB_URL_TO_SIGN) + geoData + TOSTRING(GOOGLE_CLIENT_KEY);

    //Decode the private key
    decodedKey = g_base64_decode(key, &size);
    if (NULL == decodedKey) {
        goto EXIT;
    }

    hmac = g_hmac_new(G_CHECKSUM_SHA1, decodedKey, size);
    if (NULL == hmac) {
        goto EXIT;
    }

    g_hmac_update(hmac, (unsigned char *) urlToSign.c_str(), -1);
    buffer = (guint8 *) g_malloc(strlen(urlToSign.c_str()));
    if (NULL == buffer) {
        goto EXIT;
    }

    len = urlToSign.length();
    g_hmac_get_digest(hmac, buffer, &len);

    //Encode the binary signature into base64 for use within a URL
    encodedSignature = g_base64_encode(buffer, len);

    if (NULL == encodedSignature) {
        goto EXIT;
    }

    encodedSignatureStr = string((const char *) encodedSignature);

    //make url-safe by replacing '+' and '/' characters
    replace(encodedSignatureStr.begin(), encodedSignatureStr.end(), '/', '_');
    replace(encodedSignatureStr.begin(), encodedSignatureStr.end(), '+', '-');

    finalURL = TOSTRING(GOOGLE_LBS_URL);
    finalURL.append(urlToSign);
    finalURL.append("&signature=");
    finalURL.append(encodedSignatureStr);

    LS_LOG_DEBUG("url: formatted succesfully");

    EXIT:

    if (decodedKey != NULL)
        g_free(decodedKey);

    if (hmac != NULL)
        g_hmac_unref(hmac);

    if (encodedSignature != NULL)
        delete encodedSignature;

    if (buffer != NULL)
        free(buffer);

    return finalURL;
}

char *GoogleGeoImpl::readApiKey() {
    unsigned char *geocode_api_key = NULL;
    int ret_geocode = LOC_SECURITY_ERROR_FAILURE;

    ret_geocode = locSecurityBase64Decode(GEOCODEKEY_CONFIG_PATH,
                                          &geocode_api_key);

    if (LOC_SECURITY_ERROR_SUCCESS == ret_geocode) {
        return (char*)geocode_api_key;
    } else {
        return NULL;
    }
}

ErrorCodes GoogleGeoImpl::lbsPostQuery(string url, bool isSync, LSMessage *message) {
    LS_LOG_DEBUG("==lbsPostQuery==");
    return NetworkRequestManager::getInstance()->initiateTransaction(NULL, 0, url, isSync, message, this);
}

