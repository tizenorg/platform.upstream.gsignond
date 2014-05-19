/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#ifndef __GSIGNOND_DBUS_SERVER_H_
#define __GSIGNOND_DBUS_SERVER_H_

#include <config.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_DBUS_SERVER            (gsignond_dbus_server_get_type())
#define GSIGNOND_DBUS_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_DBUS_SERVER, GSignondDbusServer))
#define GSIGNOND_DBUS_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_DBUS_SERVER, GSignondDbusServerClass))
#define GSIGNOND_IS_DBUS_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_DBUS_SERVER))
#define GSIGNOND_IS_DBUS_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_DBUS_SERVER))
#define GSIGNOND_DBUS_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_DBUS_SERVER, GSignondDbusServerClass))

typedef struct _GSignondDbusServer GSignondDbusServer;
typedef struct _GSignondDbusServerClass GSignondDbusServerClass;
typedef struct _GSignondDbusServerPrivate GSignondDbusServerPrivate;

struct _GSignondDbusServer
{
    GObject parent;

    /* priv */
    GSignondDbusServerPrivate *priv;
};

struct _GSignondDbusServerClass
{
    GObjectClass parent_class;
};

GType gsignond_dbus_server_get_type();

#ifdef USE_P2P
GSignondDbusServer * gsignond_dbus_server_new_with_address (const gchar *address);

const gchar *
gsignond_dbus_server_get_address (GSignondDbusServer *server) G_GNUC_CONST;

#endif
GSignondDbusServer * gsignond_dbus_server_new ();

#endif /* __GSIGNOND_DBUS_SERVER_H_ */
