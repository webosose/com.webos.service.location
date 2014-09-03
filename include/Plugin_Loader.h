/********************************************************************
 * Copyright (c) 2012-2013 LG Electronics Inc. All Rights Reserved
 *
 * Project Name : Location framework
 * Group        : PD-4
 * Security     : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename     : Plugin_Loader.h
 * Purpose      : Utilty to load/unload  plugin so files
 *                All plugins are in /usr/lib/location/plugins/
 *                plugin should implement init & shutdown
 * Platform     : RedRose
 * Author(s)    : Rajesh Gopu I.V, Abhishek Srivastava
 * Email ID.    : rajeshgopu.iv@lge.com/ abhishek.srivastava@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications:
 * Sl No        Modified by     Date        Version Description
 *
 *
 ********************************************************************/

#ifndef _PLUGIN_LOADER_H_
#define _PLUGIN_LOADER_H_

#include <gmodule.h>
#include <Location_Plugin.h>

static const gchar GPS_PLUGIN_NAME[] = "gps";   //without lib prefix
static const gchar WIFI_PLUGIN_NAME[] = "wifi"; //without lib prefix
static const gchar CELL_PLUGIN_NAME[] = "cell"; //without lib prefix
#ifdef NOMINATIUM_LBS
static const gchar LBS_PLUGIN_NAME[] = "lbs";   //without lib prefix
#else
static const gchar LBS_PLUGIN_NAME[] = "googlelbs";   //without lib prefix
#endif
/*
 * Common Plugin properties
 */
typedef struct {
    char *plugin_name;
    char *plugin_path;
    GModule *gmodule;   //Holds the loaded plugin eg: libgps.so
} Plugin_Prop;

/*
 * Gps Plugin Structure
 */
typedef struct {
    Plugin_Prop *plugin_prop;
    gpointer plugin_handler;
    gpointer(*init)(GpsPluginOps *ops);
    void (*shutdown)(gpointer handle);
    GpsPluginOps ops;
} GpsPlugin;

/*
 * Wifi Plugin Structure
 */
typedef struct {
    Plugin_Prop *plugin_prop;
    gpointer plugin_handler;
    gpointer(*init)(WifiPluginOps *ops);
    void (*shutdown)(gpointer handle);
    WifiPluginOps ops;
} WifiPlugin;
/*
 * Cell Plugin Structure
 */
typedef struct {
    Plugin_Prop *plugin_prop;
    gpointer plugin_handler;
    gpointer(*init)(CellPluginOps *ops);
    void (*shutdown)(gpointer handle);
    CellPluginOps ops;
} CellPlugin;

/*
 * LBS Plugin Structure
 */
typedef struct {
    Plugin_Prop *plugin_prop;
    gpointer plugin_handler;
    gpointer(*init)(LbsPluginOps *ops);
    void (*shutdown)(gpointer handle);
    LbsPluginOps ops;
} LbsPlugin;
/**
 * Add other Pluigin structure here
 */

/**
 *Plugin loader APIS
 */
gboolean plugin_init(void);
gpointer plugin_new(const char *plugin_name);
void plugin_free(gpointer mod, const char *plugin_name);
gboolean is_supported_plugin(const char *plugin_name);

#endif /* PLUGIN_LOADER_H_ */
