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


#include <loc_log.h>
#include <string.h>
#include "MockLocation.h"


static const char mock_location_disabled_msg[] = MOCK_TAG "%s mock location was not enabled";

struct _mock_location_provider network_mock_location_provider = {
    .name = "network",
};

struct _mock_location_provider gps_mock_location_provider = {
    .name = "gps",
    .next = &network_mock_location_provider,
};

static struct _mock_location_provider* mock_location_provider = &gps_mock_location_provider;


struct _mock_location_provider*
get_mock_location_provider( const char* name )
{
    struct _mock_location_provider* mlp = mock_location_provider;
    while ( mlp ) {
        if ( strcmp(mlp->name,name) == 0 ) return mlp;
        mlp = mlp->next;
    }
    return NULL;
}


enum LocationErrorCode
set_mock_location_state( const char* name, int state )
{
    struct _mock_location_provider* mlp = get_mock_location_provider(name);
    if ( mlp ) {
        if ( state ) {
            mlp->flag |= MOCKLOC_FLAG_ENABLED;
        } else {
            if ( mlp->flag & MOCKLOC_FLAG_STARTED ) {
                mlp->flag |= MOCKLOC_FLAG_DISABLE_AFTER_STOP;
            } else {
                mlp->flag &= ~MOCKLOC_FLAG_ENABLED;
            }
        }
        LS_LOG_INFO( MOCK_TAG "%s mock location state %d", name, state );
        return LOCATION_SUCCESS;
    }
    return LOCATION_UNKNOWN_MOCK_PROVIDER;
}


enum LocationErrorCode
set_mock_location( const char* name, struct _Location* loc )
{
    struct _mock_location_provider* mlp = get_mock_location_provider(name);
    if ( mlp ) {
        if ( mlp->flag & MOCKLOC_FLAG_ENABLED ) {
            if ( mlp->flag & MOCKLOC_FLAG_STARTED ) {
                mlp->location( loc, mlp->ctx );
                return LOCATION_SUCCESS;
            }
            LS_LOG_INFO( MOCK_TAG "%s was not started", name );
            return LOCATION_NOT_STARTED;
        }
        else {
            LS_LOG_INFO( mock_location_disabled_msg, name );
            return LOCATION_MOCK_DISABLED;
        }
    }
    return LOCATION_UNKNOWN_MOCK_PROVIDER;
}


enum LocationErrorCode
start_mock_location( struct _mock_location_provider* mlp )
{
    if ( mlp->flag & MOCKLOC_FLAG_ENABLED ) {
        mlp->flag |= MOCKLOC_FLAG_STARTED;
        return LOCATION_SUCCESS;
    }
    LS_LOG_ERROR( mock_location_disabled_msg, mlp->name );
    return LOCATION_MOCK_DISABLED;
}

enum LocationErrorCode
stop_mock_location( struct _mock_location_provider* mlp )
{
    if ( mlp->flag & MOCKLOC_FLAG_ENABLED ) {
        mlp->flag &= ~MOCKLOC_FLAG_STARTED;
        if ( mlp->flag & MOCKLOC_FLAG_DISABLE_AFTER_STOP ) {
            mlp->flag &= ~MOCKLOC_FLAG_ENABLED;
        }
        return LOCATION_SUCCESS;
    }
    LS_LOG_ERROR( mock_location_disabled_msg, mlp->name );
    return LOCATION_MOCK_DISABLED;
}


void
finalize_mock_location( void )
{
    struct _mock_location_provider* mlp = mock_location_provider;
    while ( mlp ) {
        mlp->flag |= MOCKLOC_FLAG_FINALIZE;
        mlp->location( NULL, mlp->ctx );
        mlp = mlp->next;
    }
    LS_LOG_INFO( MOCK_TAG "Finalized. Bye ~" );
}


/* EOF */

