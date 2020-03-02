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
