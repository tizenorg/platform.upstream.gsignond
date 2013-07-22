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

#include "gsignond-signonui-proxy.h"
#include <gsignond/gsignond-log.h>
#include <gsignond/gsignond-signonui.h>
#include "dbus/gsignond-dbus-signonui-adapter.h"

static void _process_next_request (GSignondSignonuiProxy *proxy);
static void _on_refresh_request (GSignondSignonuiProxy *proxy,
                                 gchar *request_id, gpointer userdata);

typedef struct {
    GObject *caller;
    GSignondSignonuiData *ui_data;
    GSignondSignonuiProxyQueryDialogCb cb;
    GSignondSignonuiProxyRefreshCb refresh_cb;
    gpointer userdata;
} _UIQueryRequest;

typedef struct {
    GSignondSignonuiProxyRefreshDialogCb cb;
    gpointer userdata;
} _UIRefreshRequest;

typedef struct {
    GSignondSignonuiProxyCancelRequestCb cb;
    gpointer userdata;
} _UICancelRequest;

struct _GSignondSignonuiProxyPrivate
{
    GSignondDbusSignonuiAdapter *signonui;
    guint signonui_timer_id;
    _UIQueryRequest *active_request; /* Active dialog */
    GQueue *request_queue;           /* request queue */
    gboolean is_idle;
};

G_DEFINE_TYPE (GSignondSignonuiProxy, gsignond_signonui_proxy, G_TYPE_OBJECT);
#define GSIGNOND_SIGNONUI_PROXY_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_SIGNONUI_PROXY, GSignondSignonuiProxyPrivate)

static _UIQueryRequest *
_ui_query_request_new (GObject *caller,
                       GSignondSignonuiData *ui_data,
                       GSignondSignonuiProxyQueryDialogCb cb,
                       GSignondSignonuiProxyRefreshCb refresh_cb,
                       gpointer userdata)
{
    _UIQueryRequest *req = g_new0(_UIQueryRequest, 1);

    req->caller = caller;
    req->ui_data = gsignond_signonui_data_ref (ui_data);
    req->cb = cb;
    req->refresh_cb = refresh_cb;
    req->userdata = userdata;

    return req;
}

static _UIRefreshRequest *
_ui_refresh_request_new (GSignondSignonuiProxyRefreshDialogCb cb, gpointer userdata)
{
    _UIRefreshRequest *req = g_new0(_UIRefreshRequest, 1);
        
    req->cb = cb;
    req->userdata = userdata;

    return req;
}

static _UICancelRequest *
_ui_cancel_request_new (GSignondSignonuiProxyCancelRequestCb cb, gpointer userdata)
{
    _UICancelRequest *req = g_new0(_UICancelRequest, 1);

    req->cb = cb;
    req->userdata = userdata;

    return req;
}

static void
_ui_query_request_free (_UIQueryRequest *req)
{
    if (!req) return;
    if (req->ui_data) gsignond_signonui_data_unref (req->ui_data);
    g_free (req);
}

static void
_dispose (GObject *object)
{
    GSignondSignonuiProxy *self = GSIGNOND_SIGNONUI_PROXY (object);

    if (self->priv->signonui_timer_id) {
        g_source_remove (self->priv->signonui_timer_id);
        self->priv->signonui_timer_id = 0;
    }
    if (self->priv->signonui) {
        g_object_unref (self->priv->signonui);
        self->priv->signonui = NULL;
    }

    G_OBJECT_CLASS (gsignond_signonui_proxy_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondSignonuiProxy *self = GSIGNOND_SIGNONUI_PROXY (object);

    if (self->priv->request_queue) {
        g_queue_free_full (self->priv->request_queue, (GDestroyNotify)_ui_query_request_free);
        self->priv->request_queue = NULL;
    }

    G_OBJECT_CLASS (gsignond_signonui_proxy_parent_class)->finalize (object);
}

static void
gsignond_signonui_proxy_class_init (GSignondSignonuiProxyClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (gobject_class, sizeof (GSignondSignonuiProxyPrivate));
    
    gobject_class->dispose = _dispose;
    gobject_class->finalize = _finalize;
}

static void
gsignond_signonui_proxy_init (GSignondSignonuiProxy *proxy)
{
    proxy->priv = GSIGNOND_SIGNONUI_PROXY_GET_PRIV (proxy);

    proxy->priv->signonui = NULL;
    proxy->priv->active_request = NULL;
    proxy->priv->request_queue = g_queue_new ();
    proxy->priv->is_idle = TRUE;
}

static void
_on_refresh_request (GSignondSignonuiProxy *proxy, gchar *request_id, gpointer userdata)
{
    _UIQueryRequest *req = proxy->priv->active_request;

    if (req && !g_strcmp0 (G_OBJECT_TYPE_NAME (req->caller), request_id)) {
       if (req->refresh_cb) req->refresh_cb(req->ui_data, req->userdata);
    }
    else {
        WARN("UI-Error: unhandled refresh request");
    }
}

static void
_query_dialog_cb_internal (GSignondSignonuiProxy *proxy, GSignondSignonuiData *ui_data, GError *error)
{
    _UIQueryRequest *req = proxy->priv->active_request;

    if (req && req->cb) {
        req->cb (ui_data, error, req->userdata);
    }
    else if (error) {
        WARN ("UI-Error: %s", error->message);
        g_error_free (error);
    }
    if (ui_data) gsignond_signonui_data_unref (ui_data);

    _ui_query_request_free (req);

    proxy->priv->active_request = NULL;

    _process_next_request (proxy);
}

static void
_query_dialog_cb (GVariant *reply, GError *error, gpointer user_data)
{
    GSignondSignonuiProxy *proxy = GSIGNOND_SIGNONUI_PROXY (user_data);
    GSignondSignonuiData *ui_data = reply ? gsignond_signonui_data_new_from_variant (reply) : NULL;

     _query_dialog_cb_internal (proxy, ui_data, error);
}

static gboolean
_close_ui_connection (gpointer data)
{
    GSignondSignonuiProxy *proxy = GSIGNOND_SIGNONUI_PROXY(data);
    g_return_val_if_fail (proxy, FALSE);

    proxy->priv->signonui_timer_id = 0;

    g_clear_object (&proxy->priv->signonui);

    return FALSE;
}

static void
_process_next_request (GSignondSignonuiProxy *proxy)
{
    _UIQueryRequest *req = g_queue_pop_head (proxy->priv->request_queue);
    GVariant *params = NULL;

    if (!req) {
        proxy->priv->is_idle = TRUE;
        proxy->priv->active_request = NULL;
        proxy->priv->signonui_timer_id = 
            g_timeout_add_seconds (10, (GSourceFunc)_close_ui_connection, proxy);
        return;
    }
    else {
        if (proxy->priv->signonui_timer_id) {
            g_source_remove (proxy->priv->signonui_timer_id);
            proxy->priv->signonui_timer_id = 0;
        }
        if (!proxy->priv->signonui)
            proxy->priv->signonui = gsignond_dbus_signonui_adapter_new ();
        if (proxy->priv->signonui)
            g_signal_connect_swapped (proxy->priv->signonui, "refresh",
                    G_CALLBACK(_on_refresh_request), proxy);
        else {
            GSignondSignonuiData *reply = gsignond_signonui_data_new ();
            gsignond_signonui_data_set_query_error(reply, SIGNONUI_ERROR_NO_SIGNONUI);
            _query_dialog_cb_internal (proxy, reply, NULL);
        }
    }

    proxy->priv->active_request = req;

    /* update request id */
    gsignond_signonui_data_set_request_id (req->ui_data, G_OBJECT_TYPE_NAME(req->caller));
    params =  gsignond_signonui_data_to_variant(req->ui_data) ;
    gsignond_dbus_signonui_adapter_query_dialog (proxy->priv->signonui, 
            params, _query_dialog_cb, proxy);

    proxy->priv->is_idle = FALSE;
}

gboolean
gsignond_signonui_proxy_query_dialog (GSignondSignonuiProxy *proxy,
                                      GObject *caller,
                                      GSignondSignonuiData *ui_data,
                                      GSignondSignonuiProxyQueryDialogCb cb,
                                      GSignondSignonuiProxyRefreshCb refresh_cb,
                                      gpointer userdata)
{

    g_queue_push_tail (proxy->priv->request_queue, 
            _ui_query_request_new (caller, ui_data, cb, refresh_cb, userdata));

    if (proxy->priv->is_idle) _process_next_request (proxy);

    return TRUE;
}

static void
_refresh_dialog_cb (GError *error, gpointer user_data)
{
    _UIRefreshRequest *req = (_UIRefreshRequest *)user_data;

    if (req && req->cb){
        req->cb(error, req->userdata);
    } else if (error) {
        WARN ("UI-Error : %s", error->message);
        g_error_free (error);
    }

    g_free (req);
}

gboolean
gsignond_signonui_proxy_refresh_dialog (GSignondSignonuiProxy *proxy,
                                        GObject *caller,
                                        GSignondSignonuiData *ui_data,
                                        GSignondSignonuiProxyRefreshDialogCb cb,
                                        gpointer userdata)
{
    if (proxy->priv->active_request
        && proxy->priv->active_request->caller == caller) {
        _UIRefreshRequest *req = _ui_refresh_request_new (cb, userdata);

        gsignond_signonui_data_set_request_id (ui_data, G_OBJECT_TYPE_NAME(caller));
        gsignond_dbus_signonui_adapter_refresh_dialog (proxy->priv->signonui,
                gsignond_signonui_data_to_variant (ui_data), _refresh_dialog_cb, req);

        return TRUE;
    }

    return FALSE;
}

static void
_cancel_request_cb (GError *error, gpointer user_data)
{
    _UICancelRequest *req = (_UICancelRequest*)user_data;

    if (req && req->cb)
        req->cb (error, req->userdata);
    else if (error) {
        WARN ("UI-Error : %s", error->message);
        g_error_free (error);
    }

    g_free (req);
}

static gint
_find_request_by_caller (gconstpointer a, gconstpointer b)
{
    _UIQueryRequest *req = (_UIQueryRequest *)a;
    
    return (req && req->caller == b) ? 0 : 1;
}

gboolean
gsignond_signonui_proxy_cancel_request (GSignondSignonuiProxy *proxy,
                                        GObject *caller, 
                                        GSignondSignonuiProxyCancelRequestCb cb,
                                        gpointer userdata)
{
    GList *element = NULL;
    _UIQueryRequest *req = NULL;
    /* if no active request to cacel */
    if (!proxy->priv->active_request) return FALSE;

    /* cancel active request */
    if (proxy->priv->active_request->caller == caller) {
        _UICancelRequest *req = _ui_cancel_request_new (cb, userdata);
        gsignond_dbus_signonui_adapter_cancel_request (proxy->priv->signonui,
            G_OBJECT_TYPE_NAME (caller), _cancel_request_cb, req);
        return TRUE;
    }

    /* cancel pending request */
    element = g_queue_find_custom (proxy->priv->request_queue, caller, _find_request_by_caller);

    if (!element) return FALSE;
    req = element->data;

    if (req->cb) {
        GSignondSignonuiData *reply = gsignond_signonui_data_new ();
        gsignond_signonui_data_set_query_error(reply, SIGNONUI_ERROR_CANCELED);

        req->cb (reply, NULL, req->userdata);
    }

    if (cb) cb(NULL, userdata);

    return TRUE;
}

GSignondSignonuiProxy *
gsignond_signonui_proxy_new ()
{
    return g_object_new (GSIGNOND_TYPE_SIGNONUI_PROXY, NULL);
}
