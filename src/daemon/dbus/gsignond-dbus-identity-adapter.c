/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
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
#include "gsignond-dbus-identity-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,

    PROP_IMPL,
    PROP_OBJECT_PATH,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondDbusIdentityAdapterPrivate
{
    GDBusConnection       *connection;
    GSignondIdentityIface *parent;
    gchar *object_path;
};

G_DEFINE_TYPE (GSignondDbusIdentityAdapter, gsignond_dbus_identity_adapter, GSIGNOND_DBUS_TYPE_IDENTITY_SKELETON)


#define GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_IDENTITY_ADAPTER, GSignondDbusIdentityAdapterPrivate)

static void _handle_request_credentials_update (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar*, gpointer);
static void _handle_get_info (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static void _handle_verify_user (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const GVariant *, gpointer);
static void _handle_verify_secret (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static void _handle_remove (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static void _handle_sign_out (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static void _handle_store (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const GVariant *, gpointer);
static void _handle_add_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static void _handle_remove_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);

static void
gsignond_dbus_identity_adapter_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            gpointer iface = g_value_peek_pointer (value);
            if (iface) {
                if (self->priv->parent) g_object_unref (self->priv->parent);
                self->priv->parent = GSIGNOND_IDENTITY_IFACE (g_object_ref (iface));
            }
            break;
        }
        case PROP_OBJECT_PATH: {
            const gchar *name = g_value_get_string (value);
            if (name) {
                if (self->priv->object_path) g_free (self->priv->object_path);

                self->priv->object_path = g_strdup (name);
            }
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_identity_adapter_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            g_value_set_instance (value, g_object_ref (self->priv->parent));
            break;
        }
        case PROP_OBJECT_PATH: {
            g_value_set_string (value, self->priv->object_path);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_identity_adapter_dispose (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

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
gsignond_dbus_identity_adapter_finalize (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (object));

    if (self->priv->object_path) {
        g_free (self->priv->object_path);
        self->priv->object_path = NULL;
    }

    G_OBJECT_CLASS (gsignond_dbus_identity_adapter_parent_class)->finalize (object);
}

static void
gsignond_dbus_identity_adapter_class_init (GSignondDbusIdentityAdapterClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondDbusIdentityAdapterPrivate));

    object_class->get_property = gsignond_dbus_identity_adapter_get_property;
    object_class->set_property = gsignond_dbus_identity_adapter_set_property;
    object_class->dispose = gsignond_dbus_identity_adapter_dispose;
    object_class->finalize = gsignond_dbus_identity_adapter_finalize;

    properties[PROP_IMPL] = g_param_spec_object ("auth-session-impl",
                                                  "Auth session impl",
                                                  "AuthSessionIface implementation object",
                                                  GSIGNOND_TYPE_IDENTITY_IFACE,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_OBJECT_PATH] = g_param_spec_string ("object-path",
                                                        "Object path",
                                                        "dbus object path of the identity",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_identity_adapter_init (GSignondDbusIdentityAdapter *self)
{
    static guint32 object_counter;
    GError *err = 0;

    self->priv = GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->parent = 0;
    self->priv->object_path = g_strdup_printf ("%s/Identity_%d", GSIGNOND_DAEMON_OBJECTPATH, object_counter++);


    self->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        ERR ("Error getting session bus :%s", err->message);
        g_error_free (err);
        return;
    }

    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                           self->priv->connection,
                                           self->priv->object_path,
                                           &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        return ;
    }

    g_signal_connect (self, "handle-request-credentials-update", G_CALLBACK (_handle_request_credentials_update), NULL);
    g_signal_connect (self, "handle-get-info", G_CALLBACK(_handle_get_info), NULL);
    g_signal_connect (self, "handle-verify-user", G_CALLBACK(_handle_verify_user), NULL);
    g_signal_connect (self, "handle-verify-secret", G_CALLBACK(_handle_verify_secret), NULL);
    g_signal_connect (self, "handle-remvoe", G_CALLBACK(_handle_remove), NULL);
    g_signal_connect (self, "handle-sign-out", G_CALLBACK(_handle_sign_out), NULL);
    g_signal_connect (self, "handle-store", G_CALLBACK(_handle_store), NULL);
    g_signal_connect (self, "handle-add-reference", G_CALLBACK(_handle_add_reference), NULL);
    g_signal_connect (self, "handle-remove-reference", G_CALLBACK(_handle_remove_reference), NULL);

}

static void
_handle_request_credentials_update (GSignondDbusIdentityAdapter *self,
                                    GDBusMethodInvocation *invocation,
                                    const gchar *message,
                                    gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    guint32 id = gsignond_identity_iface_request_credentials_update (self->priv->parent, message);
    if (id) {
        gsignond_dbus_identity_complete_request_credentials_update (iface, invocation, id);
    }
    else {
        /* 
         * TODO: Prepare error
         * GError *err = g_error_new ();
         * g_dbus_method_invocation_return_gerror (invocation, err);
         * g_error_free (err);
         */
    }
}

static void
_handle_get_info (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GVariant *identity_data = 0;
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);

    identity_data = gsignond_identity_iface_get_info (self->priv->parent);

    if (identity_data) {
        gsignond_dbus_identity_complete_get_info (iface, invocation, identity_data);
    }
    else {
        /*
         * TODO: Prepare error
         * GError *err = g_error_new ();
         * g_dbus_method_invocation_return_gerror (invocation, err);
         * g_error_free (err);
         */
    }
}

static void
_handle_verify_user (GSignondDbusIdentityAdapter *self,
                     GDBusMethodInvocation *invocation,
                     const GVariant *params,
                     gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gboolean res = gsignond_identity_iface_verify_user (self->priv->parent, params);

    gsignond_dbus_identity_complete_verify_user (iface, invocation, res);
}

static void
_handle_verify_secret (GSignondDbusIdentityAdapter *self,
                      GDBusMethodInvocation *invocation,
                      const gchar *secret,
                      gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gboolean res = gsignond_identity_iface_verify_secret (self->priv->parent, secret);

    gsignond_dbus_identity_complete_verify_secret (iface, invocation, res);
}

static void
_handle_remove (GSignondDbusIdentityAdapter   *self,
                GDBusMethodInvocation *invocation,
                gpointer               user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    
    gsignond_identity_iface_remove (self->priv->parent);

    gsignond_dbus_identity_complete_remove (iface, invocation);
}

static void
_handle_sign_out (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);

    gboolean res = gsignond_identity_iface_sign_out (self->priv->parent);

    gsignond_dbus_identity_complete_sign_out (iface, invocation, res);
}

static void
_handle_store (GSignondDbusIdentityAdapter *self,
               GDBusMethodInvocation *invocation,
               const GVariant *info,
               gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    guint id = gsignond_identity_iface_store (self->priv->parent, info);

    gsignond_dbus_identity_complete_store (iface, invocation, id);
}

static void
_handle_add_reference (GSignondDbusIdentityAdapter *self,
                       GDBusMethodInvocation *invocation,
                       const gchar *reference,
                       gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gint32 id ;

    id = gsignond_identity_iface_add_reference (self->priv->parent, reference);

    gsignond_dbus_identity_complete_add_reference (iface, invocation, id);
}

static void
_handle_remove_reference (GSignondDbusIdentityAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *reference,
                          gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gint32 id ;

    id = gsignond_identity_iface_remove_reference (self->priv->parent, reference);

    gsignond_dbus_identity_complete_remove_reference (iface, invocation, id);
}

GSignondDbusIdentityAdapter * gsignond_dbus_identity_adapter_new (GSignondIdentityIface *impl)
{
    return GSIGNOND_DBUS_IDENTITY_ADAPTER (g_object_new (GSIGNOND_TYPE_IDENTITY_ADAPTER, "identity-impl", impl, NULL));
}

