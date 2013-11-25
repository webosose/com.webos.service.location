/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Acccuracy.h
 * Purpose      : Provides Accuracy related variables utility functions
 * Platform     : RedRose
 * Author(s)    : rajeshg gopu
 * Email ID.    : rajeshgopu.iv@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _ACCURACY_H_
#define _ACCURACY_H_

#include <Location.h>

G_BEGIN_DECLS

typedef enum _GpsAccuracyLevel {
    ACCURACY_LEVEL_NONE = 0,
    ACCURACY_LEVEL_COUNTRY,
    ACCURACY_LEVEL_REGION,
    ACCURACY_LEVEL_LOCALITY,
    ACCURACY_LEVEL_POSTALCODE,
    ACCURACY_LEVEL_STREET,
    ACCURACY_LEVEL_DETAILED,
} GpsAccuracyLevel;

struct _Accuracy {
    GpsAccuracyLevel level;
    gdouble horizAccuracy;
    gdouble vertAccuracy;
};

Accuracy *accuracy_create(GpsAccuracyLevel level, gdouble horizontal_accuracy, gdouble vertical_accuracy);
void accuracy_free(Accuracy *accuracy);

G_END_DECLS

#endif  /* _ACCURACY_H_ */
