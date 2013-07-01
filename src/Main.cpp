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
#include "Conf.h"
#include <LocationService_Log.h>

// PmLogging
#define LS_LOG_CONTEXT_NAME     "avocado.location.location"
PmLogContext gLsLogContext;

int main(int argc, char *argv[])
{
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);

    GMainLoop *mainLoop = g_main_loop_new(NULL, FALSE);
    if (mainLoop == NULL) {
        LS_LOG_DEBUG("Out of Memory");
        return 0;
    }

    LocationService *locService = NULL;
    LocationService::setGmainloop(mainLoop);
    locService = LocationService::getInstance(mainLoop);
    if(locService == NULL)
    {
        LS_LOG_DEBUG("locService instance failed create");
        g_main_loop_unref(mainLoop);
        return 0;
    }
    if(locService->init() == false)
    {
        g_main_loop_unref(mainLoop);
        delete locService;
        return 0;
    }
    LS_LOG_DEBUG("Location service started.");
    g_main_loop_run (mainLoop);
    LS_LOG_DEBUG("Location service stopped.");
    g_main_loop_unref(mainLoop);
    delete locService;
    locService == NULL;
    return 0;

}
