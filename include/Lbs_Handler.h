/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Lbs_Handler.h
 * Purpose      : Gobject Lbs handler header file
 * Platform     : RedRose
 * Author(s)    : abhishek.srivastava
 * Email ID.    : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _LBS_HANDLER_H_
#define _LBS_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_LBS                  (lbs_handler_get_type())
#define LBS_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_LBS, LbsHandler))
#define LBS_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_LBS))
#define LBS_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_LBS, LbsHandlerClass))
#define LBS_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_LBS))
#define LBS_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_LBS, LbsHandlerClass))

typedef struct _LbsHandler LbsHandler;
typedef struct _LbsHandlerClass LbsHandlerClass;

struct _LbsHandler {
    GObject parent_instance;
};

struct _LbsHandlerClass {
    GObjectClass parent_class;
};

GType lbs_handler_get_type(void);

typedef enum {
    LBS_PROGRESS_NONE = 0,
    LBS_GEOCODE_ON = 1 << 0,
    LBS_REVGEOCODE_ON = 2 << 0,
} GeocodeHandlerStateFlags;

G_END_DECLS

#endif  /* _LBS_HANDLER_H_ */
