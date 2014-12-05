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
 * Filename  : Celld_Handler.c
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
#include <Cell_Handler.h>
#include <Plugin_Loader.h>
#include <LocationService_Log.h>
#include <pbnjson.h>

typedef struct _CellHandlerPrivate {
    CellPlugin *cell_plugin;
    gboolean is_started;

    PositionCallback pos_cb;

    StartTrackingCallBack track_cb;
    StartTrackingCallBack loc_update_cb;

    gpointer nwhandler;
    gint api_progress_flag;
    LSHandle *sh;
    LSMessage *message;
    LSMessageToken m_cellInfoReq;
    int susbcribe;
    GMutex mutex;
    void *server_status_cookie;
    Position *tracking_pos;
    Accuracy *tracking_acc;
} CellHandlerPrivate;

#define CELL_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HANDLER_TYPE_CELL, CellHandlerPrivate))
static void cell_handler_interface_init(HandlerInterface *interface);

G_DEFINE_TYPE_WITH_CODE(CellHandler, cell_handler, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(HANDLER_TYPE_INTERFACE, cell_handler_interface_init));

/**
 * <Funciton >   cell_handler_tracking_cb
 * <Description>  Callback function for Location tracking
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
static void cell_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(privateIns);

    g_return_if_fail(priv);

    //Return pos is NULL 25/10/2013 [START]
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

    if (priv->track_cb != NULL)
        (*(priv->track_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);

     if (priv->loc_update_cb != NULL)
         (*(priv->loc_update_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);

}

/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
static void cell_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_INFO("[DEBUG]cell position_cb callback called  %d \n", enable_cb);
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(privateIns);

    g_return_if_fail(priv);
    g_return_if_fail(priv->pos_cb);

    (*(priv->pos_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type); //call SA position callback

    priv->api_progress_flag &= ~CELL_GET_POSITION_ON;
}

/**
 * <Funciton >   cell_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_start(Handler *handler_data, int handler_type, const char* license_key)
{
    LS_LOG_INFO("[DEBUG]cell_handler_start Called");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(handler_data);
    int ret = ERROR_NONE;

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->cell_plugin, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->cell_plugin->ops.start, ERROR_NOT_AVAILABLE);
    g_return_val_if_fail(priv->cell_plugin->plugin_handler, ERROR_NOT_AVAILABLE);

    g_mutex_lock(&priv->mutex);

    if (priv->is_started == TRUE) {
        g_mutex_unlock(&priv->mutex);
        return ERROR_NONE;
    }

    ret = priv->cell_plugin->ops.start(priv->cell_plugin->plugin_handler, handler_data, license_key);

    if (ret == ERROR_NONE)
        priv->is_started = TRUE;

    g_mutex_unlock(&priv->mutex);

    return ret;
}

/**o
 * <Funciton >   cell_handler_get_last_position
 * <Description>  stop the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_stop(Handler *self, int handler_type, gboolean forcestop)
{
    LS_LOG_INFO("[DEBUG]cell_handler_stop() \n");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;

    LSError lserror;
    LSErrorInit(&lserror);
    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (priv->is_started == FALSE)
        return ERROR_NOT_STARTED;

    if (forcestop == TRUE)
        priv->api_progress_flag = 0;

    if (priv->api_progress_flag != DEFAULT_VALUE)
        return ERROR_REQUEST_INPROGRESS;

    if (priv->is_started == TRUE) {
        g_return_val_if_fail(priv->cell_plugin->ops.stop, ERROR_NOT_AVAILABLE);

        ret = priv->cell_plugin->ops.stop(priv->cell_plugin->plugin_handler);

        if (priv->server_status_cookie && !LSCancelServerStatus(priv->sh, priv->server_status_cookie, &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }

        if (ret == ERROR_NONE) {
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
    int pos_ret = ERROR_NONE;
    int error = ERROR_NONE;
    jvalue_ref cell_data_obj = NULL ;

    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE((Handler *) ctx);

    g_return_if_fail(priv);

    if (priv->is_started == FALSE)
        return true;

    jschema_ref input_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);

    if(!input_schema)
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

    if (error != 0) { // O i Sucess case
        LS_LOG_ERROR("Cell handler:Telephony error Happened %d\n", error);
        goto CLEANUP;
    }

    if (!jobject_get_exists(parsedObj, J_CSTR_TO_BUF("data"), &cell_data_obj)) {
        LS_LOG_ERROR("Cell handler:Telephony no data available\n");
        goto CLEANUP;
    }

    if ((priv->api_progress_flag & CELL_START_TRACKING_ON) ||
        (priv->api_progress_flag & CELL_LOCATION_UPDATES_ON)) {
        LS_LOG_INFO("Cell handler:send start tracking indication\n");
        track_ret = priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler,
                                                          TRUE,
                                                          cell_handler_tracking_cb,
                                                          (gpointer) jvalue_tostring_simple(cell_data_obj));
    }

    if (priv->api_progress_flag & CELL_GET_POSITION_ON) {
        pos_ret = priv->cell_plugin->ops.get_position(priv->cell_plugin->plugin_handler,
                                                      cell_handler_position_cb,
                                                      (gpointer) jvalue_tostring_simple(cell_data_obj));
    }

    if (track_ret != ERROR_NONE) {
        priv->api_progress_flag &= ~CELL_START_TRACKING_ON;
        priv->api_progress_flag &= ~CELL_LOCATION_UPDATES_ON;
    }

    if (pos_ret == ERROR_NOT_AVAILABLE)
        priv->api_progress_flag &= ~CELL_GET_POSITION_ON;

CLEANUP:

    if (!jis_null(parsedObj))
        j_release(&parsedObj);

    jschema_release(&input_schema);

    return true;
}

static bool tel_service_status_cb(LSHandle *sh, const char *serviceName, bool connected, void *ctx) {
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(ctx);

    if (connected) {
        LS_LOG_DEBUG("Cell handler:Telephony  service Name = %s connected",serviceName);
        if (priv->susbcribe) {
            // cuurnetly only on method later of we can request other method for subscription
            return LSCall(sh, "palm://com.palm.telephony/getCellLocUpdate", "{\"subscribe\":true}", cell_data_cb, ctx, &priv->m_cellInfoReq, NULL);
        }

        return LSCall(sh, "palm://com.palm.telephony/getCellLocUpdate", "{}", cell_data_cb, ctx, NULL, NULL);
        //request_cell_data(sh, ctx, TRUE);
    } else {
        if (priv->api_progress_flag & CELL_GET_POSITION_ON) {
            g_return_if_fail(priv->pos_cb);

            (*(priv->pos_cb))(TRUE , NULL, NULL, ERROR_NETWORK_ERROR, priv->nwhandler, HANDLER_CELLID);//call SA position callback
            priv->api_progress_flag &= ~CELL_GET_POSITION_ON;
        }
        LS_LOG_DEBUG("Cell handler:Telephony service Name = %s disconnected",serviceName);
    }
    return true;
}

static gboolean request_cell_data(LSHandle *sh, gpointer self, int subscribe)
{
    LSError lserror;
    LSErrorInit(&lserror);
    gboolean result;

    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, FALSE);

    priv->susbcribe = subscribe;

    result = LSRegisterServerStatusEx(sh,
                                      "com.palm.telephony",
                                      tel_service_status_cb,
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

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from cell receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static int cell_handler_get_position(Handler *self, gboolean enable, PositionCallback pos_cb, gpointer handler, int handlertype, LSHandle *sh)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_get_position\n");
    int result = ERROR_NONE;
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);

    g_return_val_if_fail(priv, ERROR_NOT_AVAILABLE);

    if (enable) {
        if (priv->api_progress_flag & CELL_GET_POSITION_ON)
            return ERROR_DUPLICATE_REQUEST;

        priv->sh = sh;
        priv->pos_cb = pos_cb;
        priv->nwhandler = handler;
        LS_LOG_DEBUG("[DEBUG]cell_handler_get_position value of newof hanlder %u\n", priv->nwhandler);

        if (request_cell_data(sh, self, FALSE) == TRUE) {
            priv->api_progress_flag |= CELL_GET_POSITION_ON;
        } else {
            result = ERROR_NOT_AVAILABLE;
        }
    }

    LS_LOG_INFO("[DEBUG]result : cell_handler_get_position %d  \n", result);

    return result;
}

/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void cell_handler_start_tracking(Handler *self,
                                        gboolean enable,
                                        StartTrackingCallBack track_cb,
                                        gpointer handlerobj,
                                        int handlertype,
                                        LSHandle *sh,
                                        LSMessage *msg)
{
    LS_LOG_INFO("[DEBUG]Cell handler start Tracking called ");
    gboolean mRet = false;
    LSError lserror;
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);

    if (priv == NULL || priv->cell_plugin->ops.start_tracking == NULL) {
        if (track_cb)
            (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_CELLID);

        return;
    }

    if (enable && !track_cb) {
        return;
    }

    priv->nwhandler = handlerobj;

    if (enable) {
        if (priv->api_progress_flag & CELL_START_TRACKING_ON) {
            if (priv->tracking_pos && priv->tracking_acc)
                (*track_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_CELLID);

            return;
        }
        priv->sh = sh;
        priv->track_cb = track_cb;

        if (request_cell_data(sh, self, TRUE) == TRUE) {
            priv->api_progress_flag |= CELL_START_TRACKING_ON;
        } else {
            (*track_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, priv->nwhandler, HANDLER_CELLID);
        }
    } else {
        if (!(priv->api_progress_flag & CELL_LOCATION_UPDATES_ON)) {
            int result = priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler, FALSE, NULL, NULL);
            priv->api_progress_flag &= ~CELL_START_TRACKING_ON;
            LS_LOG_DEBUG("LSCancel");

            if (priv->m_cellInfoReq != DEFAULT_VALUE)
                mRet = LSCallCancel(priv->sh, priv->m_cellInfoReq, &lserror);

            priv->m_cellInfoReq = DEFAULT_VALUE;

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
        priv->api_progress_flag &= ~CELL_START_TRACKING_ON;

        LS_LOG_DEBUG("[DEBUG] LSCallCancel return from cell_handler_start_tracking   %d \n", mRet);
    }
}


/**
 * <Funciton >   handler_stop
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void cell_handler_get_location_updates(Handler *self,
                                              gboolean enable,
                                              StartTrackingCallBack loc_update_cb,
                                              gpointer handlerobj,
                                              int handlertype,
                                              LSHandle *sh,
                                              LSMessage *msg)
{
    LS_LOG_INFO("[DEBUG]Cell handler get location updates called ");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);
    LSError lserror;
    gboolean mRet = FALSE;

    if (priv == NULL || priv->cell_plugin->ops.start_tracking == NULL) {
        if (loc_update_cb)
            (*loc_update_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_CELLID);

        return;
    }

    if (enable && !loc_update_cb)  {
        return;
    }

    priv->nwhandler = handlerobj;

    if (enable) {
        if (priv->api_progress_flag & CELL_LOCATION_UPDATES_ON) {

            if (priv->tracking_pos && priv->tracking_acc)
                (*loc_update_cb)(TRUE, priv->tracking_pos, priv->tracking_acc, ERROR_NONE, handlerobj, HANDLER_CELLID);

            return;
        }
        priv->sh = sh;
        priv->loc_update_cb = loc_update_cb;

        if ((!(priv->api_progress_flag & CELL_START_TRACKING_ON))) {
            if (request_cell_data(sh, self, TRUE) == TRUE) {
                priv->api_progress_flag |= CELL_LOCATION_UPDATES_ON;
            } else {
                (*loc_update_cb)(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, priv->nwhandler, HANDLER_CELLID);
            }
        } else {
            priv->api_progress_flag |= CELL_LOCATION_UPDATES_ON;
        }
    } else {
        if ((!(priv->api_progress_flag & CELL_START_TRACKING_ON))) {

            int result = priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler,
                                                               FALSE,
                                                               NULL,
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
            if (priv->m_cellInfoReq != DEFAULT_VALUE)
                mRet = LSCallCancel(priv->sh, priv->m_cellInfoReq, &lserror);

            priv->m_cellInfoReq = DEFAULT_VALUE;
        }
        priv->loc_update_cb = NULL;
        priv->api_progress_flag &= ~CELL_LOCATION_UPDATES_ON;
        LS_LOG_DEBUG("[DEBUG] LSCallCancel return from cell_handler_start_tracking   %d \n", mRet);
    }
}


/**
 * <Funciton >   handler_stop
 * <Description>  get the last position from the GPS handler
 * @param     <self> <In> <Handler Gobject>
 * @param     <LastPositionCallback> <In> <Callabck to to get the result>
 * @return    int
 */
static int cell_handler_get_last_position(Handler *self, Position *position, Accuracy *accuracy)
{
    if (get_stored_position(position, accuracy, LOCATION_DB_PREF_PATH_CELL) == ERROR_NOT_AVAILABLE) {
        LS_LOG_ERROR("Cell get last position Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    LS_LOG_INFO("cell get last poistion success\n");

    return ERROR_NONE;
}

/**
 * <Funciton >   cell_handler_dispose
 * <Description>  dispose the gobject
 * @param     <gobject> <In> <Gobject>
 * @return    int
 */
static void cell_handler_dispose(GObject *gobject)
{
    G_OBJECT_CLASS(cell_handler_parent_class)->dispose(gobject);
}

/**
 * <Funciton >   cell_handler_finalize
 * <Description>  finalize
 * @param     <gobject> <In> <Handler Gobject>
 * @return    int
 */
static void cell_handler_finalize(GObject *gobject)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_finalize");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(gobject);

    g_return_if_fail(priv);

    if (priv->cell_plugin != NULL) {
        plugin_free(priv->cell_plugin, "cell");
        priv->cell_plugin = NULL;
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

    memset(priv, 0x00, sizeof(CellHandlerPrivate));

    G_OBJECT_CLASS(cell_handler_parent_class)->finalize(gobject);
}

static int cell_handler_function_not_implemented(Handler *self, Position *pos, Address *address)
{
    return ERROR_NOT_APPLICABLE_TO_THIS_HANDLER;
}

/**
 * <Funciton >   cell_handler_interface_init
 * <Description>  cell interface init,map all th local function to GPS interface
 * @param     <HandlerInterface> <In> <HandlerInterface instance>
 * @return    int
 */
static void cell_handler_interface_init(HandlerInterface *interface)
{
    memset(interface, 0, sizeof(HandlerInterface));

    interface->start = (TYPE_START_FUNC) cell_handler_start;
    interface->stop = (TYPE_STOP_FUNC) cell_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) cell_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) cell_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) cell_handler_get_last_position;
    interface->get_location_updates = (TYPE_GET_LOCATION_UPDATES) cell_handler_get_location_updates;
}

/**
 * <Funciton >   cell_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void cell_handler_init(CellHandler *self)
{
    LS_LOG_INFO("[DEBUG]cell_handler_init");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);

    g_return_if_fail(priv);

    memset(priv, 0x00, sizeof(CellHandlerPrivate));
    priv->cell_plugin = (CellPlugin *) plugin_new("cell");

    if (priv->cell_plugin == NULL) {
        LS_LOG_WARNING("[DEBUG]cell plugin loading failed\n");
    }

    g_mutex_init(&priv->mutex);
}

/**
 * <Funciton >   cell_handler_class_init
 * <Description>  Gobject class init
 * @param     <CellHandlerClass> <In> <CellClass instance>
 * @return    int
 */
static void cell_handler_class_init(CellHandlerClass *klass)
{
    LS_LOG_INFO("[DEBUG]cell_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = cell_handler_dispose;
    gobject_class->finalize = cell_handler_finalize;

    g_type_class_add_private(klass, sizeof(CellHandlerPrivate));
}
