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


#include "NtpClient.h"

#include <glib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <loc_log.h>
#include <unistd.h>

#define SOCK_DEFAULT_PORT               123
#define SOCK_MODE                       (3 << 3) | 3
// NTP Time
// NTP time stamp of 1 Jan 1970 = 2,208,988,800
#define NTP_EPOCH                       (86400U * (365U * 70U + 17U))
#define NTP_REPLY_TIMEOUT               6000
#define OFFSET                          1000

#define NANO_SEC_FRAQ                   1000000000LL

#define MILLI_SEC_FRAQ                  1000000


NtpClient::NtpClient() : mDownloadNtpDataStatus(NtpDownloadState::NTPIDLE), mConfig(nullptr), mCallback(nullptr) { }

bool NtpClient::start(GPSServiceConfig *config, INtpClinetCallback *callback) {
    LS_LOG_DEBUG("enter gpsRequestUtcTimeCb\n");

    if (config == nullptr || callback == nullptr)
        return false;
    if (NtpDownloadState::NTPDOWNLOADING == mDownloadNtpDataStatus) {
        return true;
    }
    mCallback = callback;
    mConfig = config;
    mDownloadNtpDataStatus = NtpDownloadState::NTPDOWNLOADING;
    if (!g_thread_new("download ntp time",
                      (GThreadFunc) NtpClient::ntpDownloadThread, this)) {
        LS_LOG_ERROR("failed to create ntp download thread\n");
     mCallback->onRequestCompleted(NtpErrors::CONNECTION_PROBLEM, nullptr);
        return false;
    }

    return true;
}

void NtpClient::ntpDownloadThread(void *arg) {
    //Casting in c style ?? bad ??
    NtpClient *ntpClient = (NtpClient *) arg;

    GPSServiceConfig mGPSConf = *ntpClient->mConfig;
    struct sockaddr_in sock_addr;
    struct hostent *he;
    struct timeval tv;
    struct ntp_packet pkt;
    int usd = -1;
    int timeout;
    int nextserverindex = 0;
    int noofservers = 3;
    int count = 0;
    int len = 0;
    char *ntpservers[noofservers];

    LS_LOG_DEBUG("enter NtpClient::ntpDownloadThread\n");


    if (mGPSConf.mNTPServer1 != nullptr)
        ntpservers[count++] = mGPSConf.mNTPServer1;

    if (mGPSConf.mNTPServer2 != nullptr)
        ntpservers[count++] = mGPSConf.mNTPServer2;

    if (mGPSConf.mNTPServer3 != nullptr)
        ntpservers[count++] = mGPSConf.mNTPServer3;

    if (!count) {
        LS_LOG_ERROR("No NTP servers were specified in the GPS configuration\n");
        return;
    }

    LS_LOG_DEBUG("ntp download thread started\n");


    while (nextserverindex < count) {

        if (-1 == (usd = socket(AF_INET, SOCK_DGRAM, 0))) {
            LS_LOG_ERROR("ntp download socket error\n");
            return;
        }

    if ((he = gethostbyname(ntpservers[nextserverindex]))) {
            memset(&sock_addr, 0, sizeof(sock_addr));
            memcpy(&sock_addr.sin_addr, he->h_addr_list[0], sizeof(sock_addr.sin_addr));
            sock_addr.sin_port = htons(SOCK_DEFAULT_PORT);
            sock_addr.sin_family = AF_INET;

            if (-1 != connect(usd, (struct sockaddr *) &sock_addr, sizeof(sock_addr))) {
                memset(&pkt, 0, sizeof(pkt));
                pkt.modeVNli = SOCK_MODE;

                int64_t requestTicks = NtpClient::getElapsedRealtime();
                //TODO:time_t is not ime_t type as a signed integer (typically 32 or 64 bits wide)
                //Systems using a 32-bit time_t type are susceptible to the Year 2038 problem :-)
                //if system is using 64bit htonl takes  uint32_t then this passing is right ??
                pkt.originateTimeStampSecs = htonl(time(nullptr) + NTP_EPOCH);
                send(usd, &pkt, sizeof(pkt), 0);
                timeout = NTP_REPLY_TIMEOUT;
                setsockopt(usd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(int));

                LS_LOG_DEBUG("connected to socket, call recv\n");

                len = recvfrom(usd, &pkt, sizeof(pkt), 0, nullptr, nullptr);
                int64_t responseTicks = NtpClient::getElapsedRealtime();
                //TODO:nthol uint32_t where as transmitTimeStampSecs unsigned long ?? is it ok.
                tv.tv_sec = ntohl(pkt.transmitTimeStampSecs) - NTP_EPOCH;

                LS_LOG_DEBUG("tv.tv_sec= %ld\n", tv.tv_sec);

                if (len > 0) {
                    int64_t NtpTime = tv.tv_sec * OFFSET;
                    int64_t NtpTimeReference = responseTicks;
                    //TODO :: RoundTripTime is int ??? should be unsigned long ?
                    int RoundTripTime = responseTicks - requestTicks -
                                        (pkt.transmitTimeStampSecs - pkt.receiveTimeStampSeqs);
                    LS_LOG_INFO("NtpTime = %lld and NtpTimeReference = %lld and RoundTripTime=%d\n tv.tv_sec= %ld\n",
                                 NtpTime,
                                 NtpTimeReference,
                                 RoundTripTime,
                                 tv.tv_sec);

                    NTPData ntpData(NtpTime, NtpTimeReference, RoundTripTime);
                    ntpClient->mCallback->onRequestCompleted(NtpErrors::NONE, &ntpData);

                    break; // break for success
                } else {
                    LS_LOG_INFO("Download Length 0 \n");
                    //TODO: on failure why should a new socket needs to be created ??
                    close(usd);
                }
            }
            else {
                LS_LOG_ERROR("ntp download Error in connect\n");
            }
        }
        else {
            LS_LOG_ERROR("ntp download Error gethostbyname\n");

        }
        nextserverindex++;
    }

    if (nextserverindex >= count)
        ntpClient->mCallback->onRequestCompleted(NtpErrors::CONNECTION_PROBLEM, nullptr);

    close(usd);
    ntpClient->mDownloadNtpDataStatus = NtpDownloadState::NTPIDLE;
}

int64_t NtpClient::getElapsedRealtime() {
    struct timespec tp;

    tp.tv_sec = tp.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    int64_t when = tp.tv_sec * NANO_SEC_FRAQ + tp.tv_nsec;

    return when / MILLI_SEC_FRAQ;
}
