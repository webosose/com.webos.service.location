/*
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Accuracy.c
 *
 * Created on: Feb 22, 2013
 * Author: rajeshgopu.iv
 */

#include <stdlib.h>
#include <string.h>
#include <Position.h>
#include <Accuracy.h>

Accuracy *accuracy_create(GpsAccuracyLevel level, gdouble horizontal_accuracy, gdouble vertical_accuracy)
{
    Accuracy *accuracy = g_slice_new0(Accuracy);

    if (accuracy == NULL) return NULL;

    accuracy->level = level;
    accuracy->horizAccuracy = horizontal_accuracy;
    accuracy->vertAccuracy = vertical_accuracy;
    return accuracy;
}

void accuracy_free(Accuracy *accuracy)
{
    g_return_if_fail(accuracy);
    g_slice_free(Accuracy, accuracy);
}

