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

#ifndef __GSIGNOND_PLUGIN_PROXY_FACTORY_H__
#define __GSIGNOND_PLUGIN_PROXY_FACTORY_H__

#include <glib-object.h>
#include "gsignond-plugin-proxy.h"
#include <gsignond/gsignond-config.h>

#define GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY             (gsignond_plugin_proxy_factory_get_type ())
#define GSIGNOND_PLUGIN_PROXY_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY, GSignondPluginProxyFactory))
#define GSIGNOND_IS_PLUGIN_PROXY_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY))
#define GSIGNOND_PLUGIN_PROXY_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY, GSignondPluginProxyFactoryClass))
#define GSIGNOND_IS_PLUGIN_PROXY_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY))
#define GSIGNOND_PLUGIN_PROXY_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_PLUGIN_PROXY_FACTORY, GSignondPluginProxyFactoryClass))


typedef struct _GSignondPluginProxyFactory        GSignondPluginProxyFactory;
typedef struct _GSignondPluginProxyFactoryClass   GSignondPluginProxyFactoryClass;

struct _GSignondPluginProxyFactory
{
    GObject parent_instance;
    
    /* Private */
    GSignondConfig *config;
    GHashTable* plugins;
    
    gchar** methods;
    GHashTable* methods_to_mechanisms;
    GHashTable* methods_to_loader_paths;
};

struct _GSignondPluginProxyFactoryClass
{
    GObjectClass parent_class;
};

GType gsignond_plugin_proxy_factory_get_type (void);

GSignondPluginProxyFactory* 
gsignond_plugin_proxy_factory_new(GSignondConfig *config);

GSignondPluginProxy*
gsignond_plugin_proxy_factory_get_plugin(GSignondPluginProxyFactory* factory,
                                         const gchar* plugin_type);

const gchar** 
gsignond_plugin_proxy_factory_get_plugin_types(
   GSignondPluginProxyFactory* factory);
   
const gchar**
gsignond_plugin_proxy_factory_get_plugin_mechanisms(
   GSignondPluginProxyFactory* factory, const gchar* plugin_type);
   

#endif /* __GSIGNOND_PLUGIN_PROXY_FACTORY_H__ */
