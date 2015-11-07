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

#include <NetworkRequestManager.h>
#include <loc_log.h>

using namespace std;

NetworkRequestManager::NetworkRequestManager() {
    LS_LOG_DEBUG("NetworkRequestManager ctor");
}

NetworkRequestManager::~NetworkRequestManager() {
    LS_LOG_DEBUG("NetworkRequestManager dtor");
}

void NetworkRequestManager::init() {
    loc_http_start();
    loc_http_set_callback(handleDataCb, this);

    _attributes.set(Network_Defines::ACTIVE);

    LS_LOG_DEBUG("NetworkRequestManager started");
}

void NetworkRequestManager::deInit() {
    if (!_attributes.test(Network_Defines::ACTIVE)) {
        LS_LOG_ERROR("Network Manager already Inactive!!");
        return;
    }

    _attributes.reset(Network_Defines::ACTIVE); // clear the active bit

    loc_http_stop();

    LS_LOG_DEBUG("NetworkRequestManager stopped");
}

void NetworkRequestManager::handleDataCb(HttpReqTask *task, void *user_data) {

    if (!task || !user_data) {
        LS_LOG_DEBUG("fatal!!..invalid data received in callback");
        return;
    }

    NetworkRequestManager *pThis = (NetworkRequestManager *) user_data;
    LS_LOG_DEBUG("handleDataCb task  %p", task);
    HttpInterface *client = pThis->httpRequestList[(LSMessage *) task->message];

    if (HTTP_STATUS_CODE_SUCCESS == task->curlDesc.httpResponseCode) {
        LS_LOG_DEBUG("cbHttpResponsee %s", task->responseData);
    } else {
        LS_LOG_DEBUG("task->curlDesc.curlResultErrorStr %s", task->curlDesc.curlResultErrorStr);
    }

    LS_LOG_DEBUG("received client address %p", client);

    if (client)
        client->handleResponse(task);
    else {
        //client is removed, clear task
        LS_LOG_INFO("No client to handleResponse");
        pThis->clearTransaction(task);
    }

}

ErrorCodes NetworkRequestManager::initiateTransaction(const char **headers, int size, string url, bool isSync,
                                                      LSMessage *message, HttpInterface *userdata, char *post_data) {

    if (!_attributes.test(Network_Defines::ACTIVE)) {
        LS_LOG_DEBUG("Network Manager Inactive");
        return ERROR_MUTLITHREAD;
    }

    HttpReqTask *gHttpReqTask = loc_create_http_task(headers, 0, message, userdata);

    if (NULL == gHttpReqTask) {
        LS_LOG_ERROR("Fatal error...loc_create_http failed!!");
        return ERROR_NETWORK_ERROR;
    }

    if (!loc_http_task_prepare_connection(&gHttpReqTask, (char *) url.c_str())) {
        loc_http_task_destroy(&gHttpReqTask);
        return ERROR_NETWORK_ERROR;
    }

    LS_LOG_INFO("initiate_Transaction succesful gHttpReqTask %d=\n", gHttpReqTask);

    if (gHttpReqTask->post_data) {     // free up any past data
        free(gHttpReqTask->post_data);
        gHttpReqTask->post_data = NULL;
    }

    if (post_data)
        gHttpReqTask->post_data = strdup(post_data);

    if (!loc_http_add_request(gHttpReqTask, isSync)) {
        loc_http_task_destroy(&gHttpReqTask);
        return ERROR_NETWORK_ERROR;
    }

    httpRequestList[message] = userdata;
    return ERROR_NONE;

}

void NetworkRequestManager::cancelTransaction(HttpReqTask *) {
//TBD
}

void NetworkRequestManager::clearTransaction(HttpReqTask *task) {
    httpRequestList.erase((LSMessage *) task->message);
    loc_http_remove_request(task);
    loc_http_task_destroy(&task);

    LS_LOG_DEBUG("transaction cleared");
}

