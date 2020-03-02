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


#ifndef LOCATION_NTPCLIENT_H
#define LOCATION_NTPCLIENT_H

#include <stdint.h>
#include <GPSServiceConfig.h>


typedef int64_t ntpTime;
typedef int64_t ntpReferenceTime;

struct ntp_packet {
    unsigned char modeVNli;
    unsigned char stratum;
    char poll;
    char precision;
    unsigned long rootDelay;
    unsigned long rootDispersion;
    unsigned long referenceIdentifier;
    unsigned long referenceTimeStampSecs;
    unsigned long referenceTimeStampFreq;
    unsigned long originateTimeStampSecs;
    unsigned long originateTimeStampFreq;
    unsigned long receiveTimeStampSeqs;
    unsigned long receiveTimeStampFreq;
    unsigned long transmitTimeStampSecs;
    unsigned long transmitTimeStampFreq;
};

typedef enum {
    NONE,
    ALREADY_DOWNLOADING,
    CONNECTION_PROBLEM
} NtpErrors;

class NTPData {
private:
public:
    NTPData(const ntpTime &NtpTime, const ntpReferenceTime &NtpTimeReference, int RoundTripTime) : NtpTime(NtpTime),
                                                                                                   NtpTimeReference(
                                                                                                           NtpTimeReference),
                                                                                                   RoundTripTime(
                                                                                                           RoundTripTime) { }

    ntpTime getNtpTime() const {
        return NtpTime;
    }

    void setNtpTime(ntpTime NtpTime) {
        NTPData::NtpTime = NtpTime;
    }

    ntpReferenceTime getNtpTimeReference() const {
        return NtpTimeReference;
    }

    void setNtpTimeReference(ntpReferenceTime NtpTimeReference) {
        NTPData::NtpTimeReference = NtpTimeReference;
    }

    int getRoundTripTime() const {
        return RoundTripTime;
    }

    void setRoundTripTime(int RoundTripTime) {
        NTPData::RoundTripTime = RoundTripTime;
    }

private:
    ntpTime NtpTime;
    ntpReferenceTime NtpTimeReference;
    //Round trip is init ?? or signed or unsigned
    int RoundTripTime;

};

typedef enum {
    NTPPENDINGNETWORK = 0,
    NTPDOWNLOADING,
    NTPIDLE
} NtpDownloadState;

class INtpClinetCallback {
public :
    virtual void onRequestCompleted(NtpErrors error, const NTPData *data) = 0;

    virtual ~INtpClinetCallback() { };
};

class NtpClient {
private :
    NtpDownloadState mDownloadNtpDataStatus;
    GPSServiceConfig *mConfig;
    INtpClinetCallback *mCallback;
public:
    NtpClient();

    bool start(GPSServiceConfig *config, INtpClinetCallback *callback);

    GPSServiceConfig *getConfig() const {
        return mConfig;
    }

    INtpClinetCallback *getCallback() const {
        return mCallback;
    }

    const NtpDownloadState &getMDownloadNtpDataStatus() const {
        return mDownloadNtpDataStatus;
    }

private:
    static void ntpDownloadThread(void *arg);

    int64_t static getElapsedRealtime();
};


#endif //LOCATION_NTPCLIENT_H
