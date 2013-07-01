/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Google_Handler.h
 * Purpose      : Gooble handler header file
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

#ifndef _GOOGLE_HANDLER_H_
#define _GOOGLE_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_GOOGLE                  (google_handler_get_type())
#define GOOGLE_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_GOOGLE, GoogleHandler))
#define GOOGLE_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_GOOGLE))
#define GOOGLE_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_GOOGLE, GoogleHandlerClass))
#define GOOGLE_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_GOOGLE))
#define GOOGLE_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_GOOGLE, GoogleHandlerClass))

typedef struct _GoogleHandler GoogleHandler;
typedef struct _GoogleHandlerClass GoogleHandlerClass;


struct _GoogleHandler
{
    GObject parent_instance;
};

struct _GoogleHandlerClass
{
    GObjectClass parent_class;
};

GType google_handler_get_type(void);

typedef enum {
    GOOGLE_PROGRESS_NONE = 0,
    GOOGLE_GET_POSITION_ON=1 << 0,
    GOOGLE_START_TRACKING_ON = 1 << 1
} GoogleHandlerStateFlags;

typedef enum {
    GOOGLE_START_TRACK = 0,
    GOOGLE_GET_POSITION
} GoogleHandlerMethod;

G_END_DECLS

#endif  /* _GOOGLE_HANDLER_H_ */
