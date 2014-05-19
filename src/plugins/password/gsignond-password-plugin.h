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

#ifndef __GSIGNOND_PASSWORD_PLUGIN_H__
#define __GSIGNOND_PASSWORD_PLUGIN_H__

#include <glib-object.h>

#define GSIGNOND_TYPE_PASSWORD_PLUGIN             (gsignond_password_plugin_get_type ())
#define GSIGNOND_PASSWORD_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_PASSWORD_PLUGIN, GSignondPasswordPlugin))
#define GSIGNOND_IS_PASSWORD_PLUGIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_PASSWORD_PLUGIN))
#define GSIGNOND_PASSWORD_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSIGNOND_TYPE_PASSWORD_PLUGIN, GSignondPasswordPluginClass))
#define GSIGNOND_IS_PASSWORD_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSIGNOND_TYPE_PASSWORD_PLUGIN))
#define GSIGNOND_PASSWORD_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSIGNOND_TYPE_PASSWORD_PLUGIN, GSignondPasswordPluginClass))


typedef struct _GSignondPasswordPlugin        GSignondPasswordPlugin;
typedef struct _GSignondPasswordPluginClass   GSignondPasswordPluginClass;

struct _GSignondPasswordPlugin
{
    GObject parent_instance;
    
    int instance_member;
};

struct _GSignondPasswordPluginClass
{
    GObjectClass parent_class;
};

GType gsignond_password_plugin_get_type (void);

#endif /* __GSIGNOND_PASSWORD_PLUGIN_H__ */