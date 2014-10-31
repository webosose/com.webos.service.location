/********************************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group   : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Wifi_Handler.c
 * Purpose  : Provides GPS related location services to Location Service Agent
 * Platform  : RedRose
 * Author(s)  : Abhishek Srivastava
 * Email ID.  :abhishek.srivastava@lge.com
 * Creation Date : 12-04-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Handler_Interface.h>
#include <Wifi_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>

typedef struct _WifiHandlerPrivate {
    WifiPlugin *wifi_plugin;
    gboolean is_started;

    PositionCallback pos_cb;
    StartTrackingCallBack track_cb;
    StartTrackingCallBack track_criteria_cb;
    StartTrackingCallBack loc_update_cb;

    gpointer nwhandler;
    gint api_progress_flag;
    GMutex mutex;

    Position *tracking_pos;
    Accuracy *tracking_acc;
} WifiHandlerPrivate;

#define WIFI_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_WIFI, WifiHandlerPrivate))

static void wifi_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(WifiHandler, wifi_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, wifi_handler_interface_init));





static void wifi_handler_tracking_cb(gboolean enable_cb,
                                     Position *position,
                                     Accuracy *accuracy,
                                     int error,
                                     gpointer userdata,
                                     int type)
{
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(userdata);
    g_return_if_fail(priv);

    g_return_if_fail(position);
    g_return_if_fail(accuracy);

    if (priv->tracking_pos == NULL)
        priv->tracking_pos = (Position *)malloc(sizeof(Position));

    if (priv->tracking_acc == NULL)
        priv->tracking_acc = (Accuracy *)malloc(sizeof(Accuracy));

    if (priv->tracking_pos) {
        memcpy(priv->tracking_pos, position, sizeof(Position));
        priv->tracking_pos->timestamp = 0;      // means cached position
    }

    if (priv->tracking_acc) {
        memcpy(priv->tracking_acc, accuracy, sizeof(Accuracy));
    }

    if (priv->track_cb)
        (*(priv->track_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);

    if (priv->track_criteria_cb)
        (*(priv->track_criteria_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);

    if (priv->loc_update_cb)
        (*(priv->loc_update_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);
}

static void wifi_handler_position_cb(gboolean enable_cb,
                                     Position *position,
                                     Accuracy *accuracy,
                                     int error,
                                     gpointer userdata,
                                     int type)
{
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(userdata);
    g_return_if_fail(priv);

    g_return_if_fail(position);
    g_return_if_fail(accuracy);

    if (priv->pos_cb)
        (*(priv->pos_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);
}



static int wifi_handler_start(Handler *handler_data, int handler_type, const char* license_key)
{
    LS_LOG_INFO("[DEBUG]wifi_handler_start Called\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->wifi_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    g_mutex_lock(&priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(&priv->mutex);
        return ERROR_NONE;
    }

    ret = priv->wifi_plugin->ops.start(priv->wifi_plugin->plugin_handler, handler_data, license_key);

    if (ret == ERROR_NONE)
        priv->is_started = TRUE;

    g_mutex_unlock(&priv->mutex);

    return ret;
}

static int wifi_handler_stop(Handler *self, int handler_type, gboolean forcestop)
{
    LS_LOG_INFO("[DEBUG]wifi_handler_stop() \n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE)
        priv->api_progress_flag = 0;

    if (priv->api_progress_flag != 0)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->is_started == TRUE) {
        g_return_val_if_fail(priv->wifi_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->wifi_plugin->ops.stop(priv->wifi_plugin->plugin_handler);

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

static int wifi_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handler)
{
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable) {
        if (priv->api_progress_flag & WIFI_GET_POSITION_ON)
            return ERROR_DUPLICATE_REQUEST;

        g_return_val_if_fail(pos_cb, ERROR_NOT_AVAILABLE);

        priv->pos_cb = pos_cb;
        priv->nwhandler = handler;

        g_return_val_if_fail(priv->wifi_plugin->ops.get_position, ERROR_NOT_AVAILABLE);

        LS_LOG_INFO("[DEBUG]wifi_handler_get_position\n");
        result = priv->wifi_plugin->ops.get_position(priv->wifi_plugin->plugin_handler, wifi_handler_position_cb);

        if (result == ERROR_NONE)
            priv->api_progress_flag |= WIFI_GET_POSITION_ON;

    } else {
        // Clear the Flag
        priv->api_progress_flag &= ~WIFI_GET_POSITION_ON;
        LS_LOG_DEBUG("[DEBUG]result : wifi_handler_get_position Clearing Position\n");
    }

    return result;
}

static void wifi_handler_start_tracking(Handler *self,
                                        gboolean enable,
                                        StartTrackingCallBack track_cb,
                                        gpointer handlerobj)
{
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    if (priv == NULL || priv->wifi_plugin->ops.start_tracking == NULL) {
        if (track_cb)
            (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);

        return;
    }

    if (enable && !track_cb)
        return;

    if (enable) {
        if (priv->api_progress_flag & WIFI_START_TRACKING_ON) {
            if (priv->tracking_pos && priv->tracking_acc)
                (*track_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_WIFI);

            return;
        }

        priv->track_cb = track_cb;
        priv->nwhandler = handlerobj;

        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_CRITERIA_ON)) &&
            (!(priv->api_progress_flag & WIFI_GET_LOCATION_UPDATES_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           TRUE,
                                                           wifi_handler_tracking_cb);

            if (result == ERROR_NONE)
                priv->api_progress_flag |= WIFI_START_TRACKING_ON;
            else
                (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);
        }
    } else {
        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_CRITERIA_ON)) &&
            (!(priv->api_progress_flag & WIFI_GET_LOCATION_UPDATES_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           FALSE,
                                                           NULL);

            if (result == ERROR_NONE) {
                if (priv->tracking_pos) {
                    free(priv->tracking_pos);
                    priv->tracking_pos = NULL;
                }

                if (priv->tracking_acc) {
                    free(priv->tracking_acc);
                    priv->tracking_acc = NULL;
                }
            }
        }

        priv->track_cb = NULL;
        priv->api_progress_flag &= ~WIFI_START_TRACKING_ON;
    }

    LS_LOG_INFO("[DEBUG] return from wifi_handler_start_tracking , %d  \n", result);
}

static void wifi_handler_start_tracking_criteria(Handler *self,
                                                 gboolean enable,
                                                 StartTrackingCallBack track_cb,
                                                 gpointer handlerobj)
{
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    if (priv == NULL || priv->wifi_plugin->ops.start_tracking == NULL) {
        if (track_cb)
            (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);

        return;
    }

    if (enable && !track_cb)
        return;

    if (enable) {
        if (priv->api_progress_flag & WIFI_START_TRACKING_CRITERIA_ON) {
            if (priv->tracking_pos && priv->tracking_acc)
                (*track_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_WIFI);

            return;
        }

        priv->track_criteria_cb = track_cb;
        priv->nwhandler = handlerobj;

        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_ON)) &&
            (!(priv->api_progress_flag & WIFI_GET_LOCATION_UPDATES_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           TRUE,
                                                           wifi_handler_tracking_cb);

            if (result == ERROR_NONE)
                priv->api_progress_flag |= WIFI_START_TRACKING_CRITERIA_ON;
            else
                (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);
        }
    } else {
        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_ON)) &&
            (!(priv->api_progress_flag & WIFI_GET_LOCATION_UPDATES_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           FALSE,
                                                           NULL);

            if (result == ERROR_NONE) {
                if (priv->tracking_pos) {
                    free(priv->tracking_pos);
                    priv->tracking_pos = NULL;
                }

                if (priv->tracking_acc) {
                    free(priv->tracking_acc);
                    priv->tracking_acc = NULL;
                }
            }
        }

        priv->track_criteria_cb = NULL;
        priv->api_progress_flag &= ~WIFI_START_TRACKING_CRITERIA_ON;
    }
}

static void wifi_handler_get_location_updates(Handler *self,
                                              gboolean enable,
                                              StartTrackingCallBack loc_update_cb,
                                              gpointer handlerobj)
{
    int result = ERROR_NONE;
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    if (priv == NULL || priv->wifi_plugin->ops.start_tracking == NULL) {
        if (loc_update_cb)
            (*loc_update_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);

        return;
    }

    if (enable && !loc_update_cb)
        return;

    if (enable) {
        if (priv->api_progress_flag & WIFI_GET_LOCATION_UPDATES_ON) {
            if (priv->tracking_pos && priv->tracking_acc)
                (*loc_update_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_WIFI);

            return;
        }

        priv->loc_update_cb = loc_update_cb;
        priv->nwhandler = handlerobj;

        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_ON)) &&
            (!(priv->api_progress_flag & WIFI_START_TRACKING_CRITERIA_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           TRUE,
                                                           wifi_handler_tracking_cb);

            if (result == ERROR_NONE)
                priv->api_progress_flag |= WIFI_GET_LOCATION_UPDATES_ON;
            else
                loc_update_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_WIFI);
        }
    } else {
        if ((!(priv->api_progress_flag & WIFI_START_TRACKING_ON)) &&
            (!(priv->api_progress_flag & WIFI_START_TRACKING_CRITERIA_ON))) {
            result = priv->wifi_plugin->ops.start_tracking(priv->wifi_plugin->plugin_handler,
                                                           FALSE,
                                                           NULL);

            if (result == ERROR_NONE) {
                if (priv->tracking_pos) {
                    free(priv->tracking_pos);
                    priv->tracking_pos = NULL;
                }

                if (priv->tracking_acc) {
                    free(priv->tracking_acc);
                    priv->tracking_acc = NULL;
                }
            }
        }

        priv->track_criteria_cb = NULL;
        priv->api_progress_flag &= ~WIFI_GET_LOCATION_UPDATES_ON;
    }
}

static int wifi_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy)
{
    if (get_stored_position(position, accuracy, LOCATION_DB_PREF_PATH_WIFI) == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("get last wifi_handler_get_last_position Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

static void wifi_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(wifi_handler_parent_class)->dispose(gobject);
}

static void wifi_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]wifi_handler_finalize\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(gobject);

    if (priv->wifi_plugin != NULL) {
        plugin_free(priv->wifi_plugin, "wifi");
        priv->wifi_plugin = NULL;
    }

    if (priv->tracking_pos) {
        free(priv->tracking_pos);
        priv->tracking_pos = NULL;
    }

    if (priv->tracking_acc) {
        free(priv->tracking_acc);
        priv->tracking_acc = NULL;
    }

    g_mutex_clear(&priv->mutex);

    memset(priv, 0x00, sizeof(WifiHandlerPrivate));

    G_OBJECT_CLASS(wifi_handler_parent_class)->finalize(gobject);
}

static void wifi_handler_interface_init(HandlerInterface *interface)
{
    memset(interface, 0, sizeof(HandlerInterface));

    interface->start = (TYPE_START_FUNC) wifi_handler_start;
    interface->stop = (TYPE_STOP_FUNC) wifi_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) wifi_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) wifi_handler_start_tracking;
    interface->start_tracking_criteria = (TYPE_START_TRACK_CRITERIA) wifi_handler_start_tracking_criteria;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) wifi_handler_get_last_position;
    interface->get_location_updates = (TYPE_GET_LOCATION_UPDATES) wifi_handler_get_location_updates;
}

static void wifi_handler_init(WifiHandler *self)
{
    LS_LOG_INFO("[DEBUG]wifi_handler_init\n");
    WifiHandlerPrivate *priv = WIFI_HANDLER_GET_PRIVATE(self);

    memset(priv, 0x00, sizeof(WifiHandlerPrivate));
    priv->wifi_plugin = (WifiPlugin *) plugin_new("wifi");

    if (priv->wifi_plugin == NULL) {
        LS_LOG_ERROR("[DEBUG]wifi plugin loading failed\n");
    }

    g_mutex_init(&priv->mutex);
}

static void wifi_handler_class_init(WifiHandlerClass *klass)
{
    LS_LOG_INFO("[DEBUG]wifi_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = wifi_handler_dispose;
    gobject_class->finalize = wifi_handler_finalize;

    g_type_class_add_private(klass, sizeof(WifiHandlerPrivate));
}
