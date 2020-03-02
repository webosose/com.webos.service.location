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


#if !defined(_LBSENGINE_H)
#define _LBSENGINE_H

#include <string>
#include <map>
#include <WSPInterface.h>
#include <loc_log.h>
#include <memory>

class LBSEngine {

public:
    typedef std::map<std::string, WSPInterface *> WspProviderMap;

    static LBSEngine *getInstance() {
        static LBSEngine lbsObject;
        return &lbsObject;
    }

    WSPInterface *getWebServiceProvider(const std::string &provideId);

    bool registerWebServiceProvider(const std::string &provideId, WSPInterface *wspInterface);

    void unregisterWebServiceProvider(const std::string &provideId);

    virtual ~LBSEngine() {
        LS_LOG_DEBUG("Dtor of LBSEngine");
        mWspProviderMap.clear();
    }


private:
    LBSEngine() {
        LS_LOG_DEBUG("Ctor of LBSEngine");
    }

    LBSEngine(const LBSEngine &rhs) = delete;

    LBSEngine &operator=(const LBSEngine &rhs) = delete;

private:

    WspProviderMap mWspProviderMap;
};

#endif  //_LBSENGINE_H
