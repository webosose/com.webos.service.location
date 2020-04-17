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


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <loc_log.h>
#include "MockLocation.h"

#define _NETWORK_MOCK_SERVER_PORT 10080
#define _NETWORK_PORT_TO_STR(p) #p
#define _NETWORK_PORT_STR(p) _NETWORK_PORT_TO_STR(p)

extern struct _mock_location_provider network_mock_location_provider;
double network_mock_lon, network_mock_lat, network_mock_acc;

static pthread_t network_mock_server_thread;
int network_event_fd;
int network_mock_server = -1, network_mock_client = -1;

void
close_socket( int* sock )
{
    int s = *sock;
    if ( s >= 0 ) {
        shutdown(s,SHUT_RDWR);
        close( s );
        *sock = -1;
    }
}

static void*
start_network_mock_server( void* param )
{
    uint64_t u = 1;
    network_mock_server = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    (void)write( network_event_fd, &u, sizeof(u) );
    if ( network_mock_server >= 0 ) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)_NETWORK_MOCK_SERVER_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if ( bind( network_mock_server,
                (struct sockaddr*)&addr, sizeof(addr) ) == 0 ) {
            listen( network_mock_server, 50 );
            while ( ! ( network_mock_location_provider.flag
                        & MOCKLOC_FLAG_FINALIZE ) ) {
                socklen_t len = sizeof(addr);
                network_mock_client = accept
                    ( network_mock_server, (struct sockaddr*)&addr, &len );
                if ( network_mock_client >= 0 ) {
                    /* allow local connection only */
                    if ( htonl(addr.sin_addr.s_addr) == 0x7f000001 ) {
                        char buf[1024];
                        int len, pi = 0;
                        static const char p[] = { '\r', '\n', '\r', '\n' };
                        LS_LOG_INFO( MOCK_TAG "client accepted" );
                        while ( pi >= 0 ) {
                            int i;
                            len = recv( network_mock_client,
                                buf, sizeof(buf) - 1, 0 );
                            if ( len <= 0 ) break;
                            buf[len] = 0;
                            for ( i = 0; i < len; i++ ) {
                                if ( buf[i] == p[pi] ) {
                                    if ( pi >= sizeof(p) - 1 ) {
                                        pi = -1;
                                        break;
                                    }
                                    pi ++;
                                }
                                else {
                                    pi = 0; /* rematch from the first */
                                }
                            }
                        }
                        if ( network_mock_client >= 0 ) {
                            static const char header[] =
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Length: %d\r\n"
                                "Connection: close\r\n"
                                "\r\n";
                            static const char body[] =
                                "{\"location\":{\"lat\":%f,\"lng\":%f},\"accuracy\":%f}\r\n\r\n";
                            for(;;) {
                                fd_set rfds;
                                FD_ZERO( &rfds );
                                FD_SET( network_event_fd, &rfds );
                                FD_SET( network_mock_client, &rfds );
                                select( ( network_event_fd > network_mock_client ?
                                    network_event_fd : network_mock_client ) + 1,
                                    &rfds, NULL, NULL, NULL );
                                if ( FD_ISSET(network_event_fd,&rfds) ) {
                                    uint64_t u;
                                    (void)read( network_event_fd, &u, sizeof(u) );
                                    snprintf( buf, sizeof(buf), body,
                                        network_mock_lat, network_mock_lon, network_mock_acc );
                                    len = strlen(buf);
                                    snprintf( buf + len, sizeof(buf) - len, header, len );
                                    send( network_mock_client, buf + len, strlen(buf + len), 0 );
                                    send( network_mock_client, buf, len, 0 );
                                    LS_LOG_INFO( MOCK_TAG "NETWORK: %f %f",
                                        network_mock_lat, network_mock_lon );
                                    break;
                                }
                                if ( FD_ISSET(network_mock_client,&rfds) ) {
                                    len = recv( network_mock_client,
                                        buf, sizeof(buf) - 1, 0 );
                                    if ( len <= 0 ) {
                                        LS_LOG_INFO( MOCK_TAG "client leaves" );
                                        break;
                                    }
                                    buf[len] = 0;
                                }
                            }
                        }
                    }
                    close_socket( &network_mock_client );
                    /* auto stop because there is no explicit stop for nlp */
                    stop_mock_location( &network_mock_location_provider );
                }
            }
        } else {
            LS_LOG_ERROR( MOCK_TAG "Binding failed" );
            close_socket( &network_mock_server );
        }
    }
    return NULL;
}

static void
network_mock_location( struct _Location* loc, void* ctx )
{
    if ( loc ) {
        uint64_t u = 1;
        network_mock_lon = loc->longitude;
        network_mock_lat = loc->latitude;
        network_mock_acc = loc->horizontalAccuracy;
        (void)write( network_event_fd, &u, sizeof(u) );
    } else {
        void* res;
        close_socket( &network_mock_client );
        close_socket( &network_mock_server );
        close( network_event_fd );
        pthread_join( network_mock_server_thread, &res );
    }
}


const char*
network_location_provider_url( const char* url )
{
    if ( start_mock_location( &network_mock_location_provider ) == LOCATION_SUCCESS ) {
        if ( network_mock_server < 0 ) {
            uint64_t u;
            pthread_attr_t attr;
            pthread_attr_init( &attr );
            pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );

            network_event_fd = eventfd( 0, 0 );
            pthread_create(&network_mock_server_thread, NULL, start_network_mock_server, NULL);
            (void)read( network_event_fd, &u, sizeof(u) );

            pthread_attr_destroy( &attr );
            network_mock_location_provider.location = network_mock_location;
        }
        return "http://127.0.0.1:" _NETWORK_PORT_STR(_NETWORK_MOCK_SERVER_PORT) "/nlp?";
    }

    return url;
}



/* vim:set et: */
