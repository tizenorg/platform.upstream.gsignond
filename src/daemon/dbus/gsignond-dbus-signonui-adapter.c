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

#include "gsignond-dbus-signonui-adapter.h"
#include "gsignond-dbus-signonui-gen.h"
#include "gsignond/gsignond-log.h"
#include "gsignond-dbus.h"

struct _GSignondDbusSignonuiAdapterPrivate
{
    GDBusConnection *connection;
    GSignondDbusSinglesignonui *proxy;
};

G_DEFINE_TYPE (GSignondDbusSignonuiAdapter, gsignond_dbus_signonui_adapter, G_TYPE_OBJECT)

enum {
    SIG_REFRESH,
    SIG_MAX
};

static guint _signals[SIG_MAX];

#define GSIGNOND_DBUS_SIGNONUI_ADAPTER_GET_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, GSignondDbusSignonuiAdapterPrivate)

typedef struct
{
    GSignondDbusSignonuiAdapter *adapter;
    gpointer cb;
    gpointer data;
} _SignonuiDbusInfo;

static void
_dispose (GObject *object)
{
    GSignondDbusSignonuiAdapter *self = GSIGNOND_DBUS_SIGNONUI_ADAPTER (object);

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }
    
    if (self->priv->proxy) {
        g_object_unref (self->priv->proxy);
        self->priv->proxy = NULL;
    }

    G_OBJECT_CLASS (gsignond_dbus_signonui_adapter_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    G_OBJECT_CLASS (gsignond_dbus_signonui_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_signonui_adapter_class_init (GSignondDbusSignonuiAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusSignonuiAdapterPrivate));

    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    _signals[SIG_REFRESH] = g_signal_new ("refresh",
                                          GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER,
                                          G_SIGNAL_RUN_LAST,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL,
                                          G_TYPE_NONE,
                                          1, G_TYPE_STRING);
}

static void
gsignond_dbus_signonui_adapter_init (GSignondDbusSignonuiAdapter *self)
{
    self->priv = GSIGNOND_DBUS_SIGNONUI_ADAPTER_GET_PRIV(self);
    self->priv->connection = 0;
    self->priv->proxy = 0;
}

static void
_on_query_dialog_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    GVariant *out_params = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    gsignond_dbus_singlesignonui_call_query_dialog_finish (
            GSIGNOND_DBUS_SINGLESIGNONUI (proxy), &out_params, res, &error);

    if (!info) {
        ERR ("Memory curropted");
        return;
    }

    if (info->cb) {
        ((GSignondDbusSignonuiQueryDialogCb)info->cb) (out_params, error, info->data);
        g_object_unref (info->adapter);
        g_free (info);
    }
    else {
        if (error) g_error_free (error);
        if (out_params) g_variant_unref (out_params);
    }
}

gboolean
gsignond_dbus_signonui_adapter_query_dialog (GSignondDbusSignonuiAdapter *adapter,
                                             GVariant *params,
                                             const GSignondDbusSignonuiQueryDialogCb callback,
                                             gpointer user_data)
{
    if (!adapter || !GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) {
        WARN ("assert (!adapter ||!GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) failed"); 
        return FALSE;
    }
    _SignonuiDbusInfo *info = g_new0 (_SignonuiDbusInfo, 1);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;
    gsignond_dbus_singlesignonui_call_query_dialog (adapter->priv->proxy, params, NULL,
                _on_query_dialog_ready, (gpointer)info);

    return TRUE;
}

static void
_on_refresh_dialog_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    gsignond_dbus_singlesignonui_call_refresh_dialog_finish (
            GSIGNOND_DBUS_SINGLESIGNONUI (proxy), res, &error);

    if (!info) {
        ERR ("Memory curropted");
        g_error_free (error);
        return;
    }

    if (info->cb) {
        ((GSignondDbusSignonuiRefreshDialogCb)info->cb) (error, info->data);
        g_object_unref (info->adapter);
        g_free (info);
    }
    else if (error) g_error_free (error);
}

gboolean
gsignond_dbus_signonui_adapter_refresh_dialog (GSignondDbusSignonuiAdapter *adapter,
                                               GVariant *params,
                                               const GSignondDbusSignonuiRefreshDialogCb callback,
                                               gpointer user_data)
{
    if (!adapter || !GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) {
        WARN ("assert (!adapter ||!GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) failed"); 
        return FALSE;
    }
    _SignonuiDbusInfo *info = g_new0 (_SignonuiDbusInfo, 1);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;
    gsignond_dbus_singlesignonui_call_refresh_dialog (adapter->priv->proxy, params, NULL,
                _on_refresh_dialog_ready, (gpointer)info);

    return TRUE;
}

static void
_on_cancel_request_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    gsignond_dbus_singlesignonui_call_cancel_ui_request_finish (
            GSIGNOND_DBUS_SINGLESIGNONUI (proxy), res, &error);

    if (!info) {
        ERR ("Memory curropted");
        g_error_free (error);
        return;
    }

    if (info->cb) {
        ((GSignondDbusSignonuiCancelRequestCb)info->cb) (error, info->data);
        g_object_unref (info->adapter);
        g_free (info);
    }
    else if (error) g_error_free (error);
}

gboolean
gsignond_dbus_signonui_adapter_cancel_request (GSignondDbusSignonuiAdapter *adapter,
                                               const gchar *request_id,
                                               const GSignondDbusSignonuiCancelRequestCb callback,
                                               gpointer user_data)
{
    if (!adapter || !GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) {
        WARN ("assert (!adapter ||!GSIGNOND_IS_DBUS_SIGNONUI_ADAPTER (adapter)) failed"); 
        return FALSE;
    }
    _SignonuiDbusInfo *info = g_new0 (_SignonuiDbusInfo, 1);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;
    gsignond_dbus_singlesignonui_call_cancel_ui_request (adapter->priv->proxy, request_id, NULL,
                _on_cancel_request_ready, (gpointer)info);

    return TRUE;
}

static void
_on_refresh_request (GSignondDbusSignonuiAdapter *proxy, gchar *request_id, gpointer userdata)
{
    GSignondDbusSignonuiAdapter *adapter = GSIGNOND_DBUS_SIGNONUI_ADAPTER (userdata);

    if (!adapter) {
        ERR ("DBus-Error: memroy curroption");
        return;
    }

    g_signal_emit (adapter, _signals[SIG_REFRESH], 0, request_id);
}

/**
 * gsignond_dbus_signonui_adapter_new:
 *
 * Creates new instance of #GSignondDbusSignonuiAdapter 
 *
 * Retrurns: (transfer full) new instance of #GSignondDbusSignonuiAdapter
 */
GSignondDbusSignonuiAdapter * 
gsignond_dbus_signonui_adapter_new ()
{
    GError *err = NULL;
    GSignondDbusSignonuiAdapter *adapter = GSIGNOND_DBUS_SIGNONUI_ADAPTER (
        g_object_new (GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, NULL));
    
    adapter->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        ERR ("Error getting session bus :%s", err->message);
        goto fail;
    }

    adapter->priv->proxy = gsignond_dbus_singlesignonui_proxy_new_sync (adapter->priv->connection,
           G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, 
           SIGNONUI_SERVICE,
           SIGNONUI_OBJECTPATH,
           NULL,
           &err);
    if (err) {
        WARN ("failed to get ui object : %s", err->message);
        goto fail;
    }

    g_signal_connect (adapter->priv->proxy, "refresh", G_CALLBACK (_on_refresh_request), adapter);

    return adapter;

fail:
    if (err) g_error_free (err);
    g_object_unref (adapter);
    return NULL;
}

