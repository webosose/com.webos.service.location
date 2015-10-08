/********************************************************************
 * (c) Copyright 2015. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group   : CSP-2
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Network_Handler.c
 * Purpose  : Provides Network related location services to Location Service Agent
 * Platform  : w2
 * Author(s)  : Deepa Sangolli
 * Email ID.  :deepa.sangolli@lge.com
 * Creation Date : 12-08-2015
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
#include <Network_Handler.h>
#include <Plugin_Loader.h>
#include <Gps_stored_data.h>
#include <loc_log.h>
#include <stdlib.h>
#include <pbnjson.h>

typedef struct _NetworkHandlerPrivate {
    NetworkPlugin *network_plugin;
    gboolean is_started;
    gboolean isNetworkLocationUpdateOn;
    StartTrackingCallBack loc_update_cb;
    gpointer nwhandler;
    LSHandle *sh;
    LSMessage *message;
    LSMessageToken m_cellInfoReq;
    int susbcribe;
    GMutex mutex;
    void *server_status_cookie;
    Position *tracking_pos;
    Accuracy *tracking_acc;
} NetworkHandlerPrivate;

#define NETWORK_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_NETWORK, NetworkHandlerPrivate))

static void network_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(NetworkHandler, network_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, network_handler_interface_init));

static void network_handler_tracking_cb(gboolean enable_cb,
                                        Position *position,
                                        Accuracy *accuracy,
                                        int error,
                                        gpointer userdata,
                                        int type)
{
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(userdata);
    g_return_if_fail(priv);

    g_return_if_fail(position);
    g_return_if_fail(accuracy);

    if (priv->tracking_pos == NULL)
        priv->tracking_pos = (Position *)g_malloc0(sizeof(Position));

    if (priv->tracking_acc == NULL)
        priv->tracking_acc = (Accuracy *)g_malloc0(sizeof(Accuracy));

    if (priv->tracking_pos) {
        memcpy(priv->tracking_pos, position, sizeof(Position));
        priv->tracking_pos->timestamp = 0;
    }

    if (priv->tracking_acc) {
        memcpy(priv->tracking_acc, accuracy, sizeof(Accuracy));
    }

    if (priv->loc_update_cb)
        (*(priv->loc_update_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);
}


static int network_handler_start(Handler *handler_data, const char* license_key)
{
    LS_LOG_INFO("network_handler_start Called\n");
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;

    if(!priv || !priv->network_plugin || !priv->network_plugin->ops.start || !priv->network_plugin->plugin_handler){
        return ERROR_NOT_AVAILABLE;
    }

    if (priv->is_started) {
        return ERROR_NONE;
    }

    ret = priv->network_plugin->ops.start(priv->network_plugin->plugin_handler, handler_data, license_key);

    if (!ret)
        priv->is_started = TRUE;

    return ret;
}

static int network_handler_stop(Handler *self, gboolean forcestop)
{
    LS_LOG_INFO("network_handler_stop() \n");
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;
    LSError lserror;
    LSErrorInit(&lserror);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (!priv->is_started)
        return ERROR_NOT_STARTED;

    if (priv->isNetworkLocationUpdateOn)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->m_cellInfoReq) {
        if (!LSCallCancel(priv->sh, priv->m_cellInfoReq, &lserror)) {
            LS_LOG_ERROR("Failed to cancel getCellInfo subscription\n");
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
        priv->m_cellInfoReq = DEFAULT_VALUE;
    }

    if (priv->is_started) {
        g_return_val_if_fail(priv->network_plugin->ops.stop, ERROR_NOT_AVAILABLE);
        ret = priv->network_plugin->ops.stop(priv->network_plugin->plugin_handler);
        if (priv->server_status_cookie && !LSCancelServerStatus(priv->sh, priv->server_status_cookie, &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
        if (!ret) {
            priv->is_started = FALSE;
        }
    }
    return ret;
}
static bool cell_data_cb(LSHandle *sh, LSMessage *reply, void *ctx)
{
    jvalue_ref parsedObj = NULL;
    jvalue_ref error_obj = NULL;
    int track_ret = ERROR_NONE;
    int error = ERROR_NONE;
    jvalue_ref cell_data_obj = NULL ;

    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE((Handler *) ctx);
    g_return_if_fail(priv);

    if (priv->is_started == FALSE)
        return true;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
    if (!input_schema)
        return true;

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, input_schema, NULL, NULL);
    parsedObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(reply)), DOMOPT_NOOPT, &schemaInfo);
    LS_LOG_DEBUG("message=%s \n \n", LSMessageGetPayload(reply));

    if (jis_null(parsedObj)) {
        jschema_release(&input_schema);
        return true;
    }

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("errorCode"), &error_obj))
        goto CLEANUP;

    jnumber_get_i32(error_obj, &error);

    if (error != ERROR_NONE) { // O i Sucess case
        LS_LOG_ERROR("network handler:Telephony error Happened %d\n", error);
        goto CLEANUP;
    }

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("data"), &cell_data_obj)) {
        LS_LOG_ERROR("network handler:Telephony no data available\n");
        goto CLEANUP;
    }

    if (priv->isNetworkLocationUpdateOn) {
        LS_LOG_INFO("NW handler:send start tracking indication\n");
        track_ret = priv->network_plugin->ops.start_tracking(priv->network_plugin->plugin_handler,
                                                             TRUE,
                                                             network_handler_tracking_cb,
                                                             (gpointer) jvalue_tostring_simple(cell_data_obj));
    }
    if (track_ret != ERROR_NONE)
        priv->isNetworkLocationUpdateOn = false;
CLEANUP:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    jschema_release(&input_schema);

    return true;
}

static bool telephony_service_status_cb(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(ctx);
    LSError lserror;
    LSErrorInit(&lserror);
    gboolean result;

    if (connected) {
        result = LSCall(sh, "palm://com.webos.service.telephony/getCellInfo", "{\"subscribe\":true}", cell_data_cb, ctx, &priv->m_cellInfoReq, NULL);
        LS_LOG_DEBUG("nw handler:Telephony  service Name = %s connected\n",serviceName);
        if (!result) {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
            return FALSE;
        }
    } else {
        if (priv->isNetworkLocationUpdateOn) {
            g_return_val_if_fail(priv->loc_update_cb, FALSE);

            (*(priv->loc_update_cb))(TRUE , NULL, NULL, ERROR_NETWORK_ERROR, priv->nwhandler, HANDLER_NETWORK);//call SA position callback
            priv->isNetworkLocationUpdateOn = false;
        }
        LS_LOG_DEBUG("nw handler:Telephony service Name = %s disconnected",serviceName);
    }
    return TRUE;
}

static gboolean register_telephony_service_status(LSHandle *sh, gpointer self, int subscribe)
{
    LSError lserror;
    LSErrorInit(&lserror);
    gboolean result;

    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, FALSE);


    result = LSRegisterServerStatusEx(sh,
                                      "com.webos.service.telephony",
                                      telephony_service_status_cb,
                                      self,
                                      &(priv->server_status_cookie),
                                      &lserror);
    if (!result) {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
        return FALSE;
    }
    return TRUE;
}

gboolean network_handler_get_handler_status(Handler *self)
{
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, FALSE);
    return priv->is_started;
}

static void network_handler_get_location_updates(Handler *self,
                                                 gboolean enable,
                                                 StartTrackingCallBack loc_update_cb,
                                                 gpointer handlerobj,
                                                 LSHandle *sh)
{
    int result = ERROR_NONE;
    gboolean cell_data_result = FALSE;
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(self);

    if (enable && !loc_update_cb)
        return;

    LS_LOG_INFO("network_handler_get_location_updates status %d ",enable);

    if (!priv  || !(priv->network_plugin->ops.start_tracking)) {
        if (loc_update_cb)
            (*loc_update_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_NETWORK);
        return;
    }

    if (enable) {
        if (priv->isNetworkLocationUpdateOn) {
            if (priv->tracking_pos && priv->tracking_acc)
                (*loc_update_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_NETWORK);
            return;
        }

        priv->loc_update_cb = loc_update_cb;
        priv->nwhandler = handlerobj;
        priv->sh = sh;

        if (!priv->isNetworkLocationUpdateOn) {
            cell_data_result = register_telephony_service_status(sh, self, TRUE);
            result = priv->network_plugin->ops.start_tracking(priv->network_plugin->plugin_handler,
                                                              TRUE,
                                                              network_handler_tracking_cb,NULL);

            if ((cell_data_result)|| (!result))
                priv->isNetworkLocationUpdateOn = true;
            else
                loc_update_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_NETWORK);
        }
    } else {
        LS_LOG_INFO("network_handler_get_location_updates stop tracking\n");
        result = priv->network_plugin->ops.start_tracking(priv->network_plugin->plugin_handler,
                                                          FALSE,
                                                          NULL,NULL);

        if (!result) {
            if (priv->tracking_pos) {
                free(priv->tracking_pos);
                priv->tracking_pos = NULL;
            }

            if (priv->tracking_acc) {
                free(priv->tracking_acc);
                priv->tracking_acc = NULL;
            }
        }

        priv->loc_update_cb = NULL;
        priv->isNetworkLocationUpdateOn = false;
    }
}

static int network_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy)
{
    if (get_stored_position(position, accuracy, LOCATION_DB_PREF_PATH_NETWORK) == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("get last network_handler_get_last_position Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    return ERROR_NONE;
}

static void network_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(network_handler_parent_class)->dispose(gobject);
}

static void network_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]network_handler_finalize\n");
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(gobject);

    if (priv->network_plugin) {
        plugin_free(priv->network_plugin, "network");
        priv->network_plugin = NULL;
    }

    if (priv->tracking_pos) {
        free(priv->tracking_pos);
        priv->tracking_pos = NULL;
    }

    if (priv->tracking_acc) {
        free(priv->tracking_acc);
        priv->tracking_acc = NULL;
    }

    memset(priv, 0x00, sizeof(NetworkHandlerPrivate));

    G_OBJECT_CLASS(network_handler_parent_class)->finalize(gobject);
}

static void network_handler_interface_init(HandlerInterface *interface)
{
    memset(interface, 0, sizeof(HandlerInterface));

    interface->start = network_handler_start;
    interface->stop = network_handler_stop;
    interface->get_last_position = network_handler_get_last_position;
    interface->get_location_updates = network_handler_get_location_updates;
    interface->get_handler_status = network_handler_get_handler_status;
}

static void network_handler_init(NetworkHandler *self)
{
    LS_LOG_INFO("nw_handler_init\n");
    NetworkHandlerPrivate *priv = NETWORK_HANDLER_GET_PRIVATE(self);

    memset(priv, 0x00, sizeof(NetworkHandlerPrivate));
    priv->network_plugin = (NetworkPlugin *) plugin_new("network");

    if (!priv->network_plugin) {
        LS_LOG_ERROR("[DEBUG]nw plugin loading failed\n");
    }
}

static void network_handler_class_init(NetworkHandlerClass *klass)
{
    LS_LOG_INFO("network_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = network_handler_dispose;
    gobject_class->finalize = network_handler_finalize;

    g_type_class_add_private(klass, sizeof(NetworkHandlerPrivate));
}
