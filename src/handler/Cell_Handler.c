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
#include <cjson/json.h>
#include <LocationService_Log.h>
typedef struct _CellHandlerPrivate {
    CellPlugin *cell_plugin;
    gboolean is_started;

    PositionCallback pos_cb;

    StartTrackingCallBack track_cb;

    gpointer nwhandler;
    gint api_progress_flag;
    LSHandle *sh;
    LSMessage *message;
    LSMessageToken m_cellInfoReq;
    GMutex mutex;
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
void cell_handler_tracking_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(privateIns);
    g_return_if_fail(priv);
    g_return_if_fail(priv->track_cb);
    //Return pos is NULL 25/10/2013 [START]
    g_return_if_fail(position);
    g_return_if_fail(accuracy);
    //Return pos is NULL 25/10/2013 [END]
    (*(priv->track_cb))(enable_cb, position, accuracy, error, priv->nwhandler, type);
}
/**
 * <Funciton >   position_cb
 * <Description>  Callback function for Position ,as per Location_Plugin.h
 * @param     <Position> <In> <Position data stricture>
 * @param     <Accuracy> <In> <Accuracy data structure>
 * @param     <privateIns> <In> <instance of handler>
 * @return    int
 */
void cell_handler_position_cb(gboolean enable_cb, Position *position, Accuracy *accuracy, int error, gpointer privateIns, int type)
{
    LS_LOG_DEBUG("[DEBUG]cell  haposition_cb callback called  %d \n", enable_cb);
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(privateIns);
    g_return_if_fail(priv);
    g_return_if_fail(priv->pos_cb);
    (*priv->pos_cb)(enable_cb, position, accuracy, error, priv->nwhandler, type); //call SA position callback
    priv->api_progress_flag &= ~CELL_GET_POSITION_ON;
}
/**
 * <Funciton >   cell_handler_start
 * <Description>  start the GPS  handler
 * @param     <self> <In> <Handler Gobject>
 * @return    int
 */
static int cell_handler_start(Handler *handler_data)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_start Called");
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

    ret = priv->cell_plugin->ops.start(priv->cell_plugin->plugin_handler, handler_data);

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
    LS_LOG_DEBUG("[DEBUG]cell_handler_stop() \n");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);
    int ret = ERROR_NONE;
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

        if (ret == ERROR_NONE) {
            priv->is_started = FALSE;
        }
    }

    return ret;
}

static gboolean cell_data_cb(LSHandle *sh, LSMessage *reply, void *ctx)
{
    LS_LOG_DEBUG("cell_data_cb \n");
    struct json_object *new_obj = NULL;
    struct json_object *return_value = NULL;
    struct json_object *error_code = NULL;
    struct json_object *cid_obj = NULL;
    int cid;
    int track_ret = ERROR_NONE;
    int pos_ret = ERROR_NONE;
    int error = ERROR_NONE;
    gboolean ret;
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE((Handler *) ctx);
    g_return_if_fail(priv);

    if (priv->is_started == FALSE)
        return FALSE;

    /*Creating a json array*/
    struct json_object *cell_data_obj ;
    const char *str = LSMessageGetPayload(reply);
    LS_LOG_DEBUG("message=%s \n \n", str);
    // str ="{\"act\":1,\"plmn\": 40486}";
    LS_LOG_DEBUG("read root\n");
    new_obj = json_tokener_parse(str);

    if (new_obj == NULL) {
        return FALSE;
    }

    LS_LOG_DEBUG("new_obj value %d\n", new_obj);
    return_value = json_object_object_get(new_obj, "returnValue");
    ret = json_object_get_boolean(return_value);

    if (ret == FALSE) { // For time being // failure case when telephony return error
        error_code = json_object_object_get(new_obj, "errorCode");
        error = json_object_get_int(error_code);

        if (error == -1) {
            if (priv->api_progress_flag & CELL_START_TRACKING_ON) {
                request_cell_data(sh, ctx, TRUE);
            }

            if (priv->api_progress_flag & CELL_GET_POSITION_ON) {
                g_return_if_fail(priv->pos_cb);
                (*priv->pos_cb)(TRUE , NULL, NULL, ERROR_NETWORK_ERROR, priv->nwhandler, HANDLER_CELLID);//call SA position callback
                priv->api_progress_flag &= ~CELL_GET_POSITION_ON;
            }
        }

        json_object_put(new_obj);
        return FALSE;
    }

    cell_data_obj = json_object_object_get(new_obj, "data");

    if (priv->api_progress_flag & CELL_START_TRACKING_ON) {
        LS_LOG_DEBUG("send start tracking indication\n");
        track_ret = priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler, TRUE, cell_handler_tracking_cb,
                    (gpointer) json_object_to_json_string(cell_data_obj));
    }

    if (priv->api_progress_flag & CELL_GET_POSITION_ON) {
        pos_ret = priv->cell_plugin->ops.get_position(priv->cell_plugin->plugin_handler, cell_handler_position_cb,
                  (gpointer) json_object_to_json_string(cell_data_obj));
    }

    json_object_put(new_obj);

    if (track_ret == ERROR_NOT_AVAILABLE)
        priv->api_progress_flag &= ~CELL_START_TRACKING_ON;

    if (pos_ret == ERROR_NOT_AVAILABLE)
        priv->api_progress_flag &= ~CELL_GET_POSITION_ON;

    return TRUE;
}
gboolean request_cell_data(LSHandle *sh, gpointer self, int subscribe)
{
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);
    g_return_val_if_fail(priv, FALSE);

    if (subscribe) {
        // cuurnetly only on method later of we can request other method for subscription
        return LSCall(sh, "palm://com.palm.telephony/getCellLocUpdate", "{\"update\":true}", cell_data_cb, self, &priv->m_cellInfoReq, NULL);
    }

    return LSCall(sh, "palm://com.palm.telephony/getCellLocUpdate", "{\"update\":false}", cell_data_cb, self, NULL, NULL);
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

        priv->pos_cb = pos_cb;
        priv->nwhandler = handler;
        LS_LOG_DEBUG("[DEBUG]cell_handler_get_position value of newof hanlder %u\n", priv->nwhandler);

        if (request_cell_data(sh, self, FALSE) == TRUE) {
            priv->api_progress_flag |= CELL_GET_POSITION_ON;
        } else {
            result = ERROR_NOT_AVAILABLE;
        }
    }

    LS_LOG_DEBUG("[DEBUG]result : cell_handler_get_position %d  \n", result);
    return result;
}
/**
 * <Funciton >   handler_stop
 * <Description>  get the position from GPS receiver
 * @param     <self> <In> <Handler Gobject>
 * @param     <self> <In> <Position callback function to get result>
 * @return    int
 */
static void cell_handler_start_tracking(Handler *self, gboolean enable, StartTrackingCallBack track_cb,
                                        gpointer handlerobj, int handlertype,
                                        LSHandle *sh, LSMessage *msg)
{
    LS_LOG_DEBUG("[DEBUG]Cell handler start Tracking called ");
    gboolean mRet;
    LSError lserror;
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);

    if (priv == NULL) {
        track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, handlerobj, HANDLER_CELLID);
        return;
    }

    priv->track_cb = NULL;
    priv->nwhandler = handlerobj;

    if (enable) {
        if (priv->api_progress_flag & CELL_START_TRACKING_ON)
            return;

        priv->sh = sh;
        priv->track_cb = track_cb;

        if (request_cell_data(sh, self, TRUE) == TRUE) {
            priv->api_progress_flag |= CELL_START_TRACKING_ON;
        } else
            track_cb(TRUE, NULL, NULL, ERROR_NOT_AVAILABLE, priv->nwhandler, HANDLER_CELLID);
    } else {
        priv->cell_plugin->ops.start_tracking(priv->cell_plugin->plugin_handler, enable, cell_handler_tracking_cb, NULL);
        priv->api_progress_flag &= ~CELL_START_TRACKING_ON;
        LS_LOG_DEBUG("LSCancel");

        if (priv->m_cellInfoReq != DEFAULT_VALUE)
            mRet = LSCallCancel(priv->sh, priv->m_cellInfoReq, &lserror);

        priv->m_cellInfoReq = DEFAULT_VALUE;
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
        LS_LOG_DEBUG("Cell get last poistion Failed to read\n");
        return ERROR_NOT_AVAILABLE;
    }

    LS_LOG_DEBUG("get last poistion success bit\n");
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
    plugin_free(priv->cell_plugin, "cell");
    priv->cell_plugin = NULL;
    g_mutex_clear(&priv->mutex);
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
    interface->start = (TYPE_START_FUNC) cell_handler_start;
    interface->stop = (TYPE_STOP_FUNC) cell_handler_stop;
    interface->get_position = (TYPE_GET_POSITION) cell_handler_get_position;
    interface->start_tracking = (TYPE_START_TRACK) cell_handler_start_tracking;
    interface->get_last_position = (TYPE_GET_LAST_POSITION) cell_handler_get_last_position;
    interface->get_velocity = (TYPE_GET_VELOCITY) cell_handler_function_not_implemented;
    interface->get_last_velocity = (TYPE_GET_LAST_VELOCITY) cell_handler_function_not_implemented;
    interface->get_accuracy = (TYPE_GET_ACCURACY) cell_handler_function_not_implemented;
    interface->get_power_requirement = (TYPE_GET_POWER_REQ) cell_handler_function_not_implemented;
    interface->get_ttfx = (TYPE_GET_TTFF) cell_handler_function_not_implemented;
    interface->get_sat_data = (TYPE_GET_SAT) cell_handler_function_not_implemented;
    interface->get_nmea_data = (TYPE_GET_NMEA) cell_handler_function_not_implemented;
    interface->send_extra_cmd = (TYPE_SEND_EXTRA) cell_handler_function_not_implemented;
    interface->get_cur_handler = (TYPE_GET_CUR_HANDLER) cell_handler_function_not_implemented;
    interface->set_cur_handler = (TYPE_SET_CUR_HANDLER) cell_handler_function_not_implemented;
    interface->compare_handler = (TYPE_COMP_HANDLER) cell_handler_function_not_implemented;
    interface->get_geo_code = (TYPE_GEO_CODE) cell_handler_function_not_implemented;
    interface->get_rev_geocode = (TYPE_REV_GEO_CODE) cell_handler_function_not_implemented;
}

/**
 * <Funciton >   cell_handler_init
 * <Description>  GPS handler init Gobject function
 * @param     <self> <In> <GPS Handler Gobject>
 * @return    int
 */
static void cell_handler_init(CellHandler *self)
{
    LS_LOG_DEBUG("[DEBUG]cell_handler_init");
    CellHandlerPrivate *priv = CELL_HANDLER_GET_PRIVATE(self);
    g_return_if_fail(priv);
    memset(priv, 0x00, sizeof(CellHandlerPrivate));
    priv->cell_plugin = (CellPlugin *) plugin_new("cell");

    if (priv->cell_plugin == NULL) {
        LS_LOG_DEBUG("[DEBUG]cell plugin loading failed\n");
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
    LS_LOG_DEBUG("[DEBUG]cell_handler_class_init() - init object\n");
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = cell_handler_dispose;
    gobject_class->finalize = cell_handler_finalize;
    g_type_class_add_private(klass, sizeof(CellHandlerPrivate));
}
