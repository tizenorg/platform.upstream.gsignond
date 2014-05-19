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

#include "config.h"
#include "gsignond/gsignond-log.h"
#include "gsignond-dbus-auth-service-adapter.h"
#include "gsignond-dbus-identity-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,

    PROP_CONNECTION,
    PROP_AUTH_SERVICE,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusAuthServiceAdapterPrivate
{
    GDBusConnection *connection;
    GSignondDbusAuthService *dbus_auth_service;
    GSignondDaemon  *auth_service;
    GList *identities;
#ifndef USE_P2P
    GHashTable *caller_watchers; //(dbus_caller:watcher_id)
#endif
};

G_DEFINE_TYPE (GSignondDbusAuthServiceAdapter, gsignond_dbus_auth_service_adapter, GSIGNOND_TYPE_DISPOSABLE)

#define GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_GET_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, GSignondDbusAuthServiceAdapterPrivate)

static gboolean _handle_register_new_identity (GSignondDbusAuthServiceAdapter *, GDBusMethodInvocation *, 
                                           const gchar *, gpointer);
static gboolean _handle_get_identity (GSignondDbusAuthServiceAdapter *, GDBusMethodInvocation *, guint32, 
                                  const gchar *, gpointer);
static gboolean _handle_query_methods (GSignondDbusAuthServiceAdapter *,
                                   GDBusMethodInvocation *,
                                   gpointer);
static gboolean _handle_query_mechanisms (GSignondDbusAuthServiceAdapter *,
                                      GDBusMethodInvocation *,
                                      const gchar *, gpointer);
static gboolean _handle_query_identities (GSignondDbusAuthServiceAdapter *,
                                      GDBusMethodInvocation *,
                                      GVariant*, 
                                      const gchar *,
                                      gpointer);
static gboolean _handle_clear (GSignondDbusAuthServiceAdapter *, GDBusMethodInvocation *, gpointer);
static void _on_identity_disposed (gpointer data, GObject *object);

static void
_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    switch (property_id) {
        case PROP_AUTH_SERVICE: {
            GObject *auth_service = g_value_get_object (value);
            if (auth_service) {
                if (self->priv->auth_service) g_object_unref (self->priv->auth_service);
                self->priv->auth_service = GSIGNOND_DAEMON (auth_service);
            }
            break;
        }
        case PROP_CONNECTION:
            self->priv->connection = g_value_get_object(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    switch (property_id) {
        case PROP_AUTH_SERVICE: {
            g_value_set_object (value, self->priv->auth_service);
            break;
        }
        case PROP_CONNECTION:
            g_value_set_object (value, self->priv->connection);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_identity_unref (gpointer data, gpointer user_data)
{
    if (data && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER(data)) {
        GObject *identity = G_OBJECT (data);
        g_object_weak_unref (identity, _on_identity_disposed, user_data);
        g_object_unref (identity);
    }
}

static void
_dispose (GObject *object)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    DBG("- unregistering dubs auth service. %d", G_OBJECT (self->priv->auth_service)->ref_count);

    if (self->priv->identities) {
        g_list_foreach (self->priv->identities, _identity_unref, self);
    }

    if (self->priv->auth_service) {
        g_object_unref (self->priv->auth_service);
        self->priv->auth_service = NULL;
    }

    if (self->priv->dbus_auth_service) {
        g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self->priv->dbus_auth_service));
        g_object_unref (self->priv->dbus_auth_service);
        self->priv->dbus_auth_service = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }

#ifndef USE_P2P
    if (self->priv->caller_watchers) {
        g_hash_table_unref (self->priv->caller_watchers);
        self->priv->caller_watchers = NULL;
    }
#endif

    G_OBJECT_CLASS (gsignond_dbus_auth_service_adapter_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    if (self->priv->identities) {
        g_list_free (self->priv->identities);
        self->priv->identities = NULL;
    }

    G_OBJECT_CLASS (gsignond_dbus_auth_service_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_auth_service_adapter_class_init (GSignondDbusAuthServiceAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusAuthServiceAdapterPrivate));

    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    properties[PROP_AUTH_SERVICE] = g_param_spec_object ("auth-service",
                                                  "Core auth service",
                                                  "AuthService core object",
                                                  GSIGNOND_TYPE_DAEMON,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                  "Bus connection",
                                                  "DBus connection used",
                                                  G_TYPE_DBUS_CONNECTION,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_auth_service_adapter_init (GSignondDbusAuthServiceAdapter *self)
{
    self->priv = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->auth_service = 0;
    self->priv->identities = NULL;
    self->priv->dbus_auth_service = gsignond_dbus_auth_service_skeleton_new ();
#ifndef USE_P2P
    self->priv->caller_watchers = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                g_free, (GDestroyNotify)g_bus_unwatch_name);
#endif

    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-register-new-identity", G_CALLBACK (_handle_register_new_identity), self);
    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-get-identity", G_CALLBACK(_handle_get_identity), self);
    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-query-methods", G_CALLBACK(_handle_query_methods), self);
    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-query-mechanisms", G_CALLBACK(_handle_query_mechanisms), self);
    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-query-identities", G_CALLBACK(_handle_query_identities), self);
    g_signal_connect_swapped (self->priv->dbus_auth_service,
        "handle-clear", G_CALLBACK(_handle_clear), self);
}

#ifndef USE_P2P
static void
_clear_cache_for_name (gpointer data, gpointer user_data)
{
    g_return_if_fail (user_data);
    g_return_if_fail (data && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (data));
    
    const gchar *caller = (const gchar *)user_data;
    GSignondDbusIdentityAdapter *dbus_identity = GSIGNOND_DBUS_IDENTITY_ADAPTER (data);
    const gchar *identity_owner = g_object_get_data (G_OBJECT (dbus_identity), "dbus-client-name");

    if (g_strcmp0 (identity_owner, caller) == 0) {
        DBG ("removing dbus identity '%p' from cache", dbus_identity);
        g_object_unref (dbus_identity);
    }
}

static void
_on_connnection_lost (GDBusConnection *conn, const char *peer_name, gpointer user_data)
{
    (void) conn;
    g_return_if_fail (peer_name);
    g_return_if_fail (user_data && GSIGNOND_IS_DBUS_AUTH_SERVICE_ADAPTER(user_data));

    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (user_data);
    DBG ("(-)peer disappeared : %s", peer_name);

    g_list_foreach (self->priv->identities, _clear_cache_for_name, (gpointer)peer_name);

    if (g_hash_table_contains (self->priv->caller_watchers, peer_name)) {
        g_hash_table_remove (self->priv->caller_watchers, (gpointer)peer_name);
    }
}
#endif

static void
_on_identity_disposed (gpointer data, GObject *object)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (data);

    DBG ("identity object %p disposed", object);
    self->priv->identities = g_list_remove (self->priv->identities, object);

    if (g_list_length (self->priv->identities) == 0) {
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);
    }
}

static GSignondDbusIdentityAdapter *
_create_and_cache_dbus_identity (GSignondDbusAuthServiceAdapter *self,
                                 GSignondIdentity *identity,
                                 const gchar *app_context,
                                 GDBusConnection *connection,
                                 const gchar *sender)
{
    GSignondDbusIdentityAdapter *dbus_identity = NULL; 
    guint identity_timeout = gsignond_daemon_get_identity_timeout (self->priv->auth_service);

    dbus_identity = gsignond_dbus_identity_adapter_new_with_connection (
                            g_object_ref (connection), identity, app_context, identity_timeout);

    /* keep alive till this identity object gets disposed */
    if (g_list_length (self->priv->identities) == 0)
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    self->priv->identities = g_list_append (self->priv->identities, dbus_identity);
    g_object_weak_ref (G_OBJECT (dbus_identity), _on_identity_disposed, self);
#ifndef USE_P2P
    g_object_set_data_full (G_OBJECT(dbus_identity), "dbus-client-name", (gpointer)g_strdup(sender), g_free);
    if (!g_hash_table_contains (self->priv->caller_watchers, sender)) {
        guint watcher_id = g_bus_watch_name_on_connection (connection, sender, G_BUS_NAME_WATCHER_FLAGS_NONE, 
                                        NULL, _on_connnection_lost, self, NULL);
        g_hash_table_insert (self->priv->caller_watchers, 
                             (gpointer)g_strdup (sender),
                             GUINT_TO_POINTER(watcher_id));
    }
#endif

    return dbus_identity;
}

static gboolean
_handle_register_new_identity (GSignondDbusAuthServiceAdapter *self,
                               GDBusMethodInvocation *invocation,
                               const gchar *app_context,
                               gpointer user_data)
{
    GSignondIdentity *identity = NULL;
    GError *error = NULL;
    GDBusConnection *connection = NULL;
    const gchar *sender = NULL;
    int fd = -1;
    GSignondSecurityContext *sec_context = gsignond_security_context_new ();

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_AUTH_SERVICE_ADAPTER(self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    connection = g_dbus_method_invocation_get_connection (invocation);
#ifdef USE_P2P
    fd = g_socket_get_fd (g_socket_connection_get_socket (G_SOCKET_CONNECTION (g_dbus_connection_get_stream(connection))));
#else
    sender = g_dbus_method_invocation_get_sender (invocation);
#endif

    gsignond_access_control_manager_security_context_of_peer(
            gsignond_daemon_get_access_control_manager (self->priv->auth_service),
            sec_context,
            fd,
            sender,
            app_context);

    identity = gsignond_daemon_register_new_identity (self->priv->auth_service, sec_context, &error);

    if (identity) {
        GSignondDbusIdentityAdapter *dbus_identity = _create_and_cache_dbus_identity (self, identity, app_context, connection, sender);

        gsignond_dbus_auth_service_complete_register_new_identity (self->priv->dbus_auth_service,
            invocation, gsignond_dbus_identity_adapter_get_object_path (dbus_identity));
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    gsignond_security_context_free (sec_context);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);
    
    return TRUE;
}

static gboolean
_handle_get_identity (GSignondDbusAuthServiceAdapter *self,
                      GDBusMethodInvocation *invocation,
                      guint32 id,
                      const gchar *app_context,
                      gpointer user_data)
{
    GSignondIdentity *identity = NULL;
    GError *error = NULL;
    GDBusConnection *connection = NULL;
    const gchar *sender =  NULL;
    int fd = -1;
    GSignondSecurityContext *sec_context = gsignond_security_context_new ();

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    connection = g_dbus_method_invocation_get_connection (invocation);
#ifdef USE_P2P
    fd = g_socket_get_fd (g_socket_connection_get_socket (G_SOCKET_CONNECTION (g_dbus_connection_get_stream(connection))));
#else
    sender = g_dbus_method_invocation_get_sender (invocation);
#endif
    gsignond_access_control_manager_security_context_of_peer(
            gsignond_daemon_get_access_control_manager (self->priv->auth_service),
            sec_context,
            fd,
            sender,
            app_context);

    identity = gsignond_daemon_get_identity (self->priv->auth_service, id, sec_context, &error);

    if (identity) {
        GSignondIdentityInfo *info = NULL;
        GSignondDbusIdentityAdapter *dbus_identity = _create_and_cache_dbus_identity (self, identity, app_context, connection, sender);

        info = gsignond_identity_get_identity_info (identity);
        gsignond_dbus_auth_service_complete_get_identity (self->priv->dbus_auth_service,
            invocation, gsignond_dbus_identity_adapter_get_object_path (dbus_identity),
            gsignond_identity_info_to_variant(info));
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    gsignond_security_context_free (sec_context);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_query_methods (GSignondDbusAuthServiceAdapter   *self,
                       GDBusMethodInvocation *invocation,
                       gpointer               user_data)
{
    const gchar **methods = NULL;
    GError *error = NULL;
 
    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    methods = gsignond_daemon_query_methods (self->priv->auth_service, &error);

    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    } else {
        const gchar *const empty_methods[] = { NULL };
        gsignond_dbus_auth_service_complete_query_methods (
              self->priv->dbus_auth_service, invocation,
              methods ? (const gchar * const*)methods : empty_methods);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_query_mechanisms (GSignondDbusAuthServiceAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *method,
                          gpointer user_data)
{
    const gchar **mechanisms = 0;
    GError *error = NULL;

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    mechanisms = gsignond_daemon_query_mechanisms (self->priv->auth_service, method, &error);

    if (mechanisms)
        gsignond_dbus_auth_service_complete_query_mechanisms (
            self->priv->dbus_auth_service, invocation, (const gchar* const*)mechanisms);
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static void
_append_identity_info (gpointer data, gpointer user_data)
{
    GVariantBuilder *builder = (GVariantBuilder *)user_data;

    g_variant_builder_add (builder, "@a{sv}", gsignond_identity_info_to_variant ((GSignondIdentityInfo*)data));
}

static gboolean
_handle_query_identities (GSignondDbusAuthServiceAdapter *self,
                          GDBusMethodInvocation *invocation,
                          GVariant *filter,
                          const gchar *app_context,
                          gpointer user_data)
{
    GSignondIdentityInfoList *identities = NULL;
    GError *error = NULL;
    GSignondSecurityContext *sec_context;
    const gchar *sender =  NULL;
    int fd = -1;

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

#ifdef USE_P2P
    GDBusConnection *connection = NULL;
    connection = g_dbus_method_invocation_get_connection (invocation);
    fd = g_socket_get_fd (g_socket_connection_get_socket (G_SOCKET_CONNECTION (g_dbus_connection_get_stream(connection))));
#else
    sender = g_dbus_method_invocation_get_sender (invocation);
#endif
    sec_context = gsignond_security_context_new ();
    gsignond_access_control_manager_security_context_of_peer(
            gsignond_daemon_get_access_control_manager (self->priv->auth_service),
            sec_context,
            fd,
            sender, app_context);

    identities = gsignond_daemon_query_identities (self->priv->auth_service,
                                                   filter,
                                                   sec_context,
                                                   &error);

    gsignond_security_context_free (sec_context);

    if (identities) {
        GVariantBuilder builder;
        
        g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

        g_list_foreach(identities, _append_identity_info, &builder);

        gsignond_identity_info_list_free (identities);

        gsignond_dbus_auth_service_complete_query_identities (
            self->priv->dbus_auth_service, invocation,
            g_variant_builder_end(&builder));

        g_variant_builder_clear (&builder);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_clear (GSignondDbusAuthServiceAdapter *self,
               GDBusMethodInvocation *invocation,
               gpointer user_data)
{
    gboolean res ;
    GError *error = NULL;
    GSignondSecurityContext *sec_context;
    const gchar *sender =  NULL;
    int fd = -1;

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);
#ifdef USE_P2P
    GDBusConnection *connection = NULL;
    connection = g_dbus_method_invocation_get_connection (invocation);
    fd = g_socket_get_fd (g_socket_connection_get_socket (G_SOCKET_CONNECTION (g_dbus_connection_get_stream(connection))));
#else
    sender = g_dbus_method_invocation_get_sender (invocation);
#endif
    sec_context = gsignond_security_context_new ();
    gsignond_access_control_manager_security_context_of_peer(
            gsignond_daemon_get_access_control_manager (self->priv->auth_service),
            sec_context,
            fd,
            sender, "");

    res = gsignond_daemon_clear (self->priv->auth_service,
                                 sec_context,
                                 &error);

    gsignond_security_context_free (sec_context);

    if (!error)
        gsignond_dbus_auth_service_complete_clear (self->priv->dbus_auth_service, invocation, res);
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

GSignondDbusAuthServiceAdapter *
gsignond_dbus_auth_service_adapter_new_with_connection (GDBusConnection *bus_connection, GSignondDaemon *daemon)
{
    GError *err = NULL;
    guint timeout = 0;
    GSignondDbusAuthServiceAdapter *adapter = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (
        g_object_new (GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, 
            "auth-service", daemon, 
            "connection", bus_connection,
            NULL));

    if (!g_dbus_interface_skeleton_export (
            G_DBUS_INTERFACE_SKELETON(adapter->priv->dbus_auth_service),
            adapter->priv->connection, GSIGNOND_DAEMON_OBJECTPATH, &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        g_object_unref (adapter);
        return NULL;
    }
    DBG("(+) started auth service '%p' at path '%s' on connection '%p'", adapter, GSIGNOND_DAEMON_OBJECTPATH, bus_connection);

    timeout = gsignond_daemon_get_timeout (adapter->priv->auth_service);
    if (timeout) {
        gsignond_disposable_set_timeout (GSIGNOND_DISPOSABLE (adapter), timeout);
    }
    return adapter;
}
#ifndef USE_P2P
GSignondDbusAuthServiceAdapter * gsignond_dbus_auth_service_adapter_new (GSignondDaemon *daemon)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync (GSIGNOND_BUS_TYPE, NULL, &error);

    if (error) {
        ERR("failed to connect to session bus : %s", error->message);
        g_error_free (error);
        return NULL;
    }

    return gsignond_dbus_auth_service_adapter_new_with_connection (connection, daemon);
}
#endif
