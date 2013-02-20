/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
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

#ifndef __GSIGNOND_SSOTEST_PLUGIN_H__
#define __GSIGNOND_SSOTEST_PLUGIN_H__

#include <glib-object.h>

#define GSIGNOND_TYPE_SSOTEST_PLUGIN \
    (gsignond_password_plugin_get_type ())
#define GSIGNOND_SSOTEST_PLUGIN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                 GSIGNOND_TYPE_SSOTEST_PLUGIN, \
                                 GSignondSsoTestPlugin))
#define GSIGNOND_IS_SSOTEST_PLUGIN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                 GSIGNOND_TYPE_SSOTEST_PLUGIN))
#define GSIGNOND_SSOTEST_PLUGIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                              GSIGNOND_TYPE_SSOTEST_PLUGIN, \
                              GSignondSsoTestPluginClass))
#define GSIGNOND_IS_SSOTEST_PLUGIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                              GSIGNOND_TYPE_SSOTEST_PLUGIN))
#define GSIGNOND_SSOTEST_PLUGIN_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                GSIGNOND_TYPE_SSOTEST_PLUGIN, \
                                GSignondSsoTestPluginClass))

typedef struct _GSignondSsoTestPlugin        GSignondSsoTestPlugin;
typedef struct _GSignondSsoTestPluginClass   GSignondSsoTestPluginClass;
typedef struct _GSignondSsoTestPluginPrivate GSignondSsoTestPluginPrivate;

struct _GSignondSsoTestPlugin
{
    GObject parent_instance;

    GSignondSsoTestPluginPrivate *priv;
};

struct _GSignondSsoTestPluginClass
{
    GObjectClass parent_class;
};

GType gsignond_password_plugin_get_type (void);

#endif /* __GSIGNOND_SSOTEST_PLUGIN_H__ */
