/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012-2014 Intel Corporation.
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

#include "config.h"

#include "gsignond/gsignond-log.h"
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


static gchar** _get_plugin_names_from_loader(const gchar* loader_path)
{
    gchar* command_line = g_strdup_printf("%s --list-plugins", loader_path);
    gchar* standard_output = NULL;
    gchar* standard_error = NULL;
    gint exit_status;
    GError* error = NULL;

    if (g_spawn_command_line_sync(command_line, &standard_output, &standard_error,
        &exit_status, &error)) {
        DBG("Loader %s returned plugin list %s", loader_path, standard_output);
        gchar** plugin_list = g_strsplit(standard_output, "\n", 0);
        g_free(command_line);
        g_free(standard_output);
        g_free(standard_error);
        return plugin_list;
    } else {
        DBG("Loader %s returned exit status %d, error %s", loader_path,
            exit_status, error->message);
        g_error_free(error);
        g_free(command_line);
        return NULL;
    }
}

static void _add_plugins(GSignondPluginProxyFactory* self, const gchar* loader_path, gchar** plugins)
{
    DBG ("Checking mechanisms of plugins provided by %s", loader_path);
    gchar **plugin_iter = plugins;
    while (*plugin_iter) {
        GSignondPlugin* plugin = GSIGNOND_PLUGIN (
                gsignond_plugin_remote_new (loader_path, *plugin_iter));
        if (plugin != NULL) {
            gchar* plugin_type;
            gchar** mechanisms;
            g_object_get(plugin,
                        "type", &plugin_type,
                        "mechanisms", &mechanisms,
                         NULL);
            if (g_strcmp0 (plugin_type, *plugin_iter) == 0) {
                const gchar* loader = g_hash_table_lookup(self->methods_to_loader_paths,
                                                          plugin_type);
                // Do not replace plugins provided by gsignond-plugind with
                // 3rd party plugins
                if (loader && g_str_has_suffix(loader, "/gsignond-plugind")) {
                    DBG("Do not replace plugin %s with plugin provided by loader %s",
                        plugin_type, loader_path);
                    g_strfreev(mechanisms);
                } else {
                    DBG("Adding plugin %s to plugin enumeration", plugin_type);
                    g_hash_table_insert(self->methods_to_mechanisms,
                        g_strdup(plugin_type), mechanisms);
                    g_hash_table_insert(self->methods_to_loader_paths,
                        g_strdup(plugin_type), g_strdup(loader_path));
                }
            } else {
                DBG("Plugin returned type property %s, which does not match requested type %s",
                    plugin_type, *plugin_iter);
                g_strfreev(mechanisms);
            }
            g_free(plugin_type);
            g_object_unref(plugin);
        }
        plugin_iter++;
    }
}

static void _insert_method(gchar* method, gchar*** method_iter_p)
{
    *(*method_iter_p) = method;
    (*method_iter_p)++;
}


static void _enumerate_plugins(GSignondPluginProxyFactory* self)
{
    const gchar *loaders_path = GSIGNOND_PLUGINLOADERS_DIR;
#   ifdef ENABLE_DEBUG
    const gchar* env_val = g_getenv("SSO_BIN_DIR");
    if (env_val)
        loaders_path = env_val;
#   endif

    GDir* loaders_dir = g_dir_open(loaders_path, 0, NULL);
    if (loaders_dir == NULL) {
        WARN ("plugin directory empty");
        return;
    }

    DBG ("Getting lists of plugins from loaders in %s (factory=%p)", loaders_path, self);
    while (1) {
        const gchar* loader_name = g_dir_read_name(loaders_dir);
        if (loader_name == NULL)
            break;
        gchar* loader_path = g_build_filename(loaders_path, loader_name, NULL);
        gchar** plugins = _get_plugin_names_from_loader(loader_path);
        if (plugins != NULL) {
            _add_plugins(self, loader_path, plugins);
            g_strfreev(plugins);
        }
        g_free(loader_path);
    }
    g_dir_close(loaders_dir);

    // make a flat list of available plugin types
    int n_plugins = g_hash_table_size(self->methods_to_mechanisms);
    self->methods = g_new0(gchar*, n_plugins + 1);
    gchar **method_iter = self->methods;

    GList* keys = g_hash_table_get_keys(self->methods_to_mechanisms);
    g_list_foreach(keys, (GFunc)_insert_method, &method_iter);

    g_list_free(keys);
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
    if (self->methods_to_mechanisms) {
        g_hash_table_destroy (self->methods_to_mechanisms);
        self->methods_to_mechanisms = NULL;
    }
    if (self->methods_to_loader_paths) {
        g_hash_table_destroy (self->methods_to_loader_paths);
        self->methods_to_loader_paths = NULL;
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
    self->methods_to_mechanisms = g_hash_table_new_full((GHashFunc)g_str_hash,
                                             (GEqualFunc)g_str_equal,
                                             (GDestroyNotify)g_free,
                                             (GDestroyNotify)g_strfreev);

    self->methods_to_loader_paths = g_hash_table_new_full((GHashFunc)g_str_hash,
                                             (GEqualFunc)g_str_equal,
                                             (GDestroyNotify)g_free,
                                             (GDestroyNotify)g_free);

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

static gboolean
_find_proxy_by_pointer (gpointer key, gpointer value, gpointer userdata)
{
    if (userdata == value) {
        g_free (key);
        return TRUE;
    }
    return FALSE;
}

static void
_remove_dead_proxy (gpointer data, GObject *dead_proxy)
{
    GSignondPluginProxyFactory *factory = GSIGNOND_PLUGIN_PROXY_FACTORY(data);
    if (factory) {
        g_hash_table_foreach_steal (factory->plugins, 
                _find_proxy_by_pointer, dead_proxy);
    }
}

static void
_proxy_toggle_ref_cb (gpointer userdata, GObject *proxy, gboolean is_last_ref)
{
    /* start/stop timeout timer */
    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (proxy), is_last_ref);

    if (is_last_ref) g_object_weak_ref (proxy, _remove_dead_proxy, userdata);
    else g_object_weak_unref (proxy, _remove_dead_proxy, userdata);
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

    if (g_hash_table_lookup(factory->methods_to_mechanisms, plugin_type) == NULL) {
        DBG("Plugin not known %s", plugin_type);
        return NULL;
    }

    proxy = g_hash_table_lookup(factory->plugins, plugin_type);
    if (proxy != NULL) {
        DBG("get existing plugin %s -> %p", plugin_type, proxy);
        g_object_ref(proxy);
        return proxy;
    }

    proxy = gsignond_plugin_proxy_new(g_hash_table_lookup(factory->methods_to_loader_paths, plugin_type), plugin_type,
                                      gsignond_config_get_integer (factory->config, GSIGNOND_CONFIG_PLUGIN_TIMEOUT));
    if (proxy == NULL) {
        return NULL;
    }
    g_hash_table_insert(factory->plugins, g_strdup (plugin_type), proxy);
    DBG("get new plugin %s -> %p", plugin_type, proxy);
    g_object_add_toggle_ref(G_OBJECT(proxy), _proxy_toggle_ref_cb, factory);

    return proxy;
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
    g_return_val_if_fail(factory->methods_to_mechanisms, NULL);

    if (factory->methods == NULL) {
        _enumerate_plugins (factory);
    }

    return g_hash_table_lookup(factory->methods_to_mechanisms, plugin_type);
}
