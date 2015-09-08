/********************************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group  : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : Plugin_Loader.c
 * Purpose  : Utilty function to load/unload  <plugin>.so  files
 * Platform  : RedRose
 * Author(s) : rajesh gopu
 * Email ID. : rajeshgopu.iv@lge.com
 * Creation Date: 14-02-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <Plugin_Loader.h>
#include <loc_log.h>

const char *PLUGIN_PATH = "/usr/lib/location/plugins";
//const char* PLUGIN_PATH = "/usr/lib";
//const char* PLUGIN_PATH = ".";
#define PLUGIN_MAX_INDEX 4

/**
 * <Funciton >   fill_plugin_property
 * <Description>  load the given plugin
 * @param     <plugin_name> <In> <name of the plugin to load>
 * @param     <is_resident> <In> <if true module will never be unloaded.
 *      Any future g_module_close() calls on the module will be ignored.>
 * @return    Plugin_Prop*
 */
static Plugin_Prop *
fill_plugin_property(const char *plugin_name, gboolean is_resident)
{
    if (!plugin_name)
        return NULL;

    Plugin_Prop *properties = g_new0(Plugin_Prop, 1);
    properties->plugin_name = g_strdup(plugin_name);

    if (!properties->plugin_name) {
        g_free(properties);
        return NULL;
    }

    properties->plugin_path = g_module_build_path(PLUGIN_PATH, properties->plugin_name);
    LS_LOG_DEBUG("plugin->plugin_path %s module support :%d \n", properties->plugin_path, g_module_supported());

    if (!properties->plugin_path) {
        g_free(properties->plugin_name);
        g_free(properties);
        return NULL;
    }

    properties->gmodule = g_module_open(properties->plugin_path, G_MODULE_BIND_LAZY);
    LS_LOG_DEBUG("plugin->gmodule %p \n", (properties->gmodule));

    if (!properties->gmodule) {
        g_free(properties->plugin_name);
        g_free(properties->plugin_path);
        g_free(properties);
        return NULL;
    }

    LS_LOG_DEBUG("is_resident %d \n ", is_resident);

    if (is_resident)
        g_module_make_resident(properties->gmodule);

    return properties;
}

/**
 * <Funciton >   unref_plugin
 * <Description>  Free the loaded plugin
 * @param     <Plugin> <In> <pointer to the plugin>
 * @return    void
 */
static void unref_plugin(Plugin_Prop *plugin)
{
    if (plugin->plugin_name)
        g_free(plugin->plugin_name);

    if (plugin->plugin_path)
        g_free(plugin->plugin_path);

    if (plugin->gmodule)
        g_module_close(plugin->gmodule);

    g_free(plugin);
}

/**
 * <Funciton >   plugin_init
 * <Description>  check the plugin supported or not
 * @return    void
 */
gboolean plugin_init(void)
{
    if (!g_module_supported()) {
        LS_LOG_ERROR("Plugin not supported ..!");
        return FALSE;
    }

    return TRUE;
}

/**
 * <Funciton >   gmod_find_sym
 * <Description>  find the symbol in the given plugin
 * @param     <Plugin> <In> <plugin to finc the symbol>
 * @param     <init_func> <In> <init function pointer>
 * @param     <shutdown_func> <In> <shutdown function pointer>
 * @return    gboolean
 */
static gboolean load_plugin_symbols(Plugin_Prop *plugin_prop, gpointer *init_func, gpointer *shutdown_func)
{
    char sym[256];
    g_stpcpy(sym, "init");

    if (!g_module_symbol(plugin_prop->gmodule, sym, init_func)) {
        LS_LOG_ERROR("symbol not found: %s", sym);
        return FALSE;
    }

    g_stpcpy(sym, "shutdown");

    if (!g_module_symbol(plugin_prop->gmodule, sym, shutdown_func)) {
        LS_LOG_ERROR("symbol not found: %s", sym);
        return FALSE;
    }

    return TRUE;
}

/**
 * <Funciton >   mod_new
 * <Description>  load the plugin
 * @param     <plugin_name> <In> <name of plugin to load>
 * @return    gpointer
 */
static gpointer load_plugin(const char *plugin_name)
{
    gpointer ret_plugin = NULL;

    if (!plugin_name)
        return NULL;

    Plugin_Prop *plugin_prop = NULL;
    gpointer init_symb = NULL;
    gpointer shutdown_symb = NULL;
    plugin_prop = fill_plugin_property(plugin_name, TRUE);

    if (!plugin_prop) {
        LS_LOG_ERROR("plugin (%s) new failed\n", plugin_name);
        return NULL;
    }

    if (!load_plugin_symbols(plugin_prop, &init_symb, &shutdown_symb)) {
        LS_LOG_ERROR("symbol (init, shutdown) finding failed\n");
        unref_plugin(plugin_prop);
        return NULL;
    }

    if (0 == g_strcmp0(plugin_name, GPS_PLUGIN_NAME)) {
        GpsPlugin *gps_plugin = g_new0(GpsPlugin, 1); //Create GPS plugin
        g_return_val_if_fail(gps_plugin, NULL);
        gps_plugin->plugin_prop = plugin_prop;
        gps_plugin->init = init_symb;
        gps_plugin->shutdown = shutdown_symb;
        gps_plugin->plugin_handler = gps_plugin->init(&(gps_plugin->ops));

        if (!gps_plugin->plugin_handler) {
            LS_LOG_WARNING(" GPS plugin init failed\n");
            unref_plugin(gps_plugin->plugin_prop);
            ret_plugin = NULL;
        } else
            ret_plugin = (gpointer) gps_plugin;
    } else if (0 == g_strcmp0(plugin_name, NETWORK_PLUGIN_NAME)) {
        NetworkPlugin *network_plugin = g_new0(NetworkPlugin, 1); //Create NW plugin
        g_return_val_if_fail(network_plugin, NULL);
        network_plugin->plugin_prop = plugin_prop;
        network_plugin->init = init_symb;
        network_plugin->shutdown = shutdown_symb;
        network_plugin->plugin_handler = network_plugin->init(&(network_plugin->ops));

        if (!network_plugin->plugin_handler) {
            LS_LOG_WARNING(" nw plugin init failed\n");
            unref_plugin(network_plugin->plugin_prop);
            ret_plugin = NULL;
        } else
            ret_plugin = (gpointer) network_plugin;
    } else if (0 == g_strcmp0(plugin_name, LBS_PLUGIN_NAME)) {
        LbsPlugin *lbs_plugin = g_new0(LbsPlugin, 1); //Create Cell plugin
        g_return_val_if_fail(lbs_plugin, NULL);
        lbs_plugin->plugin_prop = plugin_prop;
        lbs_plugin->init = init_symb;
        lbs_plugin->shutdown = shutdown_symb;
        lbs_plugin->plugin_handler = lbs_plugin->init(&(lbs_plugin->ops));

        if (lbs_plugin->plugin_handler == NULL) {
            LS_LOG_WARNING(" LBS plugin init failed\n");
            unref_plugin(lbs_plugin->plugin_prop);
            ret_plugin = NULL;
        } else
            ret_plugin = (gpointer) lbs_plugin;
    } else {
        LS_LOG_WARNING("module name (%s) is wrong", plugin_name);
        ret_plugin = NULL;
    }

    return ret_plugin;
}

/**
 * <Funciton >   plugin_new
 * <Description>  load the given plugin
 * @param     <plugin_name> <In> <name of the plugin to enable>
 * @return    gpointer
 */
gpointer plugin_new(const char *plugin_name)
{
    if (!plugin_name)
        return NULL;

    char name[20];
    gpointer mod = NULL;
    g_snprintf(name, 20, "%s", plugin_name);
    mod = load_plugin(name);

    if (mod) {
        LS_LOG_INFO("plugin (%s) open success\n", name);
    } else {
        LS_LOG_INFO("plugin (%s) open failed\n", name);
    }

    return mod;
}

/**
 * <Funciton >   mod_free
 * <Description>  free the plugin
 * @param     <plugin> <In> <pointer to plugin>
 * @param     <plugin> <In> <plugin_name>
 * @return    void
 */
static void unload_plugin(gpointer plugin, const char *plugin_name)
{
    if (!plugin || !plugin_name)
        return;

    if (0 == g_strcmp0(plugin_name, GPS_PLUGIN_NAME)) {
        GpsPlugin *gps_plugin = (GpsPlugin *) plugin;

        if (gps_plugin->shutdown && gps_plugin->plugin_handler) {
            gps_plugin->shutdown(gps_plugin->plugin_handler);
        }

        gps_plugin->plugin_handler = NULL;
        gps_plugin->init = NULL;
        gps_plugin->shutdown = NULL;
        unref_plugin(gps_plugin->plugin_prop);
        gps_plugin->plugin_prop = NULL;
    }  else if (0 == g_strcmp0(plugin_name, LBS_PLUGIN_NAME)) {
        LbsPlugin *lbs_plugin = (LbsPlugin *) plugin;

        if (lbs_plugin->shutdown && lbs_plugin->plugin_handler) {
            lbs_plugin->shutdown(lbs_plugin->plugin_handler);
        }

        lbs_plugin->plugin_handler = NULL;
        lbs_plugin->init = NULL;
        lbs_plugin->shutdown = NULL;
        unref_plugin(lbs_plugin->plugin_prop);
        lbs_plugin->plugin_prop = NULL;
    } else if (0 == g_strcmp0(plugin_name, NETWORK_PLUGIN_NAME)) {
        NetworkPlugin *network_plugin = (NetworkPlugin *) plugin;

        if (network_plugin->shutdown && network_plugin->plugin_handler) {
            network_plugin->shutdown(network_plugin->plugin_handler);
        }

        network_plugin->plugin_handler = NULL;
        network_plugin->init = NULL;
        network_plugin->shutdown = NULL;
        unref_plugin(network_plugin->plugin_prop);
        network_plugin->plugin_prop = NULL;
    } else {
        LS_LOG_INFO("plugin  name (%s) not found or not configured in loader\n", plugin_name);
    }

    g_free(plugin);
}

/**
 * <Funciton >   plugin_free
 * <Description>  free the plugin
 * @param     <plugin> <In> pointer to the plugin>
 * @param     <plugin_name> <In> <plugin name to free>
 * @return    void
 */

void plugin_free(gpointer plugin, const char *plugin_name)
{
    if (!plugin || !plugin_name)
        return;

    unload_plugin(plugin, plugin_name);
}

gboolean plugin_is_supported(const char *plugin_name)
{
    if (!plugin_name)
        return FALSE;

    int index = 0;
    gboolean ret = FALSE;
    gboolean found = FALSE;

    for (index = 0 ; index < PLUGIN_MAX_INDEX ; index++) {
        ret = is_supported_plugin(plugin_name);

        if (ret == TRUE) {
            found = TRUE;
            LS_LOG_DEBUG("module name(%s) is found", plugin_name);
            break;
        }
    }

    return found;
}
/**
 * <Funciton >   is_supported_plugin
 * <Description>  Check the plugin is supported
 * @param     <plugin_name> <In> <name of the plugin to check>
 * @return    gboolean
 */
gboolean is_supported_plugin(const char *plugin_name)
{
    if (!plugin_name)
        return FALSE;

    Plugin_Prop *plugin = fill_plugin_property(plugin_name, FALSE);

    if (!plugin) {
        return FALSE;
    }

    unref_plugin(plugin);
    LS_LOG_DEBUG("plugin name(%s) is found", plugin_name);
    return TRUE;
}

