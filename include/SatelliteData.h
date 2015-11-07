/*
 * SatelliteData.h
 *
 *  Created on: Oct 6, 2015
 *      Author: rajeshgopu.iv
 */

#ifndef SATELLITEDATA_H_
#define SATELLITEDATA_H_

#include <glib.h>

class SatelliteData {
public:
  SatelliteData() {};

  ~SatelliteData() {};

    class SatelliteInfo {
        gint prn;
        gdouble snr;
        gdouble elevation;
        gdouble azimuth;
        gboolean used;
        gboolean hasalmanac;
        gboolean hasephemeris;
    };

private:
    guint num_satellite_used;
    guint visible_satellites_count;
    SatelliteInfo *sat_used;
};

#endif /* SATELLITEDATA_H_ */
