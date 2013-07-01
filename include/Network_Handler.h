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

#ifndef _NW_HANDLER_H_
#define _NW_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_NW                  (nw_handler_get_type())
#define NW_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_NW, _NwHandler))
#define NW_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_NW))
#define NW_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_NW, _NwHandlerClass))
#define NW_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_NW))
#define NW_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_NW, _NwHandlerClass))

typedef struct _NwHandler NwHandler;
typedef struct _NwHandlerClass NwHandlerClass;


struct _NwHandler
{
    GObject parent_instance;
};

struct _NwHandlerClass
{
    GObjectClass parent_class;
};

GType nw_handler_get_type(void);

#define MAX_HANDLER_TYPE    3

G_END_DECLS

#endif /* _NW_HANDLER_H_ */
