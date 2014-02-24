/* @@@LICENSE
 *
 *      Copyright (c) 2007-2012 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * LICENSE@@@ */

#ifndef _LUNA_LOCATION_SERVICE_UTIL_H_
#define _LUNA_LOCATION_SERVICE_UTIL_H_

#include <lunaservice.h>

void LSMessageReplyErrorInvalidHandle(LSHandle *sh, LSMessage *message);
void LSMessageReplyErrorNullPointer(LSHandle *sh, LSMessage *message);
void LSMessageReplyErrorInvalidTapiData(LSHandle *sh, LSMessage *message);
void LSMessageReplyErrorTapiCall(LSHandle *sh, LSMessage *message, int error_Code);
bool LSMessageReplyError(LSHandle *sh, LSMessage *message, int errorCode, char *errorText);
bool LSMessageReplySubscriptionSuccess(LSHandle *sh, LSMessage *message);
void LSSubscriptionRespondFailed();
//void LSMessageReplyErrorInvalidUserData(LSHandle *sh, LSMessage *message);
void LSMessageReplyErrorInvalidJSON(LSHandle *sh, LSMessage *message);
void LSMessageReplySuccess(LSHandle *sh, LSMessage *message);
void LSMessageReplyLocationError(LSHandle *sh, LSMessage *message, int errorCode);

#endif
