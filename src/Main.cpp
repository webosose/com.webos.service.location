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

#include <glib.h>
#include <loc_log.h>
#include "LocationService.h"

// PmLogging
#define LS_LOG_CONTEXT_NAME     "location"
PmLogContext gLsLogContext;

int main(int argc, char *argv[]) {
    // PmLog initialization
    PmLogGetContext(LS_LOG_CONTEXT_NAME, &gLsLogContext);
    GMainLoop *mainLoop = g_main_loop_new(NULL, FALSE);

    if (mainLoop == NULL) {
        LS_LOG_DEBUG("Out of Memory");
        return EXIT_FAILURE;
    }

    LocationService *locService = LocationService::getInstance();

    if (locService->init(mainLoop) == false) {
        g_main_loop_unref(mainLoop);
        return EXIT_FAILURE;
    }

    LS_LOG_DEBUG("Location service started.");
    g_main_loop_run(mainLoop);
    LS_LOG_DEBUG("Location service stopped.");
    g_main_loop_unref(mainLoop);
    locService->deinit();

    return EXIT_SUCCESS;
}
