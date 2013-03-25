/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
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

#ifndef __GSIGNOND_SIGNONUI_ADAPTER_H_
#define __GSIGNOND_SIGNONUI_ADAPTER_H_

#include <glib.h>
#include <gio/gio.h>
#include "gsignond-dbus-signonui-gen.h"

G_BEGIN_DECLS

#define GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER            (gsignond_dbus_signonui_adapter_get_type())
#define GSIGNOND_DBUS_SIGNONUI_ADAPTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, GSignondDbusSignonuiAdapter))
#define GSIGNOND_DBUS_SIGNONUI_ADAPTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, GSignondDbusSignonuiAdapterClass))
#define GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER))
#define GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER))
#define GSIGNOND_DBUS_SIGNONUI_ADAPTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, GSignondDbusSignonuiAdapterClass))

typedef struct _GSignondDbusSignonuiAdapter GSignondDbusSignonuiAdapter;
typedef struct _GSignondDbusSignonuiAdapterClass GSignondDbusSignonuiAdapterClass;
typedef struct _GSignondDbusSignonuiAdapterPrivate GSignondDbusSignonuiAdapterPrivate;

typedef void (*GSignondDbusSignonuiQueryDialogCb)(GVariant *out_params, GError *error, gpointer user_data);
typedef void (*GSignondDbusSignonuiRefreshDialogCb)(GError *error, gpointer user_data);
typedef void (*GSignondDbusSignonuiCancelRequestCb)(GError *error, gpointer user_data);

struct _GSignondDbusSignonuiAdapter
{
    GObject parent;

    /* priv */
    GSignondDbusSignonuiAdapterPrivate *priv;
};

struct _GSignondDbusSignonuiAdapterClass
{
    GObjectClass parent_class;
};

GType gsignond_dbus_signonui_adapter_get_type (void) G_GNUC_CONST;

GSignondDbusSignonuiAdapter * gsignond_dbus_signonui_adapter_new ();

gboolean
gsignond_dbus_signonui_adapter_query_dialog (GSignondDbusSignonuiAdapter *adapter,
                                             GVariant *params,
                                             const GSignondDbusSignonuiQueryDialogCb callback,
                                             gpointer user_data);

gboolean
gsignond_dbus_signonui_adapter_refresh_dialog (GSignondDbusSignonuiAdapter *adapter,
                                               GVariant *params,
                                               const GSignondDbusSignonuiRefreshDialogCb callback,
                                               gpointer user_data);

gboolean
gsignond_dbus_signonui_adapter_cancel_request (GSignondDbusSignonuiAdapter *adapter,
                                               const gchar *request_id,
                                               const GSignondDbusSignonuiCancelRequestCb callback,
                                               gpointer user_data);

G_END_DECLS

#endif /* __GSIGNOND_SIGNONUI_ADAPTER_H_ */

