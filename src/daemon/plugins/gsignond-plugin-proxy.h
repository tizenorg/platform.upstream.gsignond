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

#ifndef __GSIGNOND_PLUGIN_PROXY_H__
#define __GSIGNOND_PLUGIN_PROXY_H__

#include <glib-object.h>
#include <gsignond/gsignond-plugin-interface.h>
#include <gsignond/gsignond-config.h>
#include "../gsignond-auth-session-iface.h"

#define GSIGNOND_TYPE_PLUGIN_PROXY             (gsignond_plugin_proxy_get_type ())
#define GSIGNOND_PLUGIN_PROXY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_PLUGIN_PROXY, GSignondPluginProxy))
#define GSIGNOND_IS_PLUGIN_PROXY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_PLUGIN_PROXY))
#define GSIGNOND_PLUGIN_PROXY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_PLUGIN_PROXY, GSignondPluginProxyClass))
#define GSIGNOND_IS_PLUGIN_PROXY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_PLUGIN_PROXY))
#define GSIGNOND_PLUGIN_PROXY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_PLUGIN_PROXY, GSignondPluginProxyClass))


typedef struct _GSignondPluginProxy        GSignondPluginProxy;
typedef struct _GSignondPluginProxyClass   GSignondPluginProxyClass;
typedef struct _GSignondPluginProxyPrivate GSignondPluginProxyPrivate;

struct _GSignondPluginProxy
{
    GObject parent_instance;
    
    /* Private */
    GSignondPluginProxyPrivate *priv;
};

struct _GSignondPluginProxyClass
{
    GObjectClass parent_class;
};

GType gsignond_plugin_proxy_get_type (void);

GSignondPluginProxy* 
gsignond_plugin_proxy_new(GSignondConfig *config, const gchar* plugin_type);


void 
gsignond_plugin_proxy_cancel (GSignondPluginProxy *self, 
                        GSignondAuthSessionIface* session);
void gsignond_plugin_proxy_process (GSignondPluginProxy *self, 
                              GSignondAuthSessionIface* session,
                              GSignondSessionData *session_data, 
                              const gchar *mechanism);
void gsignond_plugin_proxy_user_action_finished (GSignondPluginProxy *self, 
                                           GSignondSessionData *session_data);
void gsignond_plugin_proxy_refresh (GSignondPluginProxy *self, 
                              GSignondSessionData *session_data);


#endif /* __GSIGNOND_PLUGIN_PROXY_H__ */
