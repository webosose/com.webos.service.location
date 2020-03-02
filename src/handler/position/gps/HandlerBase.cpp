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

