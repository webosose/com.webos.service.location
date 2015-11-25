/* vim:set et: */
/* Copyright (c) 2015, LG Electronics

2015.11 Hee S. Jeong, initial implementation */


#ifndef __MOCKLOCATION_H
#	define __MOCKLOCATION_H

#include <nyx/nyx_client.h>
#include "Location.h"
#include "ServiceAgent.h"

#define MOCK_TAG "MOCK: "
#define MOCK_DEBUG LS_LOG_INFO( MOCK_TAG "debug line, %s %s %d\n", __FILE__, __func__, __LINE__ );
#define MOCK_CALL_LOG LS_LOG_DEBUG( MOCK_TAG "===== %s =====\n", __func__ )

#ifndef MOCK_LOCATION_INTERNAL_SOURCE
#	define nyx_gps_init  gps_mock_init
#	define nyx_gps_start gps_mock_start
#	define nyx_gps_stop  gps_mock_stop
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _mock_location_provider {
    struct _mock_location_provider* next;
    const char* name;
    void* ctx;
    void (* location)( struct _Location* loc, void* ctx );
    unsigned flag;
};

#define MOCKLOC_FLAG_ENABLED            0x0001
#define MOCKLOC_FLAG_STARTED            0x0002
#define MOCKLOC_FLAG_DISABLE_AFTER_STOP 0x0004
#define MOCKLOC_FLAG_FINALIZE           0x0008

struct _mock_location_provider* get_mock_location_provider( const char* name );
enum LocationErrorCode set_mock_location_state( const char* name, int state );
enum LocationErrorCode set_mock_location( const char* name, struct _Location* loc );
enum LocationErrorCode start_mock_location( struct _mock_location_provider* mlp );
enum LocationErrorCode stop_mock_location( struct _mock_location_provider* mlp );
void finalize_mock_location( void );
nyx_error_t gps_mock_init(nyx_device_handle_t handle, nyx_gps_callbacks_t *gps_cbs, nyx_gps_xtra_callbacks_t *xtra_cbs, nyx_agps_callbacks_t *agps_cbs, nyx_gps_ni_callbacks_t *gps_ni_cbs, nyx_agps_ril_callbacks_t *agps_ril_cbs, nyx_gps_geofence_callbacks_t *geofence_cbs);
nyx_error_t gps_mock_start(nyx_device_handle_t handle);
nyx_error_t gps_mock_stop(nyx_device_handle_t handle);
const char* network_location_provider_url( const char* url );

#ifdef __cplusplus
}
#endif


#endif /* __MOCKLOCATION_H */

