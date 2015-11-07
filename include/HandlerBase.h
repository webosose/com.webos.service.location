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

#ifndef HANDLERBASE_H_
#define HANDLERBASE_H_

#include <cassert>
#include <cstdarg>
#include <functional>
#include <map>
#include <stdio.h>
#include <stdint.h>
#include <loc_log.h>
typedef uint32_t operationCode_t;

typedef std::function<bool(void *data)> GpsMsgFunction_t;

class HandlerBase {
public:
    static const uint32_t kEndList = 0xFFFFFFFF;

    HandlerBase();

    virtual ~HandlerBase();

    template<class C>
    void RegisterMessageHandlers(operationCode_t opCode, bool (C::*function)(void *data), ...) {
        va_list argList;
        va_start(argList, function);

        RegisterMessageHandler(opCode, function);
        opCode = va_arg(argList, operationCode_t);
        while (opCode != kEndList) {
            function = va_arg(argList, bool(C::*)
                    (void * data));
            RegisterMessageHandler(opCode, function);
            opCode = va_arg(argList, operationCode_t);
        }
        va_end(argList);
    }

    void UnregisterMessages();

    bool HandleMessage(operationCode_t opCode, void *data);

private :
    std::map<operationCode_t, GpsMsgFunction_t> mGpsFunctionTable;

    bool IsMsgSupported(operationCode_t opcode);

    template<class C>
    inline void RegisterMessageHandler(operationCode_t opCode, bool (C::*function)(void *data)) {
        if (mGpsFunctionTable.end() != mGpsFunctionTable.find(opCode)) {
            LS_LOG_DEBUG("opcode 0x%d already registered", opCode);
        }

        assert(mGpsFunctionTable.end() == mGpsFunctionTable.find(opCode));
        mGpsFunctionTable[opCode] = std::bind(function, (C *) this, std::placeholders::_1);

    }
};

#endif /* HANDLERBASE_H_ */
