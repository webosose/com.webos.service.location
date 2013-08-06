/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Wifi_Handler.h
 * Purpose      : GObject WIFI handler header file
 * Platform     : RedRose
 * Author(s)    :  abhishek.srivastava
 * Email ID.    : abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _WIFI_HANDLER_H_
#define _WIFI_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_WIFI                  (wifi_handler_get_type())
#define WIFI_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_WIFI, WifiHandler))
#define WIFI_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_WIFI))
#define WIFI_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_WIFI, WifiHandlerClass))
#define WIFI_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_WIFI))
#define WIFI_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_WIFI, WifiHandlerClass))

typedef struct _WifiHandler WifiHandler;
typedef struct _WifiHandlerClass WifiHandlerClass;

struct _WifiHandler {
    GObject parent_instance;
};

struct _WifiHandlerClass {
    GObjectClass parent_class;
};

GType wifi_handler_get_type(void);

typedef enum {
    WIFI_PROGRESS_NONE = 0,
    WIFI_GET_POSITION_ON = 1 << 0,
    WIFI_START_TRACKING_ON = 1 << 1
} WifiHandlerStateFlags;

G_END_DECLS

#endif /* _WIFI_HANDLER_H_ */
