/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __GSIGNOND_PLUGIN_REMOTE_H_
#define __GSIGNOND_PLUGIN_REMOTE_H_

#include <glib.h>
#include <daemon/dbus/gsignond-dbus-remote-plugin-gen.h>
#include <gsignond/gsignond-config.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_PLUGIN_REMOTE \
    (gsignond_plugin_remote_get_type())
#define GSIGNOND_PLUGIN_REMOTE(obj)  (G_TYPE_CHECK_INSTANCE_CAST((obj),\
    GSIGNOND_TYPE_PLUGIN_REMOTE, GSignondPluginRemote))
#define GSIGNOND_PLUGIN_REMOTE_CLASS(klass)\
    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_PLUGIN_REMOTE, \
    GSignondPluginRemoteClass))
#define GSIGNOND_IS_PLUGIN_REMOTE(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_PLUGIN_REMOTE))
#define GSIGNOND_IS_PLUGIN_REMOTE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_PLUGIN_REMOTE))
#define GSIGNOND_PLUGIN_REMOTE_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_PLUGIN_REMOTE, \
    GSignondPluginRemoteClass))

typedef struct _GSignondPluginRemote GSignondPluginRemote;
typedef struct _GSignondPluginRemoteClass GSignondPluginRemoteClass;
typedef struct _GSignondPluginRemotePrivate GSignondPluginRemotePrivate;

struct _GSignondPluginRemote
{
    GObject parent;

    /* priv */
    GSignondPluginRemotePrivate *priv;
};

struct _GSignondPluginRemoteClass
{
    GObjectClass parent_class;
};

GType
gsignond_plugin_remote_get_type (void) G_GNUC_CONST;

GSignondPluginRemote *
gsignond_plugin_remote_new (
        const gchar *loader_path,
        const gchar *plugin_type);

G_END_DECLS

#endif /* __GSIGNOND_PLUGIN_REMOTE_H_ */
