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


#ifndef __HTTP_INTERFACE__
#define __HTTP_INTERFACE__

#include "loc_http.h"
//class HttpReqTask;
/*This interface is necessary to be implemented by teh clients of HTTPEngine*/

class HttpInterface {
public:
    virtual void handleResponse(HttpReqTask *task) = 0;

};

#endif
