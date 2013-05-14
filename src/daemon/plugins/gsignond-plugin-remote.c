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

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-pipe-stream.h"
#include "gsignond/gsignond-plugin-interface.h"
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

#define GSIGNOND_PLUGIND_NAME "gsignond-plugind"

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
            if (!self->priv->plugin_type) {
                GError *error = NULL;
                gsignond_dbus_remote_plugin_call_get_info_sync (
                    self->priv->dbus_plugin_proxy, &self->priv->plugin_type,
                    &self->priv->plugin_mechanisms, NULL, &error);
                if (error) {
                    DBG ("Plugin type retrieval error :: %s", error->message);
                    g_error_free (error);
                    if (self->priv->plugin_type) {
                        g_free (self->priv->plugin_type);
                        self->priv->plugin_type = NULL;
                    }
                }
            }
            g_value_set_string (value, self->priv->plugin_type);
            break;
        }
        case PROP_MECHANISMS: {
            if (!self->priv->plugin_mechanisms) {
                GError *error = NULL;
                gsignond_dbus_remote_plugin_call_get_info_sync (
                    self->priv->dbus_plugin_proxy, &self->priv->plugin_type,
                    &self->priv->plugin_mechanisms, NULL, &error);
                if (error) {
                    DBG ("Plugin mechanisms retrieval error :: %s",
                            error->message);
                    g_error_free (error);
                    if (self->priv->plugin_mechanisms) {
                        g_strfreev (self->priv->plugin_mechanisms);
                        self->priv->plugin_mechanisms = NULL;
                    }
                }
            }
            g_value_set_boxed (value, self->priv->plugin_mechanisms);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}

static void
gsignond_plugin_remote_dispose (GObject *object)
{
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (object);

    if (self->priv->err_watch_ch) {
        g_io_channel_shutdown (self->priv->err_watch_ch, FALSE, NULL);
        g_io_channel_unref (self->priv->err_watch_ch);
        g_source_remove (self->priv->err_watch_id);
        self->priv->err_watch_ch = NULL;
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

    if (self->priv->cpid > 0) {
        if (self->priv->child_watch_id) {
            g_source_remove (self->priv->child_watch_id);
            self->priv->child_watch_id = 0;
        }
        kill (self->priv->cpid, SIGTERM);
        self->priv->cpid = 0;
    }

    G_OBJECT_CLASS (gsignond_plugin_remote_parent_class)->dispose (object);
}

static void
gsignond_plugin_remote_finalize (GObject *object)
{
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (object);

    if (self->priv->plugin_type) {
        g_free (self->priv->plugin_type);
        self->priv->plugin_type = NULL;
    }

    if (self->priv->plugin_mechanisms) {
        g_strfreev (self->priv->plugin_mechanisms);
        self->priv->plugin_mechanisms = NULL;
    }

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
    self->priv->err_watch_ch = NULL;
    self->priv->plugin_type = NULL;
    self->priv->plugin_mechanisms = NULL;
    self->priv->cpid = 0;
    self->priv->child_watch_id = 0;

}

static void
_cancel_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePlugin *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_call_cancel_finish (proxy, res, &error);
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

    gsignond_dbus_remote_plugin_call_cancel (
            self->priv->dbus_plugin_proxy, NULL, _cancel_async_cb, self);
}

static void
_request_initial_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePlugin *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);
    gsignond_dbus_remote_plugin_call_request_initial_finish (proxy,
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
    const gchar *mechanism)
{
    g_return_if_fail (session_data && plugin &&
            GSIGNOND_IS_PLUGIN_REMOTE (plugin));
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (plugin);

    GVariant *data = gsignond_dictionary_to_variant (session_data);
    gsignond_dbus_remote_plugin_call_request_initial (
            self->priv->dbus_plugin_proxy, data, mechanism, NULL,
            _request_initial_async_cb, self);
}

static void
_request_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePlugin *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_call_request_finish (proxy, res, &error);
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
    gsignond_dbus_remote_plugin_call_request (
            self->priv->dbus_plugin_proxy, data, NULL, _request_async_cb, self);
}

static void
_user_action_finished_async_cb (
        GObject *object,
        GAsyncResult *res,
        gpointer user_data)
{
    GError *error = NULL;
    GSignondDbusRemotePlugin *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_call_user_action_finished_finish (proxy,
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

    GVariant *data = gsignond_signonui_data_to_variant (signonui_data);
    gsignond_dbus_remote_plugin_call_user_action_finished (
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
    GSignondDbusRemotePlugin *proxy = GSIGNOND_DBUS_REMOTE_PLUGIN (object);
    GSignondPluginRemote *self = GSIGNOND_PLUGIN_REMOTE (user_data);

    gsignond_dbus_remote_plugin_call_refresh_finish (proxy, res, &error);
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

    GVariant *data = gsignond_signonui_data_to_variant (signonui_data);
    gsignond_dbus_remote_plugin_call_refresh (
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
            gsignond_signonui_data_new_from_variant (ui_data);
    gsignond_plugin_user_action_required (GSIGNOND_PLUGIN(self), data);
    gsignond_signonui_data_unref (data);
}

static void
_refreshed_cb(
        GSignondPluginRemote *self,
        GVariant *ui_data,
        gpointer user_data)
{
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_REMOTE (self));

    GSignondSignonuiData *data = (GSignondSignonuiData *)
            gsignond_signonui_data_new_from_variant (ui_data);
    gsignond_plugin_refreshed (GSIGNOND_PLUGIN(self), data);
    gsignond_signonui_data_unref (data);
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

static gboolean
_error_watch_cb (
        GIOChannel *channel,
        GIOCondition condition,
        gpointer data)
{

    GSignondPluginRemote *plugin = (GSignondPluginRemote*)data;

    if (condition == G_IO_HUP || condition == G_IO_NVAL) {
        g_io_channel_shutdown (plugin->priv->err_watch_ch, FALSE, NULL);
        g_io_channel_unref (plugin->priv->err_watch_ch);
        plugin->priv->err_watch_ch = NULL;
        g_source_remove (plugin->priv->err_watch_id);
        DBG ("Plugind (%s) is down",
                plugin->priv->plugin_type ? plugin->priv->plugin_type : "");

        if (plugin->priv->cpid > 0 &&
            kill (plugin->priv->cpid, 0) != 0) {
            if (plugin->priv->child_watch_id) {
                g_source_remove (plugin->priv->child_watch_id);
                plugin->priv->child_watch_id = 0;
            }
            plugin->priv->cpid = 0;
        }
        return FALSE;
    }

    if (g_io_channel_get_flags (channel) & G_IO_FLAG_IS_READABLE) {
        gchar * string = NULL;
        GError *error = NULL;
        gsize bytes_read = 0;
        gboolean keep_error_source = TRUE;
        GIOStatus status = g_io_channel_read_line (channel, &string,
                &bytes_read, NULL, &error);
        if (status == G_IO_STATUS_NORMAL && bytes_read > 0 && error == NULL) {
            DBG ("(%s) %s",plugin->priv?(plugin->priv->plugin_type ?
                    plugin->priv->plugin_type : "NULL"):"NULL", string);
        }
        if (string) {
            g_free (string);
        }
        keep_error_source = (bytes_read > 0 && error == NULL);
        if (error) {
            g_error_free (error);
        }
        if (!keep_error_source) {
            DBG ("Removing error source- bytes_read %d, error %p",
                    (gint)bytes_read, error?error:NULL);
        }
        return keep_error_source;
    }

    return TRUE;
}

static void
_child_watch_cb (
        GPid  pid,
        gint  status,
        gpointer data)
{
    GSignondPluginRemote *plugin = (GSignondPluginRemote*)data;
    DBG ("Plugin process (%s) with pid (%d) closed",
            plugin->priv->plugin_type ? plugin->priv->plugin_type : "", pid);
    g_spawn_close_pid (pid);
    g_source_remove (plugin->priv->child_watch_id);
    plugin->priv->child_watch_id = 0;
}

GSignondPluginRemote *
gsignond_plugin_remote_new (
        GSignondConfig *config,
        const gchar *plugin_type)
{
    gchar *object_path = NULL;
    GError *error = NULL;
    GPid cpid = 0;
    gchar **argv;
    gint cin_fd, cout_fd, cerr_fd;
    GSignondPluginRemote *plugin = NULL;
    GSignondPipeStream *stream = NULL;
    gboolean ret = FALSE;

    /* This guarantees that writes to a pipe will never cause
     * a process terminanation via SIGPIPE, and instead a proper
     * error will be returned */
    signal(SIGPIPE, SIG_IGN);

    /* Spawn child process */
    argv = g_malloc0 ((3 + 1) * sizeof (gchar *));
    argv[0] = g_build_filename (gsignond_config_get_string (config,
            GSIGNOND_CONFIG_GENERAL_BIN_DIR), GSIGNOND_PLUGIND_NAME, NULL);
    argv[1] = g_module_build_path (gsignond_config_get_string (config,
            GSIGNOND_CONFIG_GENERAL_PLUGINS_DIR), plugin_type);
    argv[2] = g_strdup(plugin_type);
    ret = g_spawn_async_with_pipes (NULL, argv, NULL,
            G_SPAWN_DO_NOT_REAP_CHILD, NULL,
            NULL, &cpid, &cin_fd, &cout_fd, &cerr_fd, &error);
    g_strfreev (argv);
    if (ret == FALSE || (kill(cpid, 0) != 0)) {
        DBG ("failed to start plugind: ret(%d)", ret);
        if (error) g_error_free (error);
        return NULL;
    }
    /* Create dbus plugin object */
    plugin = GSIGNOND_PLUGIN_REMOTE (g_object_new (GSIGNOND_TYPE_PLUGIN_REMOTE,
            NULL));

    plugin->priv->child_watch_id = g_child_watch_add (cpid,
            (GChildWatchFunc)_child_watch_cb, plugin);
    plugin->priv->cpid = cpid;

    object_path = g_strdup_printf ("%s_%s", GSIGNOND_PLUGIN_OBJECTPATH,
            plugin_type);

    /* Create dbus connection */
    stream = gsignond_pipe_stream_new (cout_fd, cin_fd, TRUE);
    plugin->priv->connection = g_dbus_connection_new_sync (G_IO_STREAM (stream),
            NULL, G_DBUS_CONNECTION_FLAGS_NONE, NULL, NULL, NULL);
    g_object_unref (stream);

    /* Create dbus proxy */
    plugin->priv->dbus_plugin_proxy =
            gsignond_dbus_remote_plugin_proxy_new_sync (
                    plugin->priv->connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                    NULL,
                    object_path,
                    NULL,
                    &error);
    if (error) {
        DBG ("Failed to register object: %s", error->message);
        g_error_free (error);
        g_free (object_path);
        g_object_unref (plugin);
        return NULL;
    }
    DBG("'%s' object exported(%p)", object_path, plugin);
    g_free (object_path);

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

    /* Create watch for error messages */
    plugin->priv->err_watch_ch = g_io_channel_unix_new (cerr_fd);
    plugin->priv->err_watch_id = g_io_add_watch (plugin->priv->err_watch_ch,
            G_IO_IN | G_IO_HUP, (GIOFunc)_error_watch_cb, plugin);
    g_io_channel_set_close_on_unref (plugin->priv->err_watch_ch, TRUE);
    g_io_channel_set_flags (plugin->priv->err_watch_ch, G_IO_FLAG_NONBLOCK,
            NULL);

    return plugin;
}

