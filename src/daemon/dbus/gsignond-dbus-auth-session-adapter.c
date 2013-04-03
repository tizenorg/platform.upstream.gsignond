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

#include <config.h>
#include "gsignond-dbus-auth-session-adapter.h"
#include "gsignond/gsignond-log.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,
    PROP_SESSION,
    PROP_CONNECTION,
    PROP_APP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusAuthSessionAdapterPrivate
{
    GDBusConnection     *connection;
    GSignondDbusAuthSession *dbus_auth_session;
    GSignondAuthSession *session;
    gchar *app_context;
    gboolean is_process_active;
    GSignondSecurityContext ctx;
};

G_DEFINE_TYPE (GSignondDbusAuthSessionAdapter, gsignond_dbus_auth_session_adapter, GSIGNOND_TYPE_DISPOSABLE)


#define GSIGNOND_DBUS_AUTH_SESSION_ADAPTER_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_AUTH_SESSION_ADAPTER, GSignondDbusAuthSessionAdapterPrivate)

#define PREPARE_SECURITY_CONTEXT(dbus_object, invocation) \
{ \
    GSignondDbusAuthSessionAdapterPrivate *priv = dbus_object->priv; \
    GSignondAccessControlManager *acm = gsignond_auth_session_get_acm (priv->session); \
    const gchar *sender = NULL; \
    int fd = -1; \
    sender = g_dbus_method_invocation_get_sender (invocation); \
    if (!sender) {\
        GDBusConnection *connection = g_dbus_method_invocation_get_connection (invocation);\
        fd = g_socket_get_fd(g_socket_connection_get_socket(G_SOCKET_CONNECTION(g_dbus_connection_get_stream (connection))));\
    }\
    gsignond_access_control_manager_security_context_of_peer( \
            acm, \
            &priv->ctx, \
            fd, \
            sender, \
            priv->app_context); \
}

static gboolean _handle_query_available_mechanisms (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, const gchar **, gpointer);
static gboolean _handle_process (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, const GVariant *, const gchar *, gpointer);
static gboolean _handle_cancel (GSignondDbusAuthSessionAdapter *, GDBusMethodInvocation *, gpointer);

static void
gsignond_dbus_auth_session_adapter_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    switch (property_id) {
        case PROP_SESSION: {
            gpointer object = g_value_get_object (value);
            self->priv->session = GSIGNOND_AUTH_SESSION ((object));
            break;
        }
        case PROP_CONNECTION: {
            if (self->priv->connection) g_object_unref (self->priv->connection);
            self->priv->connection = G_DBUS_CONNECTION (g_value_get_object (value));
            break;
        }
        case PROP_APP_CONTEXT: {
            if (self->priv->app_context) g_free (self->priv->app_context);
            self->priv->app_context = g_strdup (g_value_get_string (value));
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_session_adapter_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    switch (property_id) {
        case PROP_SESSION: {
            g_value_set_object (value, self->priv->session);
            break;
        }
        case PROP_CONNECTION:
            g_value_set_object (value, self->priv->connection);
            break;
        case PROP_APP_CONTEXT:
            g_value_set_string (value, self->priv->app_context);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_session_adapter_dispose (GObject *object)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    if (self->priv->session) {
        if (self->priv->is_process_active) {
            gsignond_auth_session_abort_process (self->priv->session);
            self->priv->is_process_active = FALSE;
        }

        g_object_unref (self->priv->session);
        self->priv->session = NULL;
    }

    if (self->priv->dbus_auth_session) {
        gsignond_dbus_auth_session_emit_unregistered (self->priv->dbus_auth_session);
        DBG("(-)'%s' object unexported", g_dbus_interface_skeleton_get_object_path (
            G_DBUS_INTERFACE_SKELETON(self->priv->dbus_auth_session)));
        g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON(self->priv->dbus_auth_session));
        g_object_unref (self->priv->dbus_auth_session);
        self->priv->dbus_auth_session = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }

    G_OBJECT_CLASS(gsignond_dbus_auth_session_adapter_parent_class)->dispose (object);
}

static void
gsignond_dbus_auth_session_adapter_finalize (GObject *object)
{
    GSignondDbusAuthSessionAdapter *self = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (object);

    if (self->priv->app_context) {
        g_free (self->priv->app_context);
        self->priv->app_context = NULL;
    }

    G_OBJECT_CLASS (gsignond_dbus_auth_session_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_auth_session_adapter_class_init (GSignondDbusAuthSessionAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusAuthSessionAdapterPrivate));

    object_class->get_property = gsignond_dbus_auth_session_adapter_get_property;
    object_class->set_property = gsignond_dbus_auth_session_adapter_set_property;
    object_class->dispose = gsignond_dbus_auth_session_adapter_dispose;
    object_class->finalize = gsignond_dbus_auth_session_adapter_finalize;

    properties[PROP_SESSION] = g_param_spec_object ("auth-session",
                                                    "Core auth session object",
                                                    "Core AuthSession Object",
                                                    GSIGNOND_TYPE_AUTH_SESSION,
                                                    G_PARAM_READWRITE |
                                                    G_PARAM_CONSTRUCT_ONLY |
                                                    G_PARAM_STATIC_STRINGS);

    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                       "Dbus connection used",
                                                       "Dbus connection used",
                                                       G_TYPE_DBUS_CONNECTION,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY | 
                                                       G_PARAM_STATIC_STRINGS);
    properties[PROP_APP_CONTEXT] = g_param_spec_string (
                "app-context",
                "application security context",
                "Application security context of the identity object creater",
                NULL,
                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_auth_session_adapter_init (GSignondDbusAuthSessionAdapter *self)
{
    self->priv = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->session = 0;
    self->priv->app_context = 0;
    self->priv->is_process_active = FALSE;
    self->priv->dbus_auth_session = gsignond_dbus_auth_session_skeleton_new ();

    g_signal_connect_swapped (self->priv->dbus_auth_session,
        "handle-query-available-mechanisms", 
        G_CALLBACK (_handle_query_available_mechanisms), self);
    g_signal_connect_swapped (self->priv->dbus_auth_session, "handle-process", G_CALLBACK(_handle_process), self);
    g_signal_connect_swapped (self->priv->dbus_auth_session, "handle-cancel", G_CALLBACK(_handle_cancel), self);
}

static gboolean
_handle_query_available_mechanisms (GSignondDbusAuthSessionAdapter *self,
                                    GDBusMethodInvocation *invocation,
                                    const gchar **wanted_mechanisms,
                                    gpointer user_data)
{
    gchar **mechanisms = NULL;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_AUTH_SESSION_ADAPTER (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    mechanisms = gsignond_auth_session_query_available_mechanisms (
        self->priv->session, wanted_mechanisms, &self->priv->ctx, &error);

    if (mechanisms) {
        gsignond_dbus_auth_session_complete_query_available_mechanisms (
            self->priv->dbus_auth_session, invocation, (const gchar * const *)mechanisms);
        g_free (mechanisms);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (self));
    return TRUE;
}

typedef struct {
    GSignondDbusAuthSessionAdapter *adapter;
    GDBusMethodInvocation *invocation;
} _AuthSessionDbusInfo;

static _AuthSessionDbusInfo*
_auth_session_dbus_info_new (GSignondDbusAuthSessionAdapter *self, GDBusMethodInvocation *invocation)
{
    _AuthSessionDbusInfo *info = g_slice_new0(_AuthSessionDbusInfo);

    info->adapter = g_object_ref (self);
    info->invocation = g_object_ref (invocation);

    return info;
}

static void
_auth_session_dbus_info_free (_AuthSessionDbusInfo *info)
{
    if (!info) return;

    g_object_unref (info->adapter);
    g_object_unref (info->invocation);

    g_slice_free (_AuthSessionDbusInfo, info);
}

static void
_emit_state_changed (gint state, const gchar *message, gpointer user_data)
{
    GSignondDbusAuthSessionAdapter *self = NULL;
    _AuthSessionDbusInfo *info = (_AuthSessionDbusInfo*) user_data;

    if (!info) return ;

    self = info->adapter;
    gsignond_dbus_auth_session_emit_state_changed (
            self->priv->dbus_auth_session, state, message);
}
static void
_on_process_done (GSignondSessionData *reply, const GError *error, gpointer user_data)
{
    GSignondDbusAuthSessionAdapter *self = NULL;
    _AuthSessionDbusInfo *info = (_AuthSessionDbusInfo*) user_data;

    if (!info) return ;

    self = info->adapter;
    self->priv->is_process_active = FALSE;

    if (error)
        g_dbus_method_invocation_return_gerror (info->invocation, error);
    else {
        GVariant *result = gsignond_dictionary_to_variant ((GSignondDictionary *)reply); 
        gsignond_dbus_auth_session_complete_process (
                self->priv->dbus_auth_session, info->invocation, result);
    }
    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    _auth_session_dbus_info_free (info);
}

static gboolean
_handle_process (GSignondDbusAuthSessionAdapter *self,
                 GDBusMethodInvocation *invocation,
                 const GVariant *session_data,
                 const gchar *mechanisms,
                 gpointer user_data)
{
    _AuthSessionDbusInfo *info = 0;
    GError *error = NULL;
    GSignondSessionData *data = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_AUTH_SESSION_ADAPTER (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    data = (GSignondSessionData *)gsignond_dictionary_new_from_variant ((GVariant *)session_data);
    info = _auth_session_dbus_info_new (self, invocation);
    self->priv->is_process_active = TRUE;
    if (!gsignond_auth_session_process (self->priv->session, data, mechanisms, 
                &self->priv->ctx, _on_process_done, 
                _emit_state_changed, info, &error)) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
 
        _auth_session_dbus_info_free (info);
        
        self->priv->is_process_active = FALSE;

        gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE(self));
    }
    else {
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);
    }

    gsignond_dictionary_unref (data);

    return TRUE;
}

static gboolean
_handle_cancel (GSignondDbusAuthSessionAdapter *self,
                GDBusMethodInvocation *invocation,
                gpointer user_data)
{
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_AUTH_SESSION_ADAPTER (self), FALSE);
    
    PREPARE_SECURITY_CONTEXT (self, invocation);

    if (gsignond_auth_session_cancel (self->priv->session, &self->priv->ctx, &error))
        gsignond_dbus_auth_session_complete_cancel (self->priv->dbus_auth_session, invocation);
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE(self));

    return TRUE;
}

const gchar *
gsignond_dbus_auth_session_adapter_get_object_path (GSignondDbusAuthSessionAdapter *self)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_AUTH_SESSION_ADAPTER(self), NULL);

    return g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON(self->priv->dbus_auth_session));
}

GSignondDbusAuthSessionAdapter *
gsignond_dbus_auth_session_adapter_new_with_connection (GDBusConnection *connection, 
                                                        GSignondAuthSession *session,
                                                        const gchar *app_context,
                                                        guint timeout)
{
    static guint32 object_counter;
    gchar *object_path = NULL;
    GSignondDbusAuthSessionAdapter *adapter = NULL;
    GError *error = NULL;
    
    adapter = GSIGNOND_DBUS_AUTH_SESSION_ADAPTER (
        g_object_new (GSIGNOND_TYPE_DBUS_AUTH_SESSION_ADAPTER, 
            "connection", connection, "auth-session", session, "app-context", app_context, NULL));

    if (!adapter) return NULL;

    object_path = g_strdup_printf ("%s/AuthSession_%d", GSIGNOND_DAEMON_OBJECTPATH, object_counter++);
    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (adapter->priv->dbus_auth_session),
                                           adapter->priv->connection,
                                           object_path,
                                           &error)) {
        ERR ("failed to register object: %s", error->message);
        g_error_free (error);
        g_free (object_path);
        g_object_unref (adapter);
        return NULL;
    }
    DBG("(+) '%s' object exported", object_path);
    g_free (object_path);

    gsignond_disposable_set_timeout (GSIGNOND_DISPOSABLE(adapter), timeout);

    return adapter;
}

#ifndef USE_P2P
GSignondDbusAuthSessionAdapter *
gsignond_dbus_auth_session_adapter_new (GSignondAuthSession *session, const gchar *app_context, guint timeout)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        ERR ("Error getting session bus :%s", error->message);
        g_error_free (error);
        return NULL;
    }

    return gsignond_dbus_auth_session_adapter_new_with_connection (connection, session, app_context, timeout);
}
#endif
