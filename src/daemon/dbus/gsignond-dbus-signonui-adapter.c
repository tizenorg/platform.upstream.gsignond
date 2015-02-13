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
#include "gsignond/gsignond-log.h"
#include "gsignond-dbus.h"

struct _GSignondDbusSignonuiAdapterPrivate
{
    GDBusConnection *connection;
    GDBusProxy *proxy;
    gulong connection_close_signal_id;
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

    if (self->priv->connection_close_signal_id) {
        g_signal_handler_disconnect (self->priv->connection, self->priv->connection_close_signal_id);
        self->priv->connection_close_signal_id = 0;
    }

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
    self->priv->connection_close_signal_id = 0;
}

static void
_on_proxy_signal (GSignondDbusSignonuiAdapter *adapter,
                  gchar *sender_name,
                  gchar *signal_name,
                  GVariant *params,
                  gpointer user_data)
{
    gchar *request_id = NULL;

    g_return_if_fail (adapter && signal_name && params);

    /* Ignore other than 'refresh' signal */
    if (g_strcmp0(signal_name, "refresh") != 0) return ;

    if (!g_variant_is_of_type (params, G_VARIANT_TYPE_TUPLE)) {
        WARN ("Expected 'tuple' type but got '%s' type", 
                    g_variant_get_type_string (params));
        return ;
    }

    g_variant_get (params, "(s)", &request_id);

    if (request_id) {
        g_signal_emit (adapter, _signals[SIG_REFRESH], 0, request_id);
        g_free (request_id);
    }
}

static void
_on_connection_closed (GSignondDbusSignonuiAdapter *adapter,
                       gboolean remote_peer_vanished, 
                       GError *error,
                       gpointer user_data)
{
    g_return_if_fail (adapter);

    DBG("UI Connection closed...");

    g_signal_handler_disconnect (adapter->priv->connection, adapter->priv->connection_close_signal_id);
    adapter->priv->connection_close_signal_id = 0;

    g_clear_object (&adapter->priv->connection);
    g_clear_object (&adapter->priv->proxy);
}

static gboolean
_setup_ui_connection (GSignondDbusSignonuiAdapter *adapter)
{
    GError *err = NULL;
    GVariant *reply = NULL;
    gchar *ui_server_address = NULL;
    GDBusConnection *session_bus = NULL;

    g_return_val_if_fail (adapter, FALSE);

    if (adapter->priv->connection) return TRUE;

    session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        WARN ("Error getting session bus :%s", err->message);
        goto fail;
    }

    reply = g_dbus_connection_call_sync (session_bus, SIGNONUI_SERVICE, SIGNONUI_OBJECTPATH,
                SIGNONUI_IFACE, "getBusAddress", g_variant_new ("()"), 
                G_VARIANT_TYPE_TUPLE, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (!reply) {
        WARN ("Failed to get signon ui bus address : %s", err ? err->message : "unknown");
        goto fail;
    }

    g_variant_get(reply, "(s)", &ui_server_address);

    DBG ("Connecting to UI Server at : %s", ui_server_address);

    adapter->priv->connection = g_dbus_connection_new_for_address_sync (ui_server_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, NULL, NULL, &err);
    g_free (ui_server_address);
    if (err) {
        WARN ("Failed to connect UI server at address '%s' : %s", ui_server_address,
                err->message);
        goto fail;
    }

    adapter->priv->connection_close_signal_id = 
            g_signal_connect_swapped (adapter->priv->connection, 
                "closed", G_CALLBACK(_on_connection_closed), adapter);

    adapter->priv->proxy = g_dbus_proxy_new_sync (adapter->priv->connection,
           G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
           NULL,
           NULL,
           SIGNONUI_DIALOG_OBJECTPATH,
           SIGNONUI_DIALOG_IFACE,
           NULL,
           &err);
    if (err) {
        WARN ("failed to get ui object : %s", err->message);
        goto fail;
    }

    g_signal_connect_swapped (adapter->priv->proxy, "g-signal", G_CALLBACK (_on_proxy_signal), adapter);

    return TRUE;
fail:
    if (err) g_error_free (err);
    return FALSE;
}

static void
_on_query_dialog_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    GVariant *reply = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    reply = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);

    if (info) {
        if (info->cb) {
            GVariant *out_params = NULL;

            if (!error) {
                g_variant_get (reply, "(@a{sv})", &out_params);
            }
            ((GSignondDbusSignonuiQueryDialogCb)info->cb) (out_params, error, info->data);
            if(out_params) g_variant_unref (out_params);
        }
        g_object_unref (info->adapter);
        g_slice_free (_SignonuiDbusInfo, info);
    }
    else if (error) g_error_free (error);
    if (reply) g_variant_unref (reply);
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
    if (!adapter->priv->proxy && !_setup_ui_connection(adapter)) {
        WARN ("Failed to setup ui connection");
        return FALSE;
    }
    _SignonuiDbusInfo *info = g_slice_new0 (_SignonuiDbusInfo);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;

    g_dbus_proxy_call (adapter->priv->proxy, "queryDialog",
            g_variant_new ("(@a{sv})", params), G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL,
            _on_query_dialog_ready, (gpointer)info);

    return TRUE;
}

static void
_on_refresh_dialog_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    GVariant *reply = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    reply = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);

    if (info) {
        if (info->cb)
            ((GSignondDbusSignonuiRefreshDialogCb)info->cb) (error, info->data);
        g_object_unref (info->adapter);
        g_slice_free (_SignonuiDbusInfo, info);
    }
    else if (error) g_error_free (error);
    if (reply) g_variant_unref (reply);
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
    if (!adapter->priv->proxy && !_setup_ui_connection(adapter)) {
        WARN ("Failed to setup ui connection");
        return FALSE;
    }

    _SignonuiDbusInfo *info = g_slice_new0 (_SignonuiDbusInfo);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;

    g_dbus_proxy_call (adapter->priv->proxy, "refreshDialog",
            g_variant_new ("(a{sv})", params), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
            _on_refresh_dialog_ready, (gpointer)info);

    return TRUE;
}

static void
_on_cancel_request_ready (GObject *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    GVariant *reply = NULL;
    _SignonuiDbusInfo *info = (_SignonuiDbusInfo *)user_data;

    reply = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);

    if (info) {
        if (info->cb)
            ((GSignondDbusSignonuiCancelRequestCb)info->cb) (error, info->data);
        g_object_unref (info->adapter);
        g_slice_free (_SignonuiDbusInfo, info);
    }
    else if (error) g_error_free (error);

    if (reply) g_variant_unref (reply);
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
    if (!adapter->priv->proxy && !_setup_ui_connection(adapter)) {
        WARN ("Failed to setup ui connection");
        return FALSE;
    }
    _SignonuiDbusInfo *info = g_slice_new0 (_SignonuiDbusInfo);

    info->adapter = g_object_ref (adapter);
    info->cb = callback;
    info->data = user_data;

    g_dbus_proxy_call (adapter->priv->proxy, "cancelUiRequest",
            g_variant_new ("(s)", request_id), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
            _on_cancel_request_ready, (gpointer)info);

    return TRUE;
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
    GSignondDbusSignonuiAdapter *adapter = GSIGNOND_DBUS_SIGNONUI_ADAPTER (
        g_object_new (GSIGNOND_TYPE_DBUS_SIGNONUI_ADAPTER, NULL));

    if (!_setup_ui_connection (adapter)) {
        g_object_unref (adapter);
        return NULL;
    }

    return adapter;
}
