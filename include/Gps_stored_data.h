// Copyright (c) 2024 LG Electronics, Inc.
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


#ifndef _GPS_STORED_DATA_H_
#define _GPS_STORED_DATA_H_
#include <glib.h>
#include <Position.h>
#include <Location.h>

G_BEGIN_DECLS

void set_store_position(int64_t timestamp, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed,
                        gdouble direction, gdouble hor_accuracy, gdouble ver_accuracy , const char *path);
int get_stored_position(Position *position, Accuracy *accuracy, const char *path);

G_END_DECLS

#endif /* _GPS_STORED_DATA_H_ */
