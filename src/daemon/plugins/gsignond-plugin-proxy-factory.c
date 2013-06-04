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

#include <string.h>
#include <stdio.h>

#include "gsignond/gsignond-log.h"
#include "common/gsignond-plugin-loader.h"
#include "gsignond-plugin-proxy-factory.h"
#include "gsignond-plugin-remote.h"

G_DEFINE_TYPE (GSignondPluginProxyFactory, gsignond_plugin_proxy_factory, G_TYPE_OBJECT);


enum
{
    PROP_0,
    
    PROP_CONFIG,
    
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void _enumerate_plugins(GSignondPluginProxyFactory* self)
{
    GDir* plugin_dir = g_dir_open(gsignond_config_get_string (self->config, 
        GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR), 0, NULL);
    if (plugin_dir == NULL) {
        WARN ("plugin directory empty");
        return;
    }
        
    int n_plugins = 0;
    while (g_dir_read_name(plugin_dir) != NULL)
        n_plugins++;
    g_dir_rewind(plugin_dir);
    
    self->methods = g_malloc0(sizeof(gchar*) * (n_plugins + 1));

    DBG ("enumerate plugins in %s (factory=%p)",
         gsignond_config_get_string (self->config,
                                     GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR),
                                     self
        );
    gchar **method_iter = self->methods;
    while (1) {
        const gchar* plugin_soname = g_dir_read_name(plugin_dir);
        if (plugin_soname == NULL)
            break;
        if (g_str_has_prefix(plugin_soname, "lib") && 
            g_str_has_suffix(plugin_soname, ".so")) {
            gchar* plugin_name = g_strndup(plugin_soname+3, 
                strlen(plugin_soname) - 6);
            GSignondPlugin* plugin = GSIGNOND_PLUGIN (
                    gsignond_plugin_remote_new (self->config, plugin_name));
            if (plugin != NULL) {
                gchar* plugin_type;
                gchar** mechanisms;
                g_object_get(plugin, 
                            "type", &plugin_type, 
                            "mechanisms", &mechanisms, 
                             NULL);
                if (g_strcmp0 (plugin_type, plugin_name) == 0) {
                    *method_iter = plugin_type;
                    method_iter++;
                    g_hash_table_insert(self->mechanisms,
                        plugin_type, mechanisms);
                } else {
                    g_free(plugin_type);
                    g_strfreev(mechanisms);
                }
                g_object_unref(plugin);
            }
            g_free(plugin_name);
        }
    }
    g_dir_close(plugin_dir);
}

static GObject *
gsignond_plugin_proxy_factory_constructor (GType                  gtype,
                                   guint                  n_properties,
                                   GObjectConstructParam *properties)
{
  GObject *obj;

  {
    /* Always chain up to the parent constructor */
    obj = G_OBJECT_CLASS (gsignond_plugin_proxy_factory_parent_class)->constructor (
        gtype, n_properties, properties);
  }
  
  return obj;
}

static void
gsignond_plugin_proxy_factory_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
    GSignondPluginProxyFactory *self = GSIGNOND_PLUGIN_PROXY_FACTORY (object);
    switch (property_id)
    {
        case PROP_CONFIG:
            g_assert (self->config == NULL);
            self->config = g_value_dup_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gsignond_plugin_proxy_factory_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
    GSignondPluginProxyFactory *self = GSIGNOND_PLUGIN_PROXY_FACTORY (object);
    
    switch (prop_id)
    {
        case PROP_CONFIG:
            g_value_set_object (value, self->config);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gsignond_plugin_proxy_factory_dispose (GObject *gobject)
{
    GSignondPluginProxyFactory *self = GSIGNOND_PLUGIN_PROXY_FACTORY (gobject);

    if (self->config) {
        g_object_unref (self->config);
        self->config = NULL;
    }

  /* Chain up to the parent class */
  G_OBJECT_CLASS (gsignond_plugin_proxy_factory_parent_class)->dispose (gobject);
}

static void
gsignond_plugin_proxy_factory_finalize (GObject *gobject)
{
    GSignondPluginProxyFactory *self = GSIGNOND_PLUGIN_PROXY_FACTORY (gobject);

    if (self->plugins) {
        g_hash_table_destroy (self->plugins);
        self->plugins = NULL;
    }
    if (self->mechanisms) {
        g_hash_table_destroy (self->mechanisms);
        self->mechanisms = NULL;
    }
    if (self->methods) {
        g_free (self->methods);
        self->methods = NULL;
    }

    /* Chain up to the parent class */
    G_OBJECT_CLASS (gsignond_plugin_proxy_factory_parent_class)->finalize (gobject);
}


static void
gsignond_plugin_proxy_factory_class_init (GSignondPluginProxyFactoryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->constructor = gsignond_plugin_proxy_factory_constructor;
    gobject_class->set_property = gsignond_plugin_proxy_factory_set_property;
    gobject_class->get_property = gsignond_plugin_proxy_factory_get_property;
    gobject_class->dispose = gsignond_plugin_proxy_factory_dispose;
    gobject_class->finalize = gsignond_plugin_proxy_factory_finalize;

    obj_properties[PROP_CONFIG] = g_param_spec_object ("config",
                                                       "config",
                                                       "Configuration object",
                                                       GSIGNOND_TYPE_CONFIG,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS);
    

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void
gsignond_plugin_proxy_factory_init (GSignondPluginProxyFactory *self)
{
    self->mechanisms = g_hash_table_new_full((GHashFunc)g_str_hash,
                                             (GEqualFunc)g_str_equal,
                                             (GDestroyNotify)g_free,
                                             (GDestroyNotify)g_strfreev);

    self->plugins = g_hash_table_new_full ((GHashFunc)g_str_hash,
                                           (GEqualFunc)g_str_equal,
                                           (GDestroyNotify)g_free,
                                           (GDestroyNotify)g_object_unref);

    self->methods = NULL;
}

GSignondPluginProxyFactory* 
gsignond_plugin_proxy_factory_new(GSignondConfig *config)
{
    GSignondPluginProxyFactory* proxy = g_object_new(
                                              GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY,
                                              "config", config,
                                              NULL);
    return proxy;
}

GSignondPluginProxy*
gsignond_plugin_proxy_factory_get_plugin(GSignondPluginProxyFactory* factory,
                                         const gchar* plugin_type)
{
    g_return_val_if_fail (factory && GSIGNOND_IS_PLUGIN_PROXY_FACTORY(factory), NULL);
    g_return_val_if_fail (plugin_type, NULL);

    GSignondPluginProxy* proxy = NULL;

    if (factory->methods == NULL) {
        _enumerate_plugins (factory);
    }

    if (g_hash_table_lookup(factory->mechanisms, plugin_type) == NULL) {
        DBG("Plugin not known %s", plugin_type);
        return NULL;
    }

    proxy = g_hash_table_lookup(factory->plugins, plugin_type);
    if (proxy != NULL) {
        DBG("get existing plugin %s -> %p", plugin_type, proxy);
        g_object_ref(proxy);
        return proxy;
    }

    proxy = gsignond_plugin_proxy_new(factory->config, plugin_type);
    if (proxy == NULL) {
        return NULL;
    }
    g_hash_table_insert(factory->plugins, g_strdup (plugin_type), proxy);
    DBG("get new plugin %s -> %p", plugin_type, proxy);
    g_object_ref(proxy);
    return proxy;
}

gboolean gsignond_plugin_proxy_factory_add_plugin(
    GSignondPluginProxyFactory* factory,
    GSignondPluginProxy* proxy)
{
    g_return_val_if_fail (factory && GSIGNOND_IS_PLUGIN_PROXY_FACTORY(factory), FALSE);
    g_return_val_if_fail (proxy && GSIGNOND_IS_PLUGIN_PROXY(proxy), FALSE);
    
    gchar* plugin_type = NULL;
    g_object_get (proxy, "type", &plugin_type, NULL);

    if (g_hash_table_contains(factory->plugins, plugin_type)) {
        g_free(plugin_type);
        return FALSE;
    }
    g_object_ref(proxy);
    DBG("add plugin %s -> %p", plugin_type, proxy);
    g_hash_table_insert(factory->plugins, plugin_type, proxy);

    return TRUE;
}

const gchar** 
gsignond_plugin_proxy_factory_get_plugin_types(
   GSignondPluginProxyFactory* factory)
{
	if (factory->methods == NULL) {
		_enumerate_plugins (factory);
	}
    return (const gchar**)factory->methods;
}
   
const gchar**
gsignond_plugin_proxy_factory_get_plugin_mechanisms(
   GSignondPluginProxyFactory* factory, const gchar* plugin_type)
{
    g_return_val_if_fail(factory->mechanisms, NULL);

    if (factory->methods == NULL) {
        _enumerate_plugins (factory);
    }

    return g_hash_table_lookup(factory->mechanisms, plugin_type);
}
