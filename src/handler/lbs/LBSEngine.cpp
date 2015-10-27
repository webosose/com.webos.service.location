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


#include <LBSEngine.h>
#include <GoogleWSP.h>

WSPInterface* LBSEngine::getWebServiceProvider(const std::string provideId)  {
    auto it = mWspProviderMap.find(provideId);
    if (it != mWspProviderMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool LBSEngine::registerWebServiceProvider(const std::string provideId,
         WSPInterface* wspInterface) {
    auto it = mWspProviderMap.find(provideId);
    if (it == mWspProviderMap.end() && wspInterface) {
        mWspProviderMap.insert(std::make_pair(provideId, wspInterface));
    }
    return true;
}

void LBSEngine::unregisterWebServiceProvider(const std::string provideId) {
    auto iter = mWspProviderMap.find(provideId);

    if ( iter != mWspProviderMap.end() ) {
        delete iter->second;
        mWspProviderMap.erase(iter);
    }
}
