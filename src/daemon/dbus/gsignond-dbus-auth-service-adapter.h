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

#ifndef __GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_H_
#define __GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_H_

#include <config.h>
#include <glib.h>
#include <daemon/gsignond-daemon.h>
#include "gsignond-disposable.h"
#include "gsignond-dbus-auth-service-gen.h"

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER            (gsignond_dbus_auth_service_adapter_get_type())
#define GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, GSignondDbusAuthServiceAdapter))
#define GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, GSignondDbusAuthServiceAdapterClass))
#define GSIGNOND_IS_DBUS_AUTH_SERVICE_ADAPTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER))
#define GSIGNOND_IS_DBUS_AUTH_SERVICE_ADAPTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER))
#define GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, GSignondDbusAuthServiceAdapterClass))

typedef struct _GSignondDbusAuthServiceAdapter GSignondDbusAuthServiceAdapter;
typedef struct _GSignondDbusAuthServiceAdapterClass GSignondDbusAuthServiceAdapterClass;
typedef struct _GSignondDbusAuthServiceAdapterPrivate GSignondDbusAuthServiceAdapterPrivate;

struct _GSignondDbusAuthServiceAdapter
{
    GSignondDisposable parent;

    /* priv */
    GSignondDbusAuthServiceAdapterPrivate *priv;
};

struct _GSignondDbusAuthServiceAdapterClass
{
    GSignondDisposableClass parent_class;
};

GType gsignond_dbus_auth_service_adapter_get_type (void) G_GNUC_CONST;

GSignondDbusAuthServiceAdapter *
gsignond_dbus_auth_service_adapter_new_with_connection (GDBusConnection *connection,
                                                        GSignondDaemon *daemon);

#ifndef USE_P2P
GSignondDbusAuthServiceAdapter *
gsignond_dbus_auth_service_adapter_new (GSignondDaemon *daemon);
#endif

G_END_DECLS

#endif /* __GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_H_ */
