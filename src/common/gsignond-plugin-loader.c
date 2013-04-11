/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include "gsignond/gsignond-plugin-loader.h"
#include "gsignond/gsignond-log.h"
#include <gmodule.h>

GSignondPlugin*
gsignond_load_plugin(
        GSignondConfig* config,
        gchar* plugin_type)
{
    gchar* plugin_filename = g_module_build_path (
            gsignond_config_get_string (config,
                    GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR), plugin_type);
    GSignondPlugin *plugin = gsignond_load_plugin_with_filename (plugin_type,
            plugin_filename);
    g_free(plugin_filename);
    return plugin;
}

GSignondPlugin*
gsignond_load_plugin_with_filename(
        gchar *plugin_type,
        gchar *plugin_filename)
{
    DBG("Loading plugin %s", plugin_filename);
    GModule* plugin_module = g_module_open (plugin_filename, 
            G_MODULE_BIND_LOCAL);
    if (plugin_module == NULL) {
        DBG("Plugin couldn't be opened: %s", g_module_error());
        return NULL;
    }

    gchar* plugin_get_type = g_strdup_printf("gsignond_%s_plugin_get_type",
        plugin_type);
    gpointer p;

    DBG("Resolving symbol %s", plugin_get_type);
    gboolean symfound = g_module_symbol (plugin_module,
        plugin_get_type, &p);
    g_free(plugin_get_type);
    if (!symfound) {
        DBG("Symbol couldn't be resolved");
        g_module_close (plugin_module);
        return NULL;
    }
    
    DBG("Creating plugin object");
    GType (*plugin_get_type_f)(void) = p;
    GSignondPlugin* plugin = g_object_new(plugin_get_type_f(), NULL);
    if (plugin == NULL) {
        DBG("Plugin couldn't be created");
        g_module_close (plugin_module);
        return NULL;
    }
    g_module_make_resident (plugin_module);
    return plugin;
}
