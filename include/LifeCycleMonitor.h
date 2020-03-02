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


#ifndef _LIFECYCLEMONITOR_H_
#define _LIFECYCLEMONITOR_H_

#include <glib.h>
#include <lunaservice.h>


class LifeCycleMonitor {
public:
    LifeCycleMonitor();

    ~LifeCycleMonitor();

    // Suspend Monitor
    bool registerSuspendMonitor(LSHandle *service);

    // Set wake lock
    void setWakeLock(bool set, bool force = false);

private:
    bool callService(LSHandle *service, LSFilterFunc callback,
                     const char *url, const char *payload);

    // Suspend Monitor callbacks
    static bool cbPowerdUp(LSHandle *sh, const char *serviceName, bool connected, void *ctx);

    static bool cbIdentify(LSHandle *sh, LSMessage *message, void *ctx);

    static bool cbRegisterWakeLock(LSHandle *sh, LSMessage *message, void *ctx);

private:
    LSHandle *m_suspendService;
    char *m_suspendMonitorId;
    bool m_registeredWakeLock;
    bool m_setWakeLock;
    int m_refWakeLock;
};


#endif
