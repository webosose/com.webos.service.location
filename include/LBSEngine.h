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


#if !defined(_LBSENGINE_H)
#define _LBSENGINE_H
#include <string>
#include <map>
#include <WSPInterface.h>
#include <loc_log.h>
#include <memory>

class LBSEngine {

public:
    typedef std::map<std::string, WSPInterface* > WspProviderMap;
    static LBSEngine *getInstance() {
        static LBSEngine lbsObject;
        return &lbsObject;
    }
    WSPInterface* getWebServiceProvider(const std::string provideId);
    bool registerWebServiceProvider(const std::string provideId,  WSPInterface* wspInterface);
    void unregisterWebServiceProvider(const std::string provideId);
    virtual ~LBSEngine() {
        LS_LOG_DEBUG("Dtor of LBSEngine");

        mWspProviderMap.clear();
    }


private:
    LBSEngine() {
        LS_LOG_DEBUG("Ctor of LBSEngine");
    }
    LBSEngine(const LBSEngine& rhs);
    LBSEngine& operator=(const LBSEngine& rhs);

private:

    WspProviderMap mWspProviderMap;
};

#endif  //_LBSENGINE_H
