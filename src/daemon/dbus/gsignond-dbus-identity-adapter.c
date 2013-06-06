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
#include "gsignond/gsignond-log.h"
#include "gsignond-dbus-identity-adapter.h"
#include "gsignond-dbus-auth-session-adapter.h"
#include "gsignond-dbus.h"

enum
{
    PROP_0,

    PROP_CONNECTION,
    PROP_IDENTITY,
    PROP_APP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

typedef struct {
    GSignondDbusIdentityAdapter *adapter;
    GDBusMethodInvocation *invocation;
    gpointer user_data;
} _IdentityDbusInfo;

static _IdentityDbusInfo*
_identity_dbus_info_new (GSignondDbusIdentityAdapter *self, GDBusMethodInvocation *invocation, gpointer userdata)
{
    _IdentityDbusInfo *info = g_new0(_IdentityDbusInfo, 1);

    info->adapter = g_object_ref (self);
    info->invocation = g_object_ref (invocation);
    info->user_data = userdata;

    return info;
}

static void
_identity_dbus_info_free (_IdentityDbusInfo *info)
{
    if (!info) return ;

    if (info->adapter) g_object_unref (info->adapter);
    if (info->invocation) g_object_unref (info->invocation);

    g_free (info);
}

struct _GSignondDbusIdentityAdapterPrivate
{
    GDBusConnection   *connection;
    GSignondDbusIdentity *dbus_identity;
    GSignondIdentity  *identity;
    gchar *app_context;
    GSignondSecurityContext *sec_context;
    GList *sessions;
    /* signal handler ids */
    guint info_updated_handler_id;
    guint verify_user_handler_id;
    guint verify_secret_handler_id;
    guint credentials_update_handler_id;
};

G_DEFINE_TYPE (GSignondDbusIdentityAdapter, gsignond_dbus_identity_adapter, GSIGNOND_TYPE_DISPOSABLE)

#define GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(obj) \
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_DBUS_IDENTITY_ADAPTER, GSignondDbusIdentityAdapterPrivate)

#define PREPARE_SECURITY_CONTEXT(dbus_object, invocation) \
{ \
    GSignondDbusIdentityAdapterPrivate *priv = dbus_object->priv;\
    GSignondAccessControlManager *acm = gsignond_identity_get_acm (priv->identity);\
    if (acm) { \
        const gchar *sender = NULL; \
        int fd = -1; \
        sender = g_dbus_method_invocation_get_sender (invocation);\
        if (!sender) {\
            GDBusConnection *connection = g_dbus_method_invocation_get_connection (invocation);\
            fd = g_socket_get_fd (g_socket_connection_get_socket (G_SOCKET_CONNECTION (g_dbus_connection_get_stream(connection))));\
        }\
        gsignond_access_control_manager_security_context_of_peer( \
            acm, \
            priv->sec_context, \
            fd, \
            sender, \
            priv->app_context); \
    }\
}

static gboolean _handle_request_credentials_update (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar*, gpointer);
static gboolean _handle_get_info (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_get_auth_session (GSignondDbusIdentityAdapter *self, GDBusMethodInvocation *invocation, const gchar *method, gpointer user_data);
static gboolean _handle_verify_user (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, GVariant *, gpointer);
static gboolean _handle_verify_secret (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static gboolean _handle_remove (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_sign_out (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, gpointer);
static gboolean _handle_store (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const GVariant *, gpointer);
static gboolean _handle_add_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static gboolean _handle_remove_reference (GSignondDbusIdentityAdapter *, GDBusMethodInvocation *, const gchar *, gpointer);
static void _emit_info_updated (GSignondIdentity *identity, GSignondIdentityChangeType change, gpointer userdata);

static void
gsignond_dbus_identity_adapter_set_property (GObject *object,
        guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    switch (property_id) {
        case PROP_CONNECTION: {
            self->priv->connection = g_value_get_object (value);
            break;
        }
        case PROP_IDENTITY: {
            gpointer identity = g_value_get_object (value);
            if (identity) {
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
                    if (self->priv->credentials_update_handler_id) {
                        g_signal_handler_disconnect (self->priv->identity, self->priv->credentials_update_handler_id);
                        self->priv->credentials_update_handler_id = 0;
                    }
                    g_object_unref (self->priv->identity);
                    self->priv->identity = NULL;
                }
                self->priv->identity = GSIGNOND_IDENTITY(identity);
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
        case PROP_IDENTITY:{
            g_value_set_object (value, self->priv->identity);
            break;
        }
        case PROP_CONNECTION: {
            g_value_set_object (value, self->priv->connection);
            break;
        }
        case PROP_APP_CONTEXT:{
            g_value_set_string (value, self->priv->app_context);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_destroy_session (gpointer data, gpointer user_data)
{
    (void)user_data;
    if (data) g_object_unref (G_OBJECT(data));
}

static void
gsignond_dbus_identity_adapter_dispose (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

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

        if (self->priv->credentials_update_handler_id) {
            g_signal_handler_disconnect (self->priv->identity, self->priv->credentials_update_handler_id);
            self->priv->credentials_update_handler_id = 0;
        }

        g_object_unref (self->priv->identity);
        self->priv->identity = NULL;
    }

    if (self->priv->sessions) {
        g_list_foreach (self->priv->sessions, _destroy_session, NULL);
    }

    if (self->priv->dbus_identity) {
        GDBusInterfaceSkeleton *iface = G_DBUS_INTERFACE_SKELETON(self->priv->dbus_identity);
        gsignond_dbus_identity_emit_unregistered (self->priv->dbus_identity);
        DBG("(-)'%s' object unexported", g_dbus_interface_skeleton_get_object_path (iface));
        g_dbus_interface_skeleton_unexport (iface);
        g_object_unref (self->priv->dbus_identity);
        self->priv->dbus_identity = NULL;
    }

    if (self->priv->connection) {
        g_object_unref (self->priv->connection);
        self->priv->connection = NULL;
    }

    G_OBJECT_CLASS (gsignond_dbus_identity_adapter_parent_class)->dispose (object);
}

static void
gsignond_dbus_identity_adapter_finalize (GObject *object)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (object);

    if (self->priv->sec_context) {
        gsignond_security_context_free (self->priv->sec_context);
        self->priv->sec_context = NULL;
    }

    if (self->priv->sessions) {
        g_list_free (self->priv->sessions);
        self->priv->sessions = NULL;
    }

    if (self->priv->app_context) {
        g_free (self->priv->app_context);
        self->priv->app_context = NULL;
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

    properties[PROP_IDENTITY] = g_param_spec_object ("identity",
                                                     "Core identity object",
                                                     "Core Identity Object",
                                                     GSIGNOND_TYPE_IDENTITY,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY | 
                                                     G_PARAM_STATIC_STRINGS);
    properties[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                       "bus connection used",
                                                       "Bus connection",
                                                       G_TYPE_DBUS_CONNECTION,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS);
    properties[PROP_APP_CONTEXT] = g_param_spec_string (
                "app-context",
                "application security context",
                "Application security context of the identity object creater",
                NULL,
                G_PARAM_READWRITE |G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_dbus_identity_adapter_init (GSignondDbusIdentityAdapter *self)
{
    self->priv = GSIGNOND_DBUS_IDENTITY_ADAPTER_GET_PRIV(self);

    self->priv->connection = 0;
    self->priv->identity = 0;
    self->priv->app_context = 0;
    self->priv->dbus_identity = gsignond_dbus_identity_skeleton_new();
    self->priv->sec_context = gsignond_security_context_new ();

    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-request-credentials-update", G_CALLBACK (_handle_request_credentials_update), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-get-info", G_CALLBACK(_handle_get_info), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-get-auth-session", G_CALLBACK(_handle_get_auth_session), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-verify-user", G_CALLBACK(_handle_verify_user), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-verify-secret", G_CALLBACK(_handle_verify_secret), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-remove", G_CALLBACK(_handle_remove), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-sign-out", G_CALLBACK(_handle_sign_out), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-store", G_CALLBACK(_handle_store), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-add-reference", G_CALLBACK(_handle_add_reference), self);
    g_signal_connect_swapped (self->priv->dbus_identity,
            "handle-remove-reference", G_CALLBACK(_handle_remove_reference), self);
}

static void
_on_credentials_updated (_IdentityDbusInfo *info, guint32 updated_id, GError *error, gpointer userdata)
{
    g_assert (info);
    
    GSignondDbusIdentityAdapter *self = info->adapter;
    GDBusMethodInvocation *invocation = info->invocation;

    g_signal_handler_disconnect (userdata, self->priv->credentials_update_handler_id);
    self->priv->credentials_update_handler_id = 0;
    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else
    {
        gsignond_dbus_identity_complete_request_credentials_update (
                self->priv->dbus_identity, invocation, updated_id);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    _identity_dbus_info_free (info);
}

static gboolean
_handle_request_credentials_update (GSignondDbusIdentityAdapter *self,
                                    GDBusMethodInvocation *invocation,
                                    const gchar *message,
                                    gpointer user_data)
{
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    gsignond_identity_request_credentials_update (self->priv->identity, message, self->priv->sec_context, &error);
    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
 
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);
    }
    else {
        self->priv->credentials_update_handler_id = g_signal_connect_swapped (self->priv->identity,
            "credentials-updated", G_CALLBACK (_on_credentials_updated), self);
    }

    return TRUE;
}

static gboolean
_handle_get_info (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GVariant *identity_data = 0;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);
    
    identity_data = gsignond_identity_get_info (self->priv->identity, 
        self->priv->sec_context, &error);

    if (identity_data) {
        gsignond_dbus_identity_complete_get_info (
            self->priv->dbus_identity, invocation, identity_data);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static void
_on_session_disposed (gpointer data, GObject *session)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (data);

    if (!self) return;

    self->priv->sessions = g_list_remove (self->priv->sessions, session);

    if (g_list_length (self->priv->sessions) == 0){
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE(self), TRUE);
    }
}

static gboolean
_handle_get_auth_session (GSignondDbusIdentityAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *method,
                          gpointer user_data)
{
    GSignondAuthSession *session = NULL;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    session = gsignond_identity_get_auth_session (self->priv->identity, method, self->priv->sec_context, &error);

    if (session) {
        guint timeout =gsignond_identity_get_auth_session_timeout (self->priv->identity);
        GSignondDbusAuthSessionAdapter *dbus_session = gsignond_dbus_auth_session_adapter_new_with_connection (
            g_object_ref (self->priv->connection), session, self->priv->app_context, timeout);

        if (g_list_length (self->priv->sessions) == 0)
            gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

        self->priv->sessions = g_list_append (self->priv->sessions, dbus_session);

        g_object_weak_ref (G_OBJECT (dbus_session), _on_session_disposed, self);

        gsignond_dbus_identity_complete_get_auth_session (
            self->priv->dbus_identity, invocation, 
            gsignond_dbus_auth_session_adapter_get_object_path (dbus_session));
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE(self), TRUE);

    return TRUE;
}

static void
_on_user_verfied (_IdentityDbusInfo *info, gboolean res, const GError *error, gpointer user_data)
{
    if (!info) {
        WARN ("assertion G_UNLIKELY (info) failed");
        return ;
    }

    GSignondDbusIdentityAdapter *self = info->adapter;
    GDBusMethodInvocation *invocation = info->invocation;

    g_signal_handler_disconnect (user_data, self->priv->verify_user_handler_id);
    self->priv->verify_user_handler_id = 0;

    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
    }
    else {
        gsignond_dbus_identity_complete_verify_user (
            self->priv->dbus_identity, invocation, res);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    _identity_dbus_info_free (info);
}

static gboolean
_handle_verify_user (GSignondDbusIdentityAdapter *self,
                     GDBusMethodInvocation *invocation,
                     GVariant *params,
                     gpointer user_data)
{
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    gsignond_identity_verify_user (self->priv->identity, params, self->priv->sec_context, &error);

    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);

        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);
    }
    else {
        _IdentityDbusInfo *info = _identity_dbus_info_new (self, invocation, NULL);

        /* FIXME: Do we allow multiple calls at a given point of time */
        self->priv->verify_user_handler_id = g_signal_connect_swapped (self->priv->identity, 
                    "user-verified", G_CALLBACK (_on_user_verfied), (gpointer)info);
    }

    return TRUE;
}

static void
_on_secret_verfied (_IdentityDbusInfo *info, gboolean res, const GError *error, gpointer user_data)
{
    g_assert (info);

    GSignondDbusIdentityAdapter *self = info->adapter;
    GDBusMethodInvocation *invocation = info->invocation;

    g_signal_handler_disconnect (user_data, self->priv->verify_secret_handler_id);
    self->priv->verify_secret_handler_id = 0;

    if (error) {
        g_dbus_method_invocation_return_gerror (info->invocation, error);
    }
    else {
        gsignond_dbus_identity_complete_verify_secret (
            self->priv->dbus_identity, invocation, res);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE(self), TRUE);

    _identity_dbus_info_free (info);
}

static gboolean
_handle_verify_secret (GSignondDbusIdentityAdapter *self,
                      GDBusMethodInvocation *invocation,
                      const gchar *secret,
                      gpointer user_data)
{
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);
    
    gsignond_identity_verify_secret (self->priv->identity, secret, self->priv->sec_context, &error);

    if (error) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);
    }
    else {
        _IdentityDbusInfo *info = _identity_dbus_info_new (self, invocation, NULL);

        self->priv->verify_secret_handler_id = g_signal_connect_swapped (self->priv->identity, 
                "secret-verified", G_CALLBACK (_on_secret_verfied), (gpointer)info);
    }

    return TRUE;
}

static gboolean 
_handle_remove (GSignondDbusIdentityAdapter   *self,
                GDBusMethodInvocation *invocation,
                gpointer               user_data)
{
    GError *error = NULL;
 
    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    if (!gsignond_identity_remove (self->priv->identity, self->priv->sec_context, &error)) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else {
        gsignond_dbus_identity_complete_remove (self->priv->dbus_identity, invocation);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_sign_out (GSignondDbusIdentityAdapter *self,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    GError *error = NULL;
    gboolean res;
 
    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    res = gsignond_identity_sign_out (self->priv->identity, self->priv->sec_context, &error);

    if (!error) {
        gsignond_dbus_identity_complete_sign_out (self->priv->dbus_identity, invocation, res);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_store (GSignondDbusIdentityAdapter *self,
               GDBusMethodInvocation *invocation,
               const GVariant *info,
               gpointer user_data)
{
    guint id = 0;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_store (self->priv->identity, info, self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_store (self->priv->dbus_identity, invocation, id);
    } else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_add_reference (GSignondDbusIdentityAdapter *self,
                       GDBusMethodInvocation *invocation,
                       const gchar *reference,
                       gpointer user_data)
{
    gint32 id = 0;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_add_reference (self->priv->identity, reference, self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_add_reference (self->priv->dbus_identity, invocation, id);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static gboolean
_handle_remove_reference (GSignondDbusIdentityAdapter *self,
                          GDBusMethodInvocation *invocation,
                          const gchar *reference,
                          gpointer user_data)
{
    gint32 id = 0;
    GError *error = NULL;

    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), FALSE);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), FALSE);

    PREPARE_SECURITY_CONTEXT (self, invocation);

    id = gsignond_identity_remove_reference (self->priv->identity, reference, self->priv->sec_context, &error);

    if (id) {
        gsignond_dbus_identity_complete_remove_reference (self->priv->dbus_identity, invocation, id);
    } else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (self), TRUE);

    return TRUE;
}

static void
_emit_info_updated (GSignondIdentity *identity,
                   GSignondIdentityChangeType change,
                   gpointer userdata)
{
    GSignondDbusIdentityAdapter *self = GSIGNOND_DBUS_IDENTITY_ADAPTER (userdata);

    gsignond_dbus_identity_emit_info_updated (self->priv->dbus_identity, change);

    if (change == GSIGNOND_IDENTITY_REMOVED)
        gsignond_disposable_delete_later (GSIGNOND_DISPOSABLE (self));
    else if (change == GSIGNOND_IDENTITY_SIGNED_OUT && self->priv->sessions) {
        /* destroy all sessions on this identity as it's signed out */
        g_list_foreach (self->priv->sessions, _destroy_session, NULL);
        g_list_free (self->priv->sessions);
        self->priv->sessions = NULL;
    }
}

const gchar *
gsignond_dbus_identity_adapter_get_object_path(GSignondDbusIdentityAdapter *self)
{
    g_return_val_if_fail (self && GSIGNOND_IS_DBUS_IDENTITY_ADAPTER (self), NULL);

    return g_dbus_interface_skeleton_get_object_path (
                G_DBUS_INTERFACE_SKELETON (self->priv->dbus_identity));
}

GSignondDbusIdentityAdapter *
gsignond_dbus_identity_adapter_new_with_connection (GDBusConnection *connection,
                                                    GSignondIdentity *identity,
                                                    const gchar *app_context,
                                                    guint timeout)
{
    static guint32 object_counter;
    gchar *object_path = NULL;
    GError *err = NULL;
    GSignondDbusIdentityAdapter *adapter = GSIGNOND_DBUS_IDENTITY_ADAPTER (
        g_object_new (GSIGNOND_TYPE_DBUS_IDENTITY_ADAPTER, 
            "identity", identity, "connection", connection, "app-context", app_context, NULL));

    if (!adapter) return NULL;

    object_path = g_strdup_printf ("%s/Identity_%d", GSIGNOND_DAEMON_OBJECTPATH, object_counter++);
    if (!g_dbus_interface_skeleton_export (
            G_DBUS_INTERFACE_SKELETON (adapter->priv->dbus_identity),
            adapter->priv->connection, object_path, &err)) {
        ERR ("failed to register object: %s", err->message);
        g_error_free (err);
        g_free (object_path);
        g_object_unref (adapter);

        return NULL;
    }
    DBG("(+)'%s' object exported(%p)", object_path, adapter);
    g_free (object_path);

    gsignond_disposable_set_timeout (GSIGNOND_DISPOSABLE (adapter), timeout);

    return adapter;
}

#ifndef USE_P2P
/**
 * gsignond_dbus_identity_adapter_new:
 * @impl: Instance of #GSignondDbusIdentityAdapter
 *
 * Creates new instance of #GSignondDbusIdentityAdapter 
 *
 * Retrurns: (transfer full) new instance of #GSignondDbusIdentityAdapter
 */
GSignondDbusIdentityAdapter * 
gsignond_dbus_identity_adapter_new (GSignondIdentity *identity, const gchar *app_context, guint timeout)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

    if (error) {
        ERR("failed to connect to session bus : %s", error->message);
        g_error_free (error);
        return NULL;
    }

    return gsignond_dbus_identity_adapter_new_with_connection (connection, identity, app_context, timeout);
}

#endif
#undef PREPARE_SECURITY_CONTEXT
