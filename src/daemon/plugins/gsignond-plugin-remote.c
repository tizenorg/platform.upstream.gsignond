/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013-2014 Intel Corporation.
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

#include "config.h"

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-plugin-interface.h"
#include "common/gsignond-pipe-stream.h"
#include "daemon/dbus/gsignond-dbus.h"
#include "gsignond-plugin-remote-private.h"
#include "gsignond-plugin-remote.h"

enum
{
    PROP_0,
    PROP_TYPE,
    PROP_MECHANISMS,
    N_PROPERTIES
};

static void
gsignond_plugin_remote_interface_init (
        GSignondPluginInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GSignondPluginRemote, gsignond_plugin_remote,
        G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_PLUGIN,
                gsignond_plugin_remote_interface_init));

#define GSIGNOND_PLUGIN_REMOTE_GET_PRIV(obj) \
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_PLUGIN_REMOTE, \
        GSignondPluginRemotePrivate)

static void
_on_child_down_cb (
        GPid  pid,
        gint  status,
        gpointer data)
{
    g_spawn_close_pid (pid);

    GSignondPluginRemote *plugin = GSIGNOND_PLUGIN_REMOTE (data);

    DBG ("Plugind(%p) with pid (%d) closed with status %d", plugin, pid,
            status);

    plugin->priv->is_plugind_up = FALSE;

}

static void
gsignond_plugin_remote_set_property (
        GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_plugin_remote_get_property (
        GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec)
{
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (object);

    switch (property_id) {
        case PROP_TYPE: {
            g_value_set_string (value,
                                gsignond_dbus_remote_plugin_v1_get_method(self->priv->dbus_plugin_proxy));
            break;
        }
        case PROP_MECHANISMS: {
            g_value_set_boxed (value,
                               gsignond_dbus_remote_plugin_v1_get_mechanisms(self->priv->dbus_plugin_proxy));
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}

static gboolean _check_child_exited(GSignondPluginRemote *self)
{
    if (kill (self->priv->cpid, 0) == 0) {
        WARN ("Plugind has to be killed with SIGKILL");
        kill (self->priv->cpid, SIGKILL);
    }
    return FALSE;
}

static void
gsignond_plugin_remote_dispose (GObject *object)
{
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (object);

    if (self->priv->cpid > 0 && self->priv->is_plugind_up) {
        DBG ("Send SIGTERM to Plugind");
        kill (self->priv->cpid, SIGTERM);
        guint check_id = g_timeout_add(1000, (GSourceFunc)_check_child_exited, self);
        while (self->priv->is_plugind_up)
            g_main_context_iteration(NULL, TRUE);
        g_source_remove(check_id);
        DBG ("Plugind DESTROYED");
    }
    self->priv->cpid = 0;

    if (self->priv->child_watch_id > 0) {
        g_source_remove (self->priv->child_watch_id);
        self->priv->child_watch_id = 0;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }

    if (self->priv->dbus_plugin_proxy) {
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_response);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_response_final);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_store);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_error);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_user_action_required);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_refreshed);
        g_signal_handler_disconnect (self->priv->dbus_plugin_proxy,
                self->priv->signal_status_changed);
        g_object_unref (self->priv->dbus_plugin_proxy);
        self->priv->dbus_plugin_proxy = NULL;
    }

    G_OBJECT_CLASS (gsignond_plugin_remote_parent_class)->dispose (object);
}

static void
gsignond_plugin_remote_finalize (GObject *object)
{
    //GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (object);

    G_OBJECT_CLASS (gsignond_plugin_remote_parent_class)->finalize (object);
}

static void
gsignond_plugin_remote_class_init (GSignondPluginRemoteClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class,
            sizeof (GSignondPluginRemotePrivate));

    object_class->get_property = gsignond_plugin_remote_get_property;
    object_class->set_property = gsignond_plugin_remote_set_property;
    object_class->dispose = gsignond_plugin_remote_dispose;
    object_class->finalize = gsignond_plugin_remote_finalize;

    g_object_class_override_property (object_class, PROP_TYPE, "type");
    g_object_class_override_property (object_class, PROP_MECHANISMS,
            "mechanisms");

}

static void
gsignond_plugin_remote_init (GSignondPluginRemote *self)
{
    self->priv = GSIGNOND_PLUGIN_REMOTE_GET_PRIV(self);

    self->priv->connection = NULL;
    self->priv->dbus_plugin_proxy = NULL;
    self->priv->cpid = 0;

    self->priv->child_watch_id = 0;

    self->priv->is_plugind_up = FALSE;
}

static void
_cancel_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePluginV1 *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN_V1 (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_v1_call_cancel_finish (proxy, res, &error);
    if (error) {
        gsignond_plugin_error (GSIGNOND_PLUGIN(self), error);
        g_error_free (error);
    }
}

static void
gsignond_plugin_remote_cancel (
        GSignondPlugin *plugin)
{
    g_return_if_fail (plugin && GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    gsignond_dbus_remote_plugin_v1_call_cancel (
            self->priv->dbus_plugin_proxy, NULL, _cancel_async_cb, self);
}

static void
_request_initial_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePluginV1 *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN_V1 (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);
    gsignond_dbus_remote_plugin_v1_call_request_initial_finish (proxy,
            res, &error);
    if (error) {
        gsignond_plugin_error (GSIGNOND_PLUGIN(self), error);
        g_error_free (error);
    }
}

static void
gsignond_plugin_remote_request_initial (
    GSignondPlugin *plugin,
    GSignondSessionData *session_data,
    GSignondDictionary *identity_method_cache,
    const gchar *mechanism)
{
    g_return_if_fail (session_data && plugin &&
            GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    GVariant *data = gsignond_dictionary_to_variant (session_data);
    GVariant *cache;
    if (identity_method_cache)
        cache = gsignond_dictionary_to_variant (identity_method_cache);
    else {
        GSignondDictionary* empty_cache = gsignond_dictionary_new();
        cache = gsignond_dictionary_to_variant (empty_cache);
        gsignond_dictionary_unref(empty_cache);
    }
    gsignond_dbus_remote_plugin_v1_call_request_initial (
            self->priv->dbus_plugin_proxy, data, cache, mechanism, NULL,
            _request_initial_async_cb, self);
}

static void
_request_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePluginV1 *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN_V1 (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_v1_call_request_finish (proxy, res, &error);
    if (error) {
        gsignond_plugin_error (GSIGNOND_PLUGIN(self), error);
        g_error_free (error);
    }
}

static void
gsignond_plugin_remote_request (
    GSignondPlugin *plugin,
    GSignondSessionData *session_data)
{
    g_return_if_fail (session_data && plugin &&
            GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    GVariant *data = gsignond_dictionary_to_variant (session_data);
    gsignond_dbus_remote_plugin_v1_call_request (
            self->priv->dbus_plugin_proxy, data, NULL, _request_async_cb, self);
}

static void
_user_action_finished_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePluginV1 *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN_V1 (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_v1_call_user_action_finished_finish (proxy,
            res, &error);
    if (error) {
        gsignond_plugin_error (GSIGNOND_PLUGIN(self), error);
        g_error_free (error);
    }
}

static void
gsignond_plugin_remote_user_action_finished (
    GSignondPlugin *plugin,
    GSignondSignonuiData *signonui_data)
{
    g_return_if_fail (signonui_data && plugin &&
            GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    GVariant *data = gsignond_dictionary_to_variant (signonui_data);
    gsignond_dbus_remote_plugin_v1_call_user_action_finished (
            self->priv->dbus_plugin_proxy, data, NULL,
            _user_action_finished_async_cb, self);
}

static void
_refresh_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePluginV1 *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN_V1 (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_v1_call_refresh_finish (proxy, res, &error);
    if (error) {
        gsignond_plugin_error (GSIGNOND_PLUGIN(self), error);
        g_error_free (error);
    }
}

static void
gsignond_plugin_remote_refresh (
    GSignondPlugin *plugin,
    GSignondSignonuiData *signonui_data)
{
    g_return_if_fail (signonui_data && plugin &&
            GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    GVariant *data = gsignond_dictionary_to_variant (signonui_data);
    gsignond_dbus_remote_plugin_v1_call_refresh (
            self->priv->dbus_plugin_proxy, data, NULL, _refresh_async_cb, self);
}

static void
gsignond_plugin_remote_interface_init (GSignondPluginInterface *iface)
{
    iface->cancel = gsignond_plugin_remote_cancel;
    iface->request_initial = gsignond_plugin_remote_request_initial;
    iface->request = gsignond_plugin_remote_request;
    iface->user_action_finished = gsignond_plugin_remote_user_action_finished;
    iface->refresh = gsignond_plugin_remote_refresh;
}

static void
_response_cb (
        GSignondPluginRemote *self,
        GVariant *session_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSessionData *data = (GSignondSessionData *)
            gsignond_dictionary_new_from_variant (session_data);
    gsignond_plugin_response (GSIGNOND_PLUGIN(self), data);
    gsignond_dictionary_unref (data);
}

static void
_response_final_cb (
        GSignondPluginRemote *self,
        GVariant *session_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSessionData *data = (GSignondSessionData *)
            gsignond_dictionary_new_from_variant (session_data);
    gsignond_plugin_response_final (GSIGNOND_PLUGIN(self), data);
    gsignond_dictionary_unref (data);
}

static void
_store_cb (
        GSignondPluginRemote *self,
        GVariant *session_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSessionData *data = (GSignondSessionData *)
            gsignond_dictionary_new_from_variant (session_data);
    gsignond_plugin_store (GSIGNOND_PLUGIN(self), data);
    gsignond_dictionary_unref (data);
}

static void
_error_cb (
        GSignondPluginRemote *self,
        GVariant *error,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));
    GError *gerror = gsignond_error_new_from_variant (error);
    gsignond_plugin_error (GSIGNOND_PLUGIN(self), gerror);
    g_error_free (gerror);
}

static void
_user_action_required_cb (
        GSignondPluginRemote *self,
        GVariant *ui_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSignonuiData *data = (GSignondSignonuiData *)
            gsignond_dictionary_new_from_variant (ui_data);
    gsignond_plugin_user_action_required (GSIGNOND_PLUGIN(self), data);
    gsignond_dictionary_unref (data);
}

static void
_refreshed_cb(
        GSignondPluginRemote *self,
        GVariant *ui_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSignonuiData *data = (GSignondSignonuiData *)
            gsignond_dictionary_new_from_variant (ui_data);
    gsignond_plugin_refreshed (GSIGNOND_PLUGIN(self), data);
    gsignond_dictionary_unref (data);
}

static void
_status_changed_cb (
        GSignondPluginRemote *self,
        gint status,
        gchar *message,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    gsignond_plugin_status_changed (GSIGNOND_PLUGIN(self),
            (GSignondPluginState)status, message);
}

GSignondPluginRemote *
gsignond_plugin_remote_new (
        const gchar *loader_path,
        const gchar *plugin_type)
{
    GError *error = NULL;
    GPid cpid = 0;
    gchar **argv;
    gint cin_fd, cout_fd;
    GSignondPluginRemote *plugin = NULL;
    GSignondPipeStream *stream = NULL;
    gboolean ret = FALSE;

    /* This guarantees that writes to a pipe will never cause
     * a process terminanation via SIGPIPE, and instead a proper
     * error will be returned */
    signal(SIGPIPE, SIG_IGN);

    /* Spawn child process */
    argv = g_new0 (gchar *, 2 + 1);
    argv[0] = g_strdup(loader_path);
    argv[1] = g_strdup_printf("--load-plugin=%s",plugin_type);
    ret = g_spawn_async_with_pipes (NULL, argv, NULL,
            G_SPAWN_DO_NOT_REAP_CHILD, NULL,
            NULL, &cpid, &cin_fd, &cout_fd, NULL, &error);
    g_strfreev (argv);
    if (ret == FALSE || (kill(cpid, 0) != 0)) {
        DBG ("failed to start plugind: error %s(%d)", 
            error ? error->message : "(null)", ret);
        if (error) g_error_free (error);
        return NULL;
    }

    /* Create dbus plugin object */
    plugin = GSIGNOND_PLUGIN_REMOTE (g_object_new (GSIGNOND_TYPE_PLUGIN_REMOTE,
            NULL));

    plugin->priv->child_watch_id = g_child_watch_add (cpid,
            (GChildWatchFunc)_on_child_down_cb, plugin);
    plugin->priv->cpid = cpid;
    plugin->priv->is_plugind_up = TRUE;

    /* Create dbus connection */
    stream = gsignond_pipe_stream_new (cout_fd, cin_fd, TRUE);
    plugin->priv->connection = g_dbus_connection_new_sync (G_IO_STREAM (stream),
            NULL, G_DBUS_CONNECTION_FLAGS_NONE, NULL, NULL, NULL);
    g_object_unref (stream);

    /* Create dbus proxy */
    plugin->priv->dbus_plugin_proxy =
            gsignond_dbus_remote_plugin_v1_proxy_new_sync (
                    plugin->priv->connection,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,
                    GSIGNOND_PLUGIN_OBJECTPATH,
                    NULL,
                    &error);
    if (error) {
        DBG ("Failed to register object: %s", error->message);
        g_error_free (error);
        g_object_unref (plugin);
        return NULL;
    }
    DBG("'%s' object exported(%p)", GSIGNOND_PLUGIN_OBJECTPATH, plugin);

    plugin->priv->signal_response = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "response",
            G_CALLBACK (_response_cb), plugin);
    plugin->priv->signal_response_final = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "response-final",
            G_CALLBACK(_response_final_cb), plugin);
    plugin->priv->signal_store = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "store",
            G_CALLBACK(_store_cb), plugin);
    plugin->priv->signal_error = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "error",
            G_CALLBACK(_error_cb), plugin);
    plugin->priv->signal_user_action_required = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "user-action-required",
            G_CALLBACK(_user_action_required_cb), plugin);
    plugin->priv->signal_refreshed = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "refreshed",
            G_CALLBACK(_refreshed_cb), plugin);
    plugin->priv->signal_status_changed = g_signal_connect_swapped (
            plugin->priv->dbus_plugin_proxy, "status-changed",
            G_CALLBACK(_status_changed_cb), plugin);

    return plugin;
}

