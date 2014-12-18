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
 * Filename  : LocationService.h
 * Purpose  :
 * Platform  :
 * Author(s)  :
 * E-mail id. :
 * Creation date :
 *
 * Modifications:
 *
 * Sl No Modified by  Date  Version Description
 *
 **********************************************************/

#include <iostream>
#include <lunaservice.h>
#include <glib.h>
#include "LocationService.h"
#include <loc_log.h>
#include <ConnectionStateObserver.h>

// PmLogging
#define LS_LOG_CONTEXT_NAME     "location"
PmLogContext gLsLogContext;

int main(int argc, char *argv[])
{
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);
    GMainLoop *mainLoop = g_main_loop_new(NULL, FALSE);

    if (mainLoop == NULL) {
        LS_LOG_DEBUG("Out of Memory");
        return EXIT_FAILURE;
    }

    LocationService *locService = NULL;
    ConnectionStateObserver *connectionStateObserverObj = NULL;
    locService = LocationService::getInstance();

    if (locService == NULL) {
        LS_LOG_DEBUG("locService instance failed create");
        g_main_loop_unref(mainLoop);
        return EXIT_FAILURE;
    }

    if (locService->init(mainLoop) == false) {
        g_main_loop_unref(mainLoop);
        delete locService;
        return EXIT_FAILURE;
    }

    connectionStateObserverObj = new(std::nothrow) ConnectionStateObserver();

    if (connectionStateObserverObj == NULL) {
        delete locService;
        locService == NULL;
        g_main_loop_unref(mainLoop);
        return EXIT_FAILURE;
    }

    connectionStateObserverObj->init(locService->getPrivatehandle());
    //Register
    connectionStateObserverObj->RegisterListener(locService);
    LS_LOG_DEBUG("Location service started.");
    g_main_loop_run(mainLoop);
    LS_LOG_DEBUG("Location service stopped.");
    g_main_loop_unref(mainLoop);
    //Unregister
    connectionStateObserverObj->UnregisterListener(locService);
    connectionStateObserverObj->finalize(locService->getPrivatehandle());
    //Unregister
    delete locService;
    locService = NULL;
    delete connectionStateObserverObj;
    connectionStateObserverObj = NULL;
    return EXIT_SUCCESS;
}
