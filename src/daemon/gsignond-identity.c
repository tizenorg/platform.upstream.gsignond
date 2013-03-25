/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
            Amarnath Valluri <amarnath.valluri@linux.intel.com>
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

#include "gsignond-identity.h"

#include <string.h>

#include "gsignond-daemon.h"
#include "gsignond-identity-iface.h"
#include "gsignond-auth-session.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-config-dbus.h"
#include "gsignond/gsignond-signonui.h"
#include "dbus/gsignond-dbus.h"
#include "dbus/gsignond-dbus-identity-adapter.h"
#include "plugins/gsignond-plugin-proxy-factory.h"

enum 
{
    PROP_0,
    PROP_INFO,
    PROP_APP_CONTEXT,
    N_PROPERTIES
};

enum {
    SIG_STORE,
    SIG_REMOVE,
    SIG_VERIFY_USER,
    SIG_VERIFY_SECRET,
    SIG_ADD_REFERENCE,
    SIG_REMOVE_REFERENCE,
    SIG_SIGNOUT,
    SIG_MAX
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[SIG_MAX];

struct _GSignondIdentityPrivate
{
    GSignondIdentityInfo *info;
    GSignondAuthServiceIface *owner;
    GSignondDbusIdentityAdapter *identity_adapter;
    GList *auth_sessions;
};

static void
gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondIdentity, gsignond_identity, GSIGNOND_TYPE_DISPOSABLE, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_IDENTITY_IFACE, 
                                               gsignond_identity_iface_init));


#define GSIGNOND_IDENTITY_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentityPrivate)

#define VALIDATE_IDENTITY_READ_ACCESS(identity, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_auth_service_iface_get_acm (identity->priv->owner); \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (identity->priv->info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, acl); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("cannot access identity."); \
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "identity can not be accessed"); \
        return ret; \
    } \
}

#define VALIDATE_IDENTITY_WRITE_ACCESS(identity, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_auth_service_iface_get_acm (identity->priv->owner); \
    GSignondSecurityContext *owner = gsignond_identity_info_get_owner (identity->priv->info); \
    gboolean valid = gsignond_access_control_manager_peer_is_owner_of_identity (acm, ctx, owner); \
    gsignond_security_context_free (owner); \
    if (!valid) { \
        WARN ("is_owner_of_identity check failed."); \
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "identity can not be accessed"); \
        return ret; \
    } \
}

#define VALIDATE_IDENTITY_WRITE_ACL(identity, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_auth_service_iface_get_acm (identity->priv->owner); \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (identity->priv->info); \
    gboolean valid = gsignond_access_control_manager_acl_is_valid (acm, ctx, acl); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("acl validity check failed."); \
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "invalid access control list"); \
        return ret; \
    } \
}

static gboolean 
_set_id (GSignondIdentity *identity, guint32 id)
{
    gsignond_identity_info_set_id (identity->priv->info, id);
    g_object_notify_by_pspec (G_OBJECT(identity), properties[PROP_INFO]);

    return TRUE;
}

static void
_get_property (GObject *object, guint property_id, GValue *value,
               GParamSpec *pspec)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY (object);

    switch (property_id)
    {
        case PROP_INFO:
            g_value_set_boxed (value, self->priv->info);
            break;
        case PROP_APP_CONTEXT:
            g_object_get_property (G_OBJECT (self->priv->identity_adapter), "app-context", value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_set_property (GObject *object, guint property_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY (object);

    switch (property_id)
    {
        case PROP_INFO:
            self->priv->info =
               (GSignondIdentityInfo *)g_value_get_boxed (value);
            break;
        case PROP_APP_CONTEXT:
            g_object_set_property (G_OBJECT (self->priv->identity_adapter), "app-context", value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_finalize_session (gpointer data, gpointer user_data)
{
    (void) user_data;

    DBG("finalize session %p", user_data);
    g_object_unref (G_OBJECT (data));
}

static void
_dispose (GObject *object)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(object);

    if (self->priv->identity_adapter) {
        g_object_unref (self->priv->identity_adapter);
        self->priv->identity_adapter = NULL;
    }

    if (self->priv->owner) {
        DBG("unref owner %p", self->priv->owner);
        g_object_unref (self->priv->owner);
        self->priv->owner = NULL;
    }

    if (self->priv->info) {
        gsignond_identity_info_unref (self->priv->info);
        self->priv->info = NULL;
    }

    if (self->priv->auth_sessions) {
        g_list_foreach (self->priv->auth_sessions, _finalize_session, NULL);
    }

    G_OBJECT_CLASS (gsignond_identity_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(object);

    if (self->priv->auth_sessions) {
        g_list_free (self->priv->auth_sessions);
        self->priv->auth_sessions = NULL;
    }

    G_OBJECT_CLASS (gsignond_identity_parent_class)->finalize (object);
}

static void
gsignond_identity_init (GSignondIdentity *self)
{
    self->priv = GSIGNOND_IDENTITY_PRIV(self);

    self->priv->identity_adapter =
        gsignond_dbus_identity_adapter_new (GSIGNOND_IDENTITY_IFACE (self));
    self->priv->auth_sessions = NULL;
}

static void
gsignond_identity_class_init (GSignondIdentityClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondIdentityPrivate));

    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    properties[PROP_INFO] =
        g_param_spec_boxed ("info", 
                            "identity info", 
                            "IdentityInfo structure",
                            GSIGNOND_TYPE_IDENTITY_INFO,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    
    properties[PROP_APP_CONTEXT] = g_param_spec_string (
                "app-context",
                "application security context",
                "Application security context of the identity object creater",
                NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


    g_object_class_install_properties (object_class, N_PROPERTIES, properties);

    signals[SIG_REMOVE] = g_signal_new ("remove",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST| G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_BOOLEAN,
                  0,
                  G_TYPE_NONE);
    signals[SIG_STORE] = g_signal_new ("store",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_UINT,
                  1,
                  GSIGNOND_TYPE_IDENTITY_INFO);
    signals[SIG_ADD_REFERENCE] = g_signal_new ("add-reference",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_INT,
                  1,
                  G_TYPE_STRING);
    signals[SIG_REMOVE_REFERENCE] = g_signal_new ("remove-reference",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_INT,
                  1,
                  G_TYPE_STRING);
    signals[SIG_SIGNOUT] = g_signal_new ("signout",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_BOOLEAN,
                  0,
                  G_TYPE_NONE);
}

static GVariant * 
_get_info (GSignondIdentityIface *iface, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    GSignondIdentity *identity = GSIGNOND_IDENTITY (iface);
    GSignondIdentityInfo *info = NULL;
    GVariant *vinfo = NULL;

    if (!identity->priv->info) {
        WARN ("assertion (identity->priv->info) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity not found.");
        return NULL;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, NULL);

    info = gsignond_identity_info_copy (identity->priv->info);

    /* remove password */
    gsignond_identity_info_remove_secret (info);

    /* remove username if its secret */
    if (gsignond_identity_info_get_is_username_secret (info))
        gsignond_identity_info_remove_username (info);

    /* prepare identity info, excluding password and username if secret */
    vinfo = gsignond_dictionary_to_variant (identity->priv->info);
    gsignond_identity_info_unref (info);
    if (!vinfo) {
        WARN ("identity info to variant convertion failed.");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity internal eror.");
        return NULL;
    }

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    return vinfo;
}

static void
_on_dialog_refreshed (GError *error, gpointer user_data)
{
    GSignondAuthSessionIface *session = GSIGNOND_AUTH_SESSION_IFACE (user_data);

    if (error) {
        WARN ("Error : %s", error->message);
        g_error_free (error);
    }

    if (session) {
        /*
         * FIXME: whom to pass the reply ? 
         */
    }
}

static void
_on_refresh_dialog (GSignondAuthSessionIface *session, GSignondSignonuiData *ui_data, gpointer userdata)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (userdata);

    gsignond_daemon_refresh_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT (session),
            ui_data, _on_dialog_refreshed, (gpointer)session);
}

static void
_on_refresh_requested (GSignondSignonuiData *ui_data, gpointer user_data)
{
    GSignondAuthSessionIface *session = GSIGNOND_AUTH_SESSION_IFACE (user_data);
    gsignond_auth_session_iface_refresh (session, ui_data);
}

static void
_on_user_action_completed (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondAuthSessionIface *session = GSIGNOND_AUTH_SESSION_IFACE (user_data);
    if (error) {
        WARN ("UI-Error: %s", error->message);
        g_error_free (error);
        return;
    }
    if (session) {
        gsignond_auth_session_iface_user_action_finished (session, reply);
    }
    else if (reply) gsignond_signonui_data_unref (reply);
}

static void
_on_user_action_required (GSignondAuthSessionIface *session, GSignondSignonuiData *ui_data, gpointer userdata)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (userdata);

    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT(session), 
            ui_data, _on_user_action_completed, _on_refresh_requested, session);
}

static void
_on_session_close (gpointer data, GObject *session)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (data);

    DBG ("identity %p session %p disposed", identity, session);
    identity->priv->auth_sessions = g_list_remove (identity->priv->auth_sessions, session);
    
    if (g_list_length (identity->priv->auth_sessions) == 0) {
        gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));
        gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), TRUE);
    }
}

static const gchar *
_get_auth_session (GSignondIdentityIface *iface, const gchar *method, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY (iface);
    GSignondAuthSession *session = NULL;
    const gchar *object_path = NULL;
    GHashTable *supported_methods = NULL;
    gboolean method_available = FALSE;
    gchar *app_context = NULL;
    gint timeout = 0;

    if (!method) {
        WARN ("assertion (method) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_KNOWN,
                      "authentication method not provided");
        return NULL;
    }
    DBG ("create auth session for method %s", method);

    if (!gsignond_plugin_proxy_factory_get_plugin_mechanisms (
                                                              gsignond_get_plugin_proxy_factory (),
                                                              method)) {
        WARN ("method '%s' doesn't exist", method);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_KNOWN,
                                                        "authentication method '%s' doesn't exist",
                                                        method);
        return NULL;
    }

    supported_methods = gsignond_identity_info_get_methods (identity->priv->info);

    if (supported_methods) {
        method_available = g_hash_table_contains (supported_methods, method);
        g_hash_table_unref (supported_methods);
    } else if (gsignond_identity_info_get_is_identity_new (
                                                         identity->priv->info))
        method_available = TRUE;
    else
        method_available = FALSE;


    if (!method_available) {
        WARN ("authentication method '%s' is not supported", method);
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_AVAILABLE,
                      "authentication method '%s' not supported for this identity", method);
        return NULL;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, NULL);

    timeout = gsignond_config_get_integer (gsignond_get_config(), GSIGNOND_CONFIG_DBUS_AUTH_SESSION_TIMEOUT);
    g_object_get (identity->priv->identity_adapter, "app-context", &app_context, NULL);
    session = gsignond_auth_session_new (identity->priv->info,
                                         app_context, 
                                         method,
                                         timeout);
    g_free (app_context);

    if (!session) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }
    /* Handle 'ui' signanls on session */
    g_signal_connect (session, "process-user-action-required", G_CALLBACK (_on_user_action_required), identity);
    g_signal_connect (session, "process-refreshed", G_CALLBACK (_on_refresh_dialog), identity);

    object_path = gsignond_auth_session_get_object_path (session);

    identity->priv->auth_sessions = g_list_append (identity->priv->auth_sessions, session);

    g_object_weak_ref (G_OBJECT (session), _on_session_close, identity);

    /* Keep live till all active sessions closes */
    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), FALSE);

    DBG ("session %p creation for method %s complete", session, method);

    return object_path;
}

static void
_on_query_dialog_done (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (user_data);
    guint32 id = 0;
    GError *err = NULL;
    GSignondSignonuiError err_id = 0;

    if (error) {
        WARN ("failed to verfiy user : %s", error->message);
        g_error_free (error);

        err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, "Operation cancled");
    }

    err_id = gsignond_signonui_data_get_query_error (reply);
    if (err_id != SIGNONUI_ERROR_NONE) {
        switch (err_id) {
            case SIGNONUI_ERROR_CANCELED:
                err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED,
                        "Operation cancled");
                break;
            default:
                err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, 
                        "signon ui returned with error : %d", err_id);
                break;
        }
    }
    else {
        const gchar *secret = gsignond_signonui_data_get_password (reply);

        if (!secret) {
            err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER,
                                "Server internal error occured");
        } else if (identity->priv->info) {
            gsignond_identity_info_set_secret (identity->priv->info, secret) ;

            /* Save new secret in db */
            g_signal_emit (identity, signals[SIG_STORE], 0, identity->priv->info, &id);
            if (!id) err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_STORE_FAILED, "Failed to store secret");
        }
    }

    gsignond_signonui_data_unref (reply);

    gsignond_identity_iface_notify_credentials_updated (GSIGNOND_IDENTITY_IFACE (identity), id, err);

    if (err) g_error_free (err);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), TRUE);
    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));
}

static gboolean
_request_credentials_update (GSignondIdentityIface *iface, const gchar *message, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    GSignondIdentity *identity = GSIGNOND_IDENTITY (iface);
    GSignondSignonuiData *ui_data = NULL;

    if (!(identity && identity->priv->info)) {
        WARN ("assertion (identity && identity->priv->info) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity not found.");
        return FALSE;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    if (!gsignond_identity_info_get_store_secret (identity->priv->info)) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE, "Password can not be stored.");
        return FALSE;
    }

    ui_data = gsignond_signonui_data_new ();

    gsignond_signonui_data_set_query_username (ui_data, TRUE);
    gsignond_signonui_data_set_username (ui_data, gsignond_identity_info_get_username (identity->priv->info));
    gsignond_signonui_data_set_caption (ui_data, gsignond_identity_info_get_caption (identity->priv->info));
    gsignond_signonui_data_set_message (ui_data, message);
  
    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT(identity),
        ui_data, _on_query_dialog_done, NULL, identity);

    gsignond_signonui_data_unref (ui_data);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), FALSE);

    return TRUE;
}

static void
_on_user_verfied (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (user_data);
    gboolean res = FALSE;
    GError *err = NULL;
    GSignondSignonuiError err_id = 0;

    if (error) {
        WARN ("failed to verfiy user : %s", error->message);
        g_error_free (error);

        err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, "Operation cancled");
    }

    err_id = gsignond_signonui_data_get_query_error (reply);
    if (err_id != SIGNONUI_ERROR_NONE) {
        switch (err_id) {
            case SIGNONUI_ERROR_CANCELED:
                err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED,
                        "Operation cancled");
                break;
            case SIGNONUI_ERROR_FORGOT_PASSWORD:
                err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_FORGOT_PASSWORD, "Forgot password");
                break;
            default:
                err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER, 
                        "signon ui returned error : %d", err_id);
                break;
        }
    }
    else {
        const gchar *secret = gsignond_signonui_data_get_password (reply);

        if (!secret) {
            err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_INTERNAL_SERVER,
                                "Server internal error occured");
        } else if (identity->priv->info) {
            res = g_strcmp0 (secret, gsignond_identity_info_get_secret 
                                       (identity->priv->info)) == 0;
        }
    }

    gsignond_signonui_data_unref (reply);

    gsignond_identity_iface_notify_user_verified (GSIGNOND_IDENTITY_IFACE (identity), res, err);

    if (err) g_error_free (err);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), TRUE);
    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));
}

static gboolean 
_verify_user (GSignondIdentityIface *iface, GVariant *params, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY (iface);
    const gchar *passwd = 0;
    GSignondSignonuiData *ui_data = NULL;

    if (!identity->priv->info) {
        WARN ("assertion (identity->priv->info) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity not found.");
        return FALSE;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    if (!gsignond_identity_info_get_store_secret (identity->priv->info) ||
        !(passwd = gsignond_identity_info_get_secret (identity->priv->info)) ||
        !strlen (passwd)) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE,
                                                        "user can not be verified as credentials are not stored");
        return FALSE;
    }

    ui_data = gsignond_signonui_data_new_from_variant (params);
    gsignond_signonui_data_set_query_password (ui_data, TRUE);
    gsignond_signonui_data_set_username (ui_data, gsignond_identity_info_get_username (identity->priv->info));
    gsignond_signonui_data_set_caption (ui_data, gsignond_identity_info_get_caption (identity->priv->info));
   
    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT (identity),
        ui_data, _on_user_verfied, NULL, identity);

    gsignond_signonui_data_unref (ui_data);

    gsignond_disposable_set_auto_dispose (GSIGNOND_DISPOSABLE (identity), FALSE);

    return TRUE;
}

static gboolean
_verify_secret (GSignondIdentityIface *iface, const gchar *secret, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Not supported");

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    return FALSE;
}

static gboolean 
_sign_out (GSignondIdentityIface *iface, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);
    gboolean success = FALSE;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    /*
     * TODO: close all auth_sessions and emit "identity-signed-out"
     */
    g_signal_emit (iface,
                   signals[SIG_SIGNOUT],
                   0,
                   &success);

    if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Not supported");

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    return success;
}

static guint32
_store (GSignondIdentityIface *iface, const GVariant *info, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);
    GSignondIdentityInfo *identity_info = NULL;
    gboolean was_new_identity = FALSE;
    GSignondSecurityContextList *contexts = NULL;
    GSignondSecurityContext *owner = NULL;
    guint32 id;

    VALIDATE_IDENTITY_WRITE_ACCESS (identity, ctx, 0);

    identity_info = gsignond_dictionary_new_from_variant ((GVariant *)info);
    /* dont trust 'identity id' passed via 'info' */
    id = gsignond_identity_info_get_id (identity->priv->info);
    gsignond_identity_info_set_id (identity_info, id);

    was_new_identity = gsignond_identity_info_get_is_identity_new (identity_info);

    contexts = gsignond_identity_info_get_access_control_list (identity_info);
    if (!contexts) {
        contexts = gsignond_identity_info_get_access_control_list (identity->priv->info);
        gsignond_identity_info_set_access_control_list (identity_info, contexts);
        gsignond_security_context_list_free (contexts);
    }
    else {
        VALIDATE_IDENTITY_WRITE_ACL (identity, ctx, 0);
    }
   
    owner = gsignond_identity_info_get_owner (identity_info);
    if (!owner) {
        owner = gsignond_identity_info_get_owner (identity->priv->info);
        gsignond_identity_info_set_owner (identity_info, owner);
    }

    /* update object cache */
    if (identity->priv->info)
        gsignond_identity_info_unref (identity->priv->info);
    identity->priv->info = identity_info;

    /* Ask daemon to store identity info to db */
    g_signal_emit (identity,
                   signals[SIG_STORE],
                   0,
                   identity_info, 
                   &id);

    if (!id) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_STORE_FAILED,
                                                        "Failed to store identity");
    }
    else {
        if (was_new_identity) 
            _set_id (identity, id);

        gsignond_identity_iface_notify_info_updated (iface, GSIGNOND_IDENTITY_DATA_UPDATED);
    }
 
    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    return id;
}

static gboolean
_remove (GSignondIdentityIface *iface, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);
    gboolean is_removed = FALSE;
    
    VALIDATE_IDENTITY_WRITE_ACCESS (identity, ctx, FALSE);

    g_signal_emit (identity,
                   signals[SIG_REMOVE],
                   0,
                   &is_removed);

    if (is_removed)
        gsignond_identity_iface_notify_info_updated (iface, GSIGNOND_IDENTITY_REMOVED);
    else if (error)
        *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_REMOVE_FAILED, "failed to remove identity");

    gsignond_disposable_delete_later (GSIGNOND_DISPOSABLE (identity));

    return is_removed;
}

static gint32
_add_reference (GSignondIdentityIface *iface, const gchar *reference, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }
    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);
    gint32 res = 0;
    
    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, 0);

    g_signal_emit (iface,
                   signals[SIG_ADD_REFERENCE],
                   0,
                   reference,
                   &res);

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    if (res == 0) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
    }

    return res;
}

static gint32
_remove_reference (GSignondIdentityIface *iface, const gchar *reference, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(iface && GSIGNOND_IS_IDENTITY (iface))) {
        WARN ("assertion (iface && GSIGNOND_IS_IDENTITY(iface)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }

    GSignondIdentity *identity = GSIGNOND_IDENTITY(iface);
    gint32 res = 0;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, 0);

    g_signal_emit (iface,
            signals[SIG_REMOVE_REFERENCE],
            0,
            reference,
            &res);

    gsignond_disposable_set_keep_in_use (GSIGNOND_DISPOSABLE (identity));

    if (res == 0) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_REFERENCE_NOT_FOUND,
                                                        "reference '%s' not found", reference);
    }
    return res;
}

static GSignondAccessControlManager *
_get_acm (GSignondIdentityIface *iface)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (iface);

    g_return_val_if_fail (identity, NULL);

    return gsignond_auth_service_iface_get_acm (identity->priv->owner);
}

static void
gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondIdentityIfaceInterface *identity_iface =
        (GSignondIdentityIfaceInterface *) g_iface;

    (void)iface_data;

    identity_iface->request_credentials_update = _request_credentials_update;
    identity_iface->get_info = _get_info;
    identity_iface->get_auth_session = _get_auth_session;
    identity_iface->verify_user = _verify_user;
    identity_iface->verify_secret = _verify_secret;
    identity_iface->remove = _remove;
    identity_iface->sign_out = _sign_out;
    identity_iface->store = _store;
    identity_iface->add_reference = _add_reference;
    identity_iface->remove_reference = _remove_reference;
    identity_iface->get_acm = _get_acm;
}

/**
 * gsignond_identity_get_id:
 * @identity: instance of #GSignondIdentity
 * 
 * Retrieves identity id.
 *
 * Returns: identity id
 */
guint32 
gsignond_identity_get_id (GSignondIdentity *identity)
{
    g_assert (GSIGNOND_IS_IDENTITY (identity));

    return gsignond_identity_info_get_id (identity->priv->info);
}

/**
 * gsignond_identity_get_identity_info:
 * @identity: instance of #GSignondIdentity
 *
 * Retrieves identity's #GSignondIdentityInfo.
 *
 * Returns: (transfer none) #GSignondIdentityInfo
 */
GSignondIdentityInfo *
gsignond_identity_get_identity_info (GSignondIdentity *identity)
{
    g_assert (GSIGNOND_IS_IDENTITY (identity));
    g_assert (identity->priv != NULL);

    return identity->priv->info;
}

/**
 * gsignond_identity_get_object_path:
 * @identity: instance of #GSignondIdentity
 * 
 * Retrieves the dbus object path of the identity.
 *
 * Returns[transfer null]: Dbus object path used by this identity.
 */
const gchar *
gsignond_identity_get_object_path (GSignondIdentity *identity)
{
    return gsignond_dbus_identity_adapter_get_object_path (identity->priv->identity_adapter);
}

/**
 * gsignond_identity_new:
 * @owner: Owner of this object, instance of #GSignondAuthServiceIface
 * @info (transfer full): Identity info, instance of #GSignondIdentityInfo
 * @app_context: application security context
 * 
 * Creates new instance of #GSignondIdentity
 *
 * Returns[transfer full]: new instance of #GSignondIdentity
 */
GSignondIdentity * gsignond_identity_new (GSignondAuthServiceIface *owner,
                                          GSignondIdentityInfo *info,
                                          const gchar *app_context,
                                          gint timeout)
{
    GSignondIdentity *identity =
        GSIGNOND_IDENTITY(g_object_new (GSIGNOND_TYPE_IDENTITY,
                                        "info", info,
                                        "app-context", app_context,
                                        "timeout", timeout,
                                        NULL));

    identity->priv->owner = g_object_ref (owner);

    return identity;
}
