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
#include "gsignond-dbus-identity-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,

    PROP_IMPL,
    PROP_APP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

typedef struct {
    GSignondDbusIdentityAdapter *adapter;
    GDBusMethodInvocation *invocation;
    gpointer user_data;
} _IdentityDbusInfo;

struct _GSignondDbusIdentityAdapterPrivate
{
    GDBusConnection       *connection;
    GSignondIdentityIface *identity;
    gchar *app_context;
    GSignondSecurityContext sec_context;
    /* signal handler ids */
    guint info_updated_handler_id;
    guint verify_user_handler_id;
    guint verify_secret_handler_id;
};

G_DEFINE_TYPE (GSignondDbusIdentityAdapter, gsignond_dbus_identity_adapter, GSIGNOND_DBUS_TYPE_IDENTITY_SKELETON)

#define GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_IDENTITY_ADAPTER, GSignondDbusIdentityAdapterPrivate)

#define PREPARE_SECURITY_CONTEXT(dbus_object, invocation) \
{ \
    GSignondDbusIdentityAdapterPrivate *priv = dbus_object->priv; \
    const gchar *sender = g_dbus_method_invocation_get_sender (invocation); \
    GSignondAccessControlManager *acm = gsignond_identity_iface_get_acm (priv->identity); \
    gsignond_access_control_manager_security_context_of_peer( \
            acm, \
            &priv->sec_context, \
            -1, \
            sender, \
            priv->app_context); \
}

static gboolean _handle_request_credentials_update (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar*, gpointer);
static gboolean _handle_get_info (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_get_auth_session (GSignondDbusIdentityAdapter *self, GDBusMethodInvocation *invocation, const gchar *method, gpointer user_data);
static gboolean _handle_verify_user (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const GVariant *, gpointer);
static gboolean _handle_verify_secret (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static gboolean _handle_remove (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_sign_out (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_store (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const GVariant *, gpointer);
static gboolean _handle_add_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static gboolean _handle_remove_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static void _emit_info_updated (GSignondIdentityIface *identity, GSignondIdentityChangeType change, gpointer userdata);

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
                if (self->priv->identity) {
                    if (self->priv->info_updated_handler_id) {
                        g_signal_handler_disconnect (self->priv->identity, self->priv->info_updated_handler_id);
                        self->priv->info_updated_handler_id = 0;
                    }
                    if (self->priv->verify_user_handler_id) {
                        g_signal_handler_disconnect (self->priv->identity, self->priv->verify_user_handler_id);
                        self->priv->verify_user_handler_id = 0;
                    }
                    if (self->priv->verify_secret_handler_id) {
                        g_signal_handler_disconnect (self->priv->identity, self->priv->verify_secret_handler_id);
                        self->priv->verify_secret_handler_id = 0;
                    }
                }
                self->priv->identity = GSIGNOND_IDENTITY_IFACE (iface);
                self->priv->info_updated_handler_id = g_signal_connect (
                        self->priv->identity, "info-updated", G_CALLBACK (_emit_info_updated), self);
            }
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
gsignond_dbus_identity_adapter_get_property (GObject *object,
        guint property_id,
        GValue *value, 
        GParamSpec *pspec)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    switch (property_id) {
        case PROP_IMPL: {
            g_value_set_instance (value, self->priv->identity);
            break;
        }
        case PROP_APP_CONTEXT:
            g_value_set_string (value, self->priv->app_context);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_dbus_identity_adapter_dispose (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    gsignond_dbus_identity_emit_unregistered (GSIGNOND_DBUS_IDENTITY (object));

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }
    
    if (self->priv->identity) {
        if (self->priv->info_updated_handler_id) {
            g_signal_handler_disconnect (self->priv->identity, self->priv->info_updated_handler_id);
            self->priv->info_updated_handler_id = 0;
        }
        if (self->priv->verify_user_handler_id) {
            g_signal_handler_disconnect (self->priv->identity, self->priv->verify_user_handler_id);
            self->priv->verify_user_handler_id = 0;
        }
        if (self->priv->verify_secret_handler_id) {
            g_signal_handler_disconnect (self->priv->identity, self->priv->verify_secret_handler_id);
            self->priv->verify_secret_handler_id = 0;
        }
    }

    G_OBJECT_CLASS (gsignond_dbus_identity_adapter_parent_class)->dispose (object);
}

static void
gsignond_dbus_identity_adapter_finalize (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    if (self->priv->app_context) {
        g_free (self->priv->app_context);
        self->priv->app_context = NULL;
    }

    if (self->priv->identity) {
        self->priv->identity = NULL;
    }

    DBG("(-)'%s' object unexported", g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON(object)));
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (object));

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

    properties[PROP_IMPL] = g_param_spec_object ("identity-impl",
                                                  "Identity Iface implementation",
                                                  "IdentityIface implementation object",
                                                  GSIGNOND_TYPE_IDENTITY_IFACE,
                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    properties[PROP_APP_CONTEXT] = g_param_spec_string (
                "app-context",
                "application security context",
                "Application security context of the identity object creater",
                NULL,
                G_PARAM_READWRITE);
    
    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_identity_adapter_init (GSignondDbusIdentityAdapter *self)
{
    static guint32 object_counter;
    GError *err = 0;
    gchar *object_path = 0;

    self->priv = GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->identity = 0;
    self->priv->app_context = 0;

    self->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        ERR ("Error getting session bus :%s", err->message);
        g_error_free (err);
        return;
    }

    object_path = g_strdup_printf ("%s/Identity_%d", GSIGNOND_DAEMON_OBJECTPATH, object_counter++);
    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                           self->priv->connection,
                                           object_path,
                                           &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        g_free (object_path);
        return ;
    }
    DBG("(+)'%s' object exported", object_path);
    g_free (object_path);

    g_signal_connect (self, "handle-request-credentials-update", G_CALLBACK (_handle_request_credentials_update), NULL);
    g_signal_connect (self, "handle-get-info", G_CALLBACK(_handle_get_info), NULL);
    g_signal_connect (self, "handle-get-auth-session", G_CALLBACK(_handle_get_auth_session), NULL);
    g_signal_connect (self, "handle-verify-user", G_CALLBACK(_handle_verify_user), NULL);
    g_signal_connect (self, "handle-verify-secret", G_CALLBACK(_handle_verify_secret), NULL);
    g_signal_connect (self, "handle-remove", G_CALLBACK(_handle_remove), NULL);
    g_signal_connect (self, "handle-sign-out", G_CALLBACK(_handle_sign_out), NULL);
    g_signal_connect (self, "handle-store", G_CALLBACK(_handle_store), NULL);
    g_signal_connect (self, "handle-add-reference", G_CALLBACK(_handle_add_reference), NULL);
    g_signal_connect (self, "handle-remove-reference", G_CALLBACK(_handle_remove_reference), NULL);

}

static gboolean
_handle_request_credentials_update (GSignondDbusIdentityAdapter *self,
                                    GDBusMethodInvocation *invocation,
                                    const gchar *message,
                                    gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    guint32 id;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);
    
    id = gsignond_identity_iface_request_credentials_update (self->priv->identity, message, &self->priv->sec_context, &error);
    if (id) {
        gsignond_dbus_identity_complete_request_credentials_update (iface, invocation, id);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static gboolean
_handle_get_info (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    GVariant *identity_data = 0;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);
    identity_data = gsignond_identity_iface_get_info (self->priv->identity, &self->priv->sec_context, &error);

    if (identity_data) {
        gsignond_dbus_identity_complete_get_info (iface, invocation, identity_data);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static gboolean
_handle_get_auth_session (GSignondDbusIdentityAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *method,
                          gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    const gchar *object_path = NULL;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);

    object_path = gsignond_identity_iface_get_auth_session (self->priv->identity, method, &self->priv->sec_context, &error);

    if (object_path) {
        gsignond_dbus_identity_complete_get_auth_session (iface, invocation, object_path);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static void
_on_user_verfied (GSignondIdentityIface *identity, gboolean res, const GError *error, gpointer user_data)
{
    _IdentityDbusInfo *info = (_IdentityDbusInfo *)user_data;
   
    if (G_UNLIKELY (info)) {
        WARN ("assertion G_UNLIKELY (info) fialed");
        return ;
    }

    g_signal_handler_disconnect (identity, info->adapter->priv->verify_user_handler_id);
    info->adapter->priv->verify_user_handler_id = 0;

    if (error) {
        g_dbus_method_invocation_return_gerror (info->invocation, error);
    }
    else {
        gsignond_dbus_identity_complete_verify_user (
            GSIGNOND_DBUS_IDENTITY (info->adapter), info->invocation, res);
    }

    g_free (info);
}

static gboolean
_handle_verify_user (GSignondDbusIdentityAdapter *self,
                     GDBusMethodInvocation *invocation,
                     const GVariant *params,
                     gpointer user_data)
{
    gboolean res = FALSE;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);

    res = gsignond_identity_iface_verify_user (self->priv->identity, params, &self->priv->sec_context, &error);

    if (!res) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else {
        _IdentityDbusInfo *info = g_new0(_IdentityDbusInfo, 1);

        info->adapter = self;
        info->invocation = invocation;
        info->user_data = NULL;

        /* FIXME: Do we allow multiple calls at a given point of time */
        self->priv->verify_user_handler_id = g_signal_connect (self->priv->identity, 
                    "user-verified", G_CALLBACK (_on_user_verfied), (gpointer)info);
    }

    return TRUE;
}

static void
_on_secret_verfied (GSignondIdentityIface *identity, gboolean res, const GError *error, gpointer user_data)
{
    _IdentityDbusInfo *info = (_IdentityDbusInfo *)user_data;

    g_signal_handler_disconnect (identity, info->adapter->priv->verify_secret_handler_id);
    info->adapter->priv->verify_secret_handler_id = 0;

    if (G_UNLIKELY (info)) {
        WARN ("assertion G_UNLIKELY (info) fialed");
        return ;
    }
    if (error) {
        g_dbus_method_invocation_return_gerror (info->invocation, error);
    }
    else {
        gsignond_dbus_identity_complete_verify_secret (
            GSIGNOND_DBUS_IDENTITY (info->adapter), info->invocation, res);
    }

    g_free (info);
}

static gboolean
_handle_verify_secret (GSignondDbusIdentityAdapter *self,
                      GDBusMethodInvocation *invocation,
                      const gchar *secret,
                      gpointer user_data)
{
    gboolean res = FALSE;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);
    
    res = gsignond_identity_iface_verify_secret (self->priv->identity, secret, &self->priv->sec_context, &error);

    if (!res) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else {
        _IdentityDbusInfo *info = g_new0(_IdentityDbusInfo, 1);

        info->adapter = self;
        info->invocation = invocation;
        info->user_data = NULL;

        /* FIXME: Do we allow multiple calls at a given point of time */
        self->priv->verify_secret_handler_id = g_signal_connect (self->priv->identity, 
                "secret-verified", G_CALLBACK (_on_secret_verfied), (gpointer)info);
    }

    return TRUE;
}

static gboolean 
_handle_remove (GSignondDbusIdentityAdapter   *self,
                GDBusMethodInvocation *invocation,
                gpointer               user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    GError *error = NULL;
    
    PREPARE_SECURITY_CONTEXT (self, invocation);

    if (!gsignond_identity_iface_remove (self->priv->identity, &self->priv->sec_context, &error)) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else {
        gsignond_dbus_identity_complete_remove (iface, invocation);
    }

    return TRUE;
}

static gboolean
_handle_sign_out (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gboolean res = FALSE;
    GError *error = NULL;
    
    PREPARE_SECURITY_CONTEXT (self, invocation);

    res = gsignond_identity_iface_sign_out (self->priv->identity, &self->priv->sec_context, &error);

    if (res) {
        gsignond_dbus_identity_complete_sign_out (iface, invocation, res);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static gboolean
_handle_store (GSignondDbusIdentityAdapter *self,
               GDBusMethodInvocation *invocation,
               const GVariant *info,
               gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    guint id = 0;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_iface_store (self->priv->identity, info, &self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_store (iface, invocation, id);
    } else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static gboolean
_handle_add_reference (GSignondDbusIdentityAdapter *self,
                       GDBusMethodInvocation *invocation,
                       const gchar *reference,
                       gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gint32 id = 0;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_iface_add_reference (self->priv->identity, reference, &self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_add_reference (iface, invocation, id);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static gboolean
_handle_remove_reference (GSignondDbusIdentityAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *reference,
                          gpointer user_data)
{
    GSignondDbusIdentity *iface = GSIGNOND_DBUS_IDENTITY (self);
    gint32 id = 0;
    GError *error = NULL;

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_iface_remove_reference (self->priv->identity, reference, &self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_remove_reference (iface, invocation, id);
    } else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    return TRUE;
}

static void
_emit_info_updated (GSignondIdentityIface *identity,
                   GSignondIdentityChangeType change,
                   gpointer userdata)
{
    GSignondDbusIdentity *self = GSIGNOND_DBUS_IDENTITY (userdata);

    gsignond_dbus_identity_emit_info_updated (self, change);
}

/**
 * gsignond_dbus_identity_adapter_new:
 * @impl: Instance of #GSignondDbusIdentityAdapter
 *
 * Creates new instance of #GSignondDbusIdentityAdapter 
 *
 * Retrurns: (transfer full) new instance of #GSignondDbusIdentityAdapter
 */
GSignondDbusIdentityAdapter * 
gsignond_dbus_identity_adapter_new (GSignondIdentityIface *impl)
{
    return GSIGNOND_DBUS_IDENTITY_ADAPTER (g_object_new (GSIGNOND_TYPE_DBUS_IDENTITY_ADAPTER, "identity-impl", impl, NULL));
}

#undef PREPARE_SECURITY_CONTEXT
