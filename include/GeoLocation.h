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

#if !defined(_GEOLOCATION_H)
#define _GEOLOCATION_H

#include <string>

class GeoLocation {
public:
    GeoLocation();

    GeoLocation(std::string slocation);

    GeoLocation(double latitude, double longitude, double altitude, double horAccuracy, double timestamp,
                double direction, double climb, double speed, double vertAcc);

    GeoLocation(const GeoLocation &rhs) = default;

    GeoLocation &operator=(const GeoLocation &rhs) = default;

    bool operator==(const GeoLocation &other) const;

    ~GeoLocation();

    double getLatitude() const;

    double getLongitude() const;

    double getAltitude() const;

    double getTimeStamp() const;

    double getSpeed() const;

    double getDirection() const;

    double getClimb() const;

    double getVerticalAccuracy() const;

    double getHorizontalAccuracy() const;

    std::string toString() const;

    void setLatitude(double latitude);

    void setLongitude(double longitude);

    void setAltitude(double altitude);

    void printLocation();

    const std::string &getProviderId() const;

    void setProviderId(const std::string &providerId);

    void setHorizontalAccuracy(double horAccuracy);

    void setVerticalAccuracy(double vertAccuracy);
private:

    std::string mStrLocation;
    double mLatitude;
    double mLongitude;
    double mAltitude;
    double mHorAccuracy;
    double mTimeStamp;
    double mDirection;
    double mClimb;
    double mSpeed;
    double mVertAccuracy;
    std::string mProviderId;


};

#endif  //_GEOLOCATION_H
