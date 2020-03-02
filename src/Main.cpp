// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


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
