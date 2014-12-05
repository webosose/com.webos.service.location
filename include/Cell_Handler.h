/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Cell_Handler.h
 * Purpose      : Gobject CELL handler header file
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

#ifndef _CELL_HANDLER_H_
#define _CELL_HANDLER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define HANDLER_TYPE_CELL                  (cell_handler_get_type())
#define CELL_HANDLER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HANDLER_TYPE_CELL, CellHandler))
#define CELL_IS_HANDLER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HANDLER_TYPE_CELL))
#define CELL_HANDLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), HANDLER_TYPE_CELL, CellHandlerClass))
#define CELL_IS_HANDLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), HANDLER_TYPE_CELL))
#define CELL_HANDLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HANDLER_TYPE_CELL, CellHandlerClass))

typedef struct _CellHandler CellHandler;
typedef struct _CellHandlerClass CellHandlerClass;

struct _CellHandler {
    GObject parent_instance;
};

struct _CellHandlerClass {
    GObjectClass parent_class;
};

GType cell_handler_get_type(void);

typedef enum {
    CELL_PROGRESS_NONE = 0,
    CELL_GET_POSITION_ON = 1 << 0,
    CELL_START_TRACKING_ON = 1 << 1,
    CELL_LOCATION_UPDATES_ON = 1 << 2
} CellHandlerStateFlags;

G_END_DECLS

#endif /* _CELL_HANDLER_H_ */
