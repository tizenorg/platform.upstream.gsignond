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

#include "gsignond/gsignond-log.h"
#include "gsignond-dbus-auth-service-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,

    PROP_IMPL,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusAuthServiceAdapterPrivate
{
    GDBusConnection     *connection;
    GSignondAuthServiceIface *parent;
};

G_DEFINE_TYPE (GSignondDbusAuthServiceAdapter, gsignond_dbus_auth_service_adapter, GSIGNOND_DBUS_TYPE_AUTH_SERVICE_SKELETON)


#define GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, GSignondDbusAuthServiceAdapterPrivate)

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
                                      const GVariant*, gpointer);
static gboolean _handle_clear (GSignondDbusAuthServiceAdapter *, GDBusMethodInvocation *, gpointer);

static void
gsignond_dbus_auth_service_adapter_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            gpointer iface = g_value_peek_pointer (value);
            if (iface) {
                if (self->priv->parent) g_object_unref (self->priv->parent);
                self->priv->parent = GSIGNOND_AUTH_SERVICE_IFACE (g_object_ref (iface));
            }
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_service_adapter_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            g_value_set_instance (value, g_object_ref (self->priv->parent));
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_auth_service_adapter_dispose (GObject *object)
{
    GSignondDbusAuthServiceAdapter *self = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (object);

    if (self->priv->parent) {
        g_object_unref (self->priv->parent);
        self->priv->parent = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }
}

static void
gsignond_dbus_auth_service_adapter_finalize (GObject *object)
{
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (object));

    G_OBJECT_CLASS (gsignond_dbus_auth_service_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_auth_service_adapter_class_init (GSignondDbusAuthServiceAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusAuthServiceAdapterPrivate));

    object_class->get_property = gsignond_dbus_auth_service_adapter_get_property;
    object_class->set_property = gsignond_dbus_auth_service_adapter_set_property;
    object_class->dispose = gsignond_dbus_auth_service_adapter_dispose;
    object_class->finalize = gsignond_dbus_auth_service_adapter_finalize;

    properties[PROP_IMPL] = g_param_spec_object ("auth-session-impl",
                                                  "Auth session impl",
                                                  "AuthSessionIface implementation object",
                                                  GSIGNOND_TYPE_AUTH_SERVICE_IFACE,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_auth_service_adapter_init (GSignondDbusAuthServiceAdapter *self)
{
    GError *err = 0;

    self->priv = GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->parent = 0;

    self->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        ERR ("Error getting session bus :%s", err->message);
        g_error_free (err);
        return;
    }

    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                           self->priv->connection,
                                           GSIGNOND_DAEMON_OBJECTPATH,
                                           &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        return ;
    }

    g_signal_connect (self, "handle-register-new-identity", G_CALLBACK (_handle_register_new_identity), NULL);
    g_signal_connect (self, "handle-get-identity", G_CALLBACK(_handle_get_identity), NULL);
    g_signal_connect (self, "handle-query-methods", G_CALLBACK(_handle_query_methods), NULL);
    g_signal_connect (self, "handle-query-mechanisms", G_CALLBACK(_handle_query_mechanisms), NULL);
    g_signal_connect (self, "handle-query-identities", G_CALLBACK(_handle_query_identities), NULL);
    g_signal_connect (self, "handle-clear", G_CALLBACK(_handle_clear), NULL);

}

static void
_on_connnection_lost (GDBusConnection *conn,
                  const char *peer_name,
                  gpointer user_data)
{
    DBG ("peer disappeared : %s", peer_name);
    /*
     * FIXME; inform upper layer that peer closed so free objects
     * referenced by that peer.
     */
}

static gboolean
_handle_register_new_identity (GSignondDbusAuthServiceAdapter *self,
                               GDBusMethodInvocation *invocation,
                               const gchar *app_context,
                               gpointer user_data)
{
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);
    const gchar *object_path = gsignond_auth_service_iface_register_new_identity (self->priv->parent, app_context);

    GDBusConnection *connection = g_dbus_method_invocation_get_connection (invocation);
    const gchar *sender =  g_dbus_method_invocation_get_sender (invocation);

    g_bus_watch_name_on_connection (connection, 
                                    sender, 
                                    G_BUS_NAME_WATCHER_FLAGS_NONE, 
                                    NULL, 
                                    _on_connnection_lost, 
                                    iface,
                                    NULL);

    if (object_path) {
        gsignond_dbus_auth_service_complete_register_new_identity (iface, invocation, object_path);
    }
    else {
        /* 
         * TODO: Prepare error
         * GError *err = g_error_new ();
         * g_dbus_method_invocation_return_gerror (invocation, err);
         * g_error_free (err);
         */
    }

    return TRUE;
}

static gboolean
_handle_get_identity (GSignondDbusAuthServiceAdapter *self,
                      GDBusMethodInvocation *invocation,
                      guint32 id,
                      const gchar *app_context,
                      gpointer user_data)
{
    const gchar *object_path = 0;
    GVariant *identity_data = 0;
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);
    GDBusConnection *connection = g_dbus_method_invocation_get_connection (invocation);
    const gchar *sender =  g_dbus_method_invocation_get_sender (invocation);

    object_path = gsignond_auth_service_iface_get_identity (self->priv->parent, id, app_context, &identity_data);

    if (object_path) {
        g_bus_watch_name_on_connection (connection, 
                                    sender, 
                                    G_BUS_NAME_WATCHER_FLAGS_NONE, 
                                    NULL, 
                                    _on_connnection_lost, 
                                    iface,
                                    NULL);


        gsignond_dbus_auth_service_complete_get_identity (iface, invocation, object_path, identity_data);
        g_variant_unref (identity_data);
    }
    else {
        /*
         * TODO: Prepare error
         * GError *err = g_error_new ();
         * g_dbus_method_invocation_return_gerror (invocation, err);
         * g_error_free (err);
         */
    }

    return TRUE;
}

static gboolean
_handle_query_methods (GSignondDbusAuthServiceAdapter   *self,
                       GDBusMethodInvocation *invocation,
                       gpointer               user_data)
{
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);
    gchar **methods = gsignond_auth_service_iface_query_methods (self->priv->parent);

    gsignond_dbus_auth_service_complete_query_methods (iface, invocation, (const gchar * const*)methods);
    if (methods) g_strfreev(methods);

    return TRUE;
}

static gboolean
_handle_query_mechanisms (GSignondDbusAuthServiceAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *method,
                          gpointer user_data)
{
    gchar **mechanisms = 0;
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);

    mechanisms = gsignond_auth_service_iface_query_mechanisms (self->priv->parent, method);

    gsignond_dbus_auth_service_complete_query_mechanisms (iface, invocation, (const gchar* const*)mechanisms);

    if (mechanisms) g_strfreev(mechanisms);

    return TRUE;
}

static gboolean
_handle_query_identities (GSignondDbusAuthServiceAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const GVariant *filter,
                          gpointer user_data)
{
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);
    GVariant *identities = gsignond_auth_service_iface_query_identities (self->priv->parent, filter);

    gsignond_dbus_auth_service_complete_query_identities (iface, invocation, identities);

    return TRUE;
}

static gboolean
_handle_clear (GSignondDbusAuthServiceAdapter *self,
               GDBusMethodInvocation *invocation,
               gpointer user_data)
{
    gboolean res ;
    GSignondDbusAuthService *iface = GSIGNOND_DBUS_AUTH_SERVICE (self);

    res = gsignond_auth_service_iface_clear (self->priv->parent);

    gsignond_dbus_auth_service_complete_clear (iface, invocation, res);

    return TRUE;
}

GSignondDbusAuthServiceAdapter * gsignond_dbus_auth_service_adapter_new (GSignondAuthServiceIface *impl)
{
    return GSIGNOND_DBUS_AUTH_SERVICE_ADAPTER (g_object_new (GSIGNOND_TYPE_AUTH_SERVICE_ADAPTER, "auth-session-impl", impl, NULL));
}
