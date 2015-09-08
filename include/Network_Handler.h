/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Network_Handler.h
 * Purpose      : GObject NW handler header file
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

#ifndef _NETWORK_HANDLER_H_
#define _NETWORK_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_NETWORK                  (network_handler_get_type())
#define NETWORK_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_NETWORK, NetworkHandler))
#define NETWORK_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_NETWORK))
#define NETWORK_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_NETWORK, NetworkHandlerClass))
#define NETWORK_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_NETWORK))
#define NETWORK_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_NETWORK, NetworkHandlerClass))

typedef struct _NetworkHandler NetworkHandler;
typedef struct _NetworkHandlerClass NetworkHandlerClass;

struct _NetworkHandler {
    GObject parent_instance;
};

struct _NetworkHandlerClass {
    GObjectClass parent_class;
};

GType network_handler_get_type(void);

G_END_DECLS

#endif /* _NW_HANDLER_H_ */
