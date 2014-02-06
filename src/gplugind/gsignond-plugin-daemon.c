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

#include "gsignond/gsignond-plugin-interface.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "common/gsignond-plugin-loader.h"
#include "common/gsignond-pipe-stream.h"
#include "daemon/dbus/gsignond-dbus-remote-plugin-gen.h"
#include "daemon/dbus/gsignond-dbus.h"
#include "gsignond-plugin-daemon.h"

struct _GSignondPluginDaemonPrivate
{
    GDBusConnection   *connection;
    GSignondDbusRemotePlugin *dbus_remote_plugin;
    GSignondPlugin *plugin;
    gchar *plugin_type;
};

G_DEFINE_TYPE (GSignondPluginDaemon, gsignond_plugin_daemon, G_TYPE_OBJECT)


#define GSIGNOND_PLUGIN_DAEMON_GET_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_PLUGIN_DAEMON,\
            GSignondPluginDaemonPrivate)

static void
_dispose (GObject *object)
{
    GSignondPluginDaemon *self = GSIGNOND_PLUGIN_DAEMON (object);

    if (self->priv->dbus_remote_plugin) {
        g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (
                self->priv->dbus_remote_plugin));
        g_object_unref (self->priv->dbus_remote_plugin);
        self->priv->dbus_remote_plugin = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }

    if (self->priv->plugin) {
        g_object_unref (self->priv->plugin);
        self->priv->plugin = NULL;
    }

    G_OBJECT_CLASS (gsignond_plugin_daemon_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondPluginDaemon *self = GSIGNOND_PLUGIN_DAEMON (object);

    if (self->priv->plugin_type) {
        g_free (self->priv->plugin_type);
        self->priv->plugin_type = NULL;
    }

    G_OBJECT_CLASS (gsignond_plugin_daemon_parent_class)->finalize (object);
}

static void
gsignond_plugin_daemon_class_init (
        GSignondPluginDaemonClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (
            GSignondPluginDaemonPrivate));

    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

}

static void
gsignond_plugin_daemon_init (
        GSignondPluginDaemon *self)
{
    self->priv = GSIGNOND_PLUGIN_DAEMON_GET_PRIV(self);
    self->priv->connection = NULL;
    self->priv->dbus_remote_plugin = NULL;
    self->priv->plugin_type = NULL;
    self->priv->plugin = NULL;
}

static void
_on_connection_closed (
        GDBusConnection *connection,
        gboolean         remote_peer_vanished,
        GError          *error,
        gpointer         user_data)
{
    GSignondPluginDaemon *daemon = GSIGNOND_PLUGIN_DAEMON (user_data);

    g_signal_handlers_disconnect_by_func (connection, _on_connection_closed,
            user_data);
    DBG("dbus connection(%p) closed (peer vanished : %d)", connection,
            remote_peer_vanished);
    if (error) {
       DBG("...reason : %s", error->message);
    }
    g_object_unref (daemon);
}

static gboolean
_handle_cancel_from_dbus (
        GSignondPluginDaemon *self,
        GDBusMethodInvocation *invocation,
        gpointer user_data)
{
    DBG ("");
    g_return_val_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self), FALSE);
    gsignond_dbus_remote_plugin_complete_cancel (self->priv->dbus_remote_plugin,
            invocation);

    gsignond_plugin_cancel (self->priv->plugin);
    return TRUE;
}

static gboolean
_handle_request_from_dbus (
        GSignondPluginDaemon *self,
        GDBusMethodInvocation *invocation,
        const GVariant *session_data,
        gpointer user_data)
{
    DBG ("");
    g_return_val_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self), FALSE);

    gsignond_dbus_remote_plugin_complete_request (
            self->priv->dbus_remote_plugin, invocation);

    GSignondSessionData *data = (GSignondSessionData *)
            gsignond_dictionary_new_from_variant ((GVariant *)session_data);
    gsignond_plugin_request (self->priv->plugin, data);
    gsignond_dictionary_unref (data);
    return TRUE;
}

static void
_handle_response_final_from_plugin (
        GSignondPluginDaemon *self,
        GSignondSessionData *session_data,
        gpointer user_data);

static gboolean
_handle_request_initial_from_dbus (
        GSignondPluginDaemon *self,
        GDBusMethodInvocation *invocation,
        const GVariant *session_data,
        const GVariant *identity_method_cache,
        const gchar *mechanism,
        gpointer user_data)
{
    DBG ("");
    g_return_val_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self), FALSE);

    gsignond_dbus_remote_plugin_complete_request_initial (
            self->priv->dbus_remote_plugin, invocation);

    GSignondSessionData *data = (GSignondSessionData *)
            gsignond_dictionary_new_from_variant ((GVariant *)session_data);
    GSignondSessionData *cache = 
            gsignond_dictionary_new_from_variant ((GVariant *)identity_method_cache);
    gsignond_plugin_request_initial (self->priv->plugin, data, cache, mechanism);
    gsignond_dictionary_unref (data);
    gsignond_dictionary_unref (cache);

    return TRUE;
}

static gboolean
_handle_user_action_finished_from_dbus (
        GSignondPluginDaemon *self,
        GDBusMethodInvocation *invocation,
        const GVariant *ui_data,
        gpointer user_data)
{
    DBG ("");
    g_return_val_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self), FALSE);

    gsignond_dbus_remote_plugin_complete_user_action_finished (
            self->priv->dbus_remote_plugin, invocation);

    GSignondSignonuiData *data = (GSignondSignonuiData *)
            gsignond_dictionary_new_from_variant ((GVariant *)ui_data);
    gsignond_plugin_user_action_finished (self->priv->plugin, data);
    gsignond_dictionary_unref (data);
    return TRUE;
}

static gboolean
_handle_refresh_from_dbus (
        GSignondPluginDaemon *self,
        GDBusMethodInvocation *invocation,
        const GVariant *ui_data,
        gpointer user_data)
{
    DBG ("");
    g_return_val_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self), FALSE);

    gsignond_dbus_remote_plugin_complete_refresh (
            self->priv->dbus_remote_plugin, invocation);

    GSignondSignonuiData *data = (GSignondSignonuiData *)
            gsignond_dictionary_new_from_variant ((GVariant *)ui_data);
    gsignond_plugin_refresh (self->priv->plugin, data);
    gsignond_dictionary_unref (data);
    return TRUE;
}

static void
_handle_response_from_plugin (
        GSignondPluginDaemon *self,
        GSignondSessionData *session_data,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *data = gsignond_dictionary_to_variant (
            (GSignondDictionary *)session_data);
    gsignond_dbus_remote_plugin_emit_response (self->priv->dbus_remote_plugin,
            data);
}

static void
_handle_response_final_from_plugin (
        GSignondPluginDaemon *self,
        GSignondSessionData *session_data,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *data = gsignond_dictionary_to_variant (
            (GSignondDictionary *)session_data);
    gsignond_dbus_remote_plugin_emit_response_final (
            self->priv->dbus_remote_plugin, data);
}

static void
_handle_store_from_plugin (
        GSignondPluginDaemon *self,
        GSignondSessionData *session_data,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *data = gsignond_dictionary_to_variant (
            (GSignondDictionary *)session_data);
    gsignond_dbus_remote_plugin_emit_store (self->priv->dbus_remote_plugin,
            data);
}

static void
_handle_error_from_plugin (
        GSignondPluginDaemon *self,
        GError *gerror,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *error = gsignond_error_to_variant (gerror);
    gsignond_dbus_remote_plugin_emit_error (self->priv->dbus_remote_plugin,
            error);
}

static void
_handle_user_action_required_from_plugin (
        GSignondPluginDaemon *self,
        GSignondSignonuiData *ui_data,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *data = gsignond_dictionary_to_variant (ui_data);
    gsignond_dbus_remote_plugin_emit_user_action_required (
            self->priv->dbus_remote_plugin, data);
}

static void
_handle_refreshed_from_plugin(
        GSignondPluginDaemon *self,
        GSignondSignonuiData *ui_data,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    GVariant *data = gsignond_dictionary_to_variant (ui_data);
    gsignond_dbus_remote_plugin_emit_refreshed (self->priv->dbus_remote_plugin,
            data);
}

static void
_handle_status_changed_from_plugin (
        GSignondPluginDaemon *self,
        GSignondPluginState status,
        gchar *message,
        gpointer user_data)
{
    DBG ("");
    g_return_if_fail (self && GSIGNOND_IS_PLUGIN_DAEMON (self));

    gsignond_dbus_remote_plugin_emit_status_changed (
            self->priv->dbus_remote_plugin, (gint)status,
            (const gchar *)message);
}

GSignondPluginDaemon *
gsignond_plugin_daemon_new (
        const gchar* filename,
        const gchar* plugin_type,
        gint in_fd,
        gint out_fd)
{
    GError *error = NULL;
    GSignondPipeStream *stream = NULL;

    g_return_val_if_fail (filename != NULL && plugin_type != NULL, NULL);

    GSignondPluginDaemon *daemon = GSIGNOND_PLUGIN_DAEMON (g_object_new (
            GSIGNOND_TYPE_PLUGIN_DAEMON, NULL));

    /* Load plugin */
    daemon->priv->plugin = gsignond_load_plugin_with_filename (
            (gchar *)plugin_type, (gchar*)filename);
    if (!daemon->priv->plugin) {
        DBG ("failed to load plugin");
        g_object_unref (daemon);
        return NULL;
    }

    daemon->priv->plugin_type = g_strdup (plugin_type);

    /* Create dbus connection */
    stream = gsignond_pipe_stream_new (in_fd, out_fd, TRUE);
    daemon->priv->connection = g_dbus_connection_new_sync (G_IO_STREAM (stream),
            NULL, G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING, NULL, NULL,
            NULL);
    g_object_unref (stream);

    /* Create dbus object */
    daemon->priv->dbus_remote_plugin =
            gsignond_dbus_remote_plugin_skeleton_new ();

    g_dbus_interface_skeleton_export (
                G_DBUS_INTERFACE_SKELETON(daemon->priv->dbus_remote_plugin),
                daemon->priv->connection, GSIGNOND_PLUGIN_OBJECTPATH, &error);
    if (error) {
        DBG ("failed to register object: %s", error->message);
        g_error_free (error);
        g_object_unref (daemon);
        return NULL;
    }
    DBG("Started plugin daemon '%p' at path '%s' on conneciton '%p'",
            daemon, GSIGNOND_PLUGIN_OBJECTPATH, daemon->priv->connection);

    /* Connect dbus remote plugin signals to handlers */
    g_signal_connect_swapped (daemon->priv->dbus_remote_plugin,
            "handle-cancel", G_CALLBACK (_handle_cancel_from_dbus), daemon);
    g_signal_connect_swapped (daemon->priv->dbus_remote_plugin,
            "handle-request", G_CALLBACK(_handle_request_from_dbus), daemon);
    g_signal_connect_swapped (daemon->priv->dbus_remote_plugin,
            "handle-request-initial",
            G_CALLBACK(_handle_request_initial_from_dbus), daemon);
    g_signal_connect_swapped (daemon->priv->dbus_remote_plugin,
            "handle-user-action-finished",
            G_CALLBACK(_handle_user_action_finished_from_dbus), daemon);
    g_signal_connect_swapped (daemon->priv->dbus_remote_plugin,
            "handle-refresh", G_CALLBACK(_handle_refresh_from_dbus), daemon);

    /* Connect plugin signals to handlers */
    g_signal_connect_swapped (daemon->priv->plugin, "response",
            G_CALLBACK (_handle_response_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "response-final",
            G_CALLBACK(_handle_response_final_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "store",
            G_CALLBACK(_handle_store_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "error",
            G_CALLBACK(_handle_error_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "user-action-required",
            G_CALLBACK(_handle_user_action_required_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "refreshed",
            G_CALLBACK(_handle_refreshed_from_plugin), daemon);
    g_signal_connect_swapped (daemon->priv->plugin, "status-changed",
            G_CALLBACK(_handle_status_changed_from_plugin), daemon);

    g_signal_connect (daemon->priv->connection, "closed",
            G_CALLBACK(_on_connection_closed), daemon);

    /* Set DBus properties */
    gchar* type;
    gchar** mechanisms;

    g_object_get(daemon->priv->plugin, "type", &type, "mechanisms", &mechanisms, NULL);
    gsignond_dbus_remote_plugin_set_method(daemon->priv->dbus_remote_plugin, type);
    gsignond_dbus_remote_plugin_set_mechanisms(daemon->priv->dbus_remote_plugin, (const gchar* const*) mechanisms);

    g_free(type);
    g_strfreev(mechanisms);

    g_dbus_connection_start_message_processing (daemon->priv->connection);

    return daemon;
}

