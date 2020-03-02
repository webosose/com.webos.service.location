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


#include <LBSEngine.h>
#include <WSPInterface.h>

WSPInterface* LBSEngine::getWebServiceProvider(const std::string &provideId)  {
    auto it = mWspProviderMap.find(provideId);
    if (it != mWspProviderMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool LBSEngine::registerWebServiceProvider(const std::string &provideId,
         WSPInterface* wspInterface) {
    auto it = mWspProviderMap.find(provideId);
    if (it == mWspProviderMap.end() && wspInterface) {
        mWspProviderMap.insert(std::make_pair(provideId, wspInterface));
    }
    return true;
}

void LBSEngine::unregisterWebServiceProvider(const std::string &provideId) {
    auto iter = mWspProviderMap.find(provideId);

    if ( iter != mWspProviderMap.end() ) {
        delete iter->second;
        mWspProviderMap.erase(iter);
    }
}
