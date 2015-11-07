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

#include <HandlerBase.h>

HandlerBase::HandlerBase() {
}

HandlerBase::~HandlerBase() {
    UnregisterMessages();
}

void HandlerBase::UnregisterMessages() {
    mGpsFunctionTable.clear();
}

bool HandlerBase::IsMsgSupported(operationCode_t opcode) {
    return (mGpsFunctionTable.find(opcode) != mGpsFunctionTable.end());
}

bool HandlerBase::HandleMessage(operationCode_t opCode,void* data) {
    if (IsMsgSupported(opCode)) {
        return mGpsFunctionTable[opCode](data);
    }
    return false;
}

