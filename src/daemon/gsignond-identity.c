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

#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-error.h"
#include "gsignond-daemon.h"
#include "gsignond-identity-enum-types.h"
#include "gsignond-auth-session.h"
#include "gsignond/gsignond-config-dbus.h"
#include "common/gsignond-identity-info-internal.h"
#include "plugins/gsignond-plugin-proxy-factory.h"

enum 
{
    PROP_0,
    PROP_INFO,
    N_PROPERTIES
};

enum {
    SIG_USER_VERIFIED,
    SIG_SECRET_VERIFIED,
    SIG_CREDENTIALS_UPDATED,
    SIG_INFO_UPDATED,
 
    SIG_MAX
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[SIG_MAX];

struct _GSignondIdentityPrivate
{
    GSignondIdentityInfo *info;
    GSignondDaemon *owner;
    GHashTable *auth_sessions; // (auth_method,auth_session) table
};

typedef struct _GSignondIdentityCbData
{
    GSignondIdentity *identity;
    GSignondAuthSession *session;
} GSignondIdentityCbData;

G_DEFINE_TYPE (GSignondIdentity, gsignond_identity, G_TYPE_OBJECT);


static void _on_session_dead (gpointer data, GObject *session);
static void _on_refresh_dialog (GSignondAuthSession *session, GSignondSignonuiData *ui_data, gpointer userdata);
static void _on_process_canceled (GSignondAuthSession *session, GSignondIdentityCbData *cb_data);
static void _on_user_action_required (GSignondAuthSession *session, GSignondSignonuiData *ui_data, gpointer userdata);
static void _on_store_token (GSignondAuthSession *session, GSignondDictionary *token_data, gpointer userdata);

#define GSIGNOND_IDENTITY_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentityPrivate)

#define VALIDATE_IDENTITY_READ_ACCESS(identity, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_daemon_get_access_control_manager (identity->priv->owner); \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (identity->priv->info); \
    GSignondSecurityContext *owner = gsignond_identity_info_get_owner (identity->priv->info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, owner, acl); \
    gsignond_security_context_free (owner); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("cannot access identity."); \
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "identity can not be accessed"); \
        return ret; \
    } \
}

#define VALIDATE_IDENTITY_WRITE_ACCESS(identity, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_daemon_get_access_control_manager (identity->priv->owner); \
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
    GSignondAccessControlManager *acm = gsignond_daemon_get_access_control_manager (identity->priv->owner); \
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
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_release_weak_ref_on_session (gpointer key, gpointer value, gpointer data)
{
    GObject *session = G_OBJECT (value);
    g_signal_handlers_disconnect_by_func (session, G_CALLBACK (_on_user_action_required), data);
    g_signal_handlers_disconnect_by_func (session, G_CALLBACK (_on_store_token), data);
    g_object_weak_unref (session, _on_session_dead, data);
}

static void
_dispose (GObject *object)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(object);

    if (self->priv->auth_sessions) {
        g_hash_table_foreach (self->priv->auth_sessions, _release_weak_ref_on_session, self);
        g_hash_table_unref (self->priv->auth_sessions);
        self->priv->auth_sessions = NULL;
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

    G_OBJECT_CLASS (gsignond_identity_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    G_OBJECT_CLASS (gsignond_identity_parent_class)->finalize (object);
}

static void
gsignond_identity_init (GSignondIdentity *self)
{
    self->priv = GSIGNOND_IDENTITY_PRIV(self);

    self->priv->auth_sessions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
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
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                            G_PARAM_STATIC_STRINGS);
    
    g_object_class_install_properties (object_class, N_PROPERTIES, properties);

    signals[SIG_USER_VERIFIED] =  g_signal_new ("user-verified",
                GSIGNOND_TYPE_IDENTITY,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                NULL,
                G_TYPE_NONE,
                2,
                G_TYPE_BOOLEAN,
                G_TYPE_ERROR);

    signals[SIG_SECRET_VERIFIED] = g_signal_new ("secret-verified",
                GSIGNOND_TYPE_IDENTITY,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                NULL,
                G_TYPE_NONE,
                2,
                G_TYPE_BOOLEAN,
                G_TYPE_ERROR);

    signals[SIG_CREDENTIALS_UPDATED] = g_signal_new ("credentials-updated",
                GSIGNOND_TYPE_IDENTITY,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                NULL,
                G_TYPE_NONE,
                2,
                G_TYPE_UINT,
                G_TYPE_ERROR);

    signals[SIG_INFO_UPDATED] = g_signal_new ("info-updated",
                GSIGNOND_TYPE_IDENTITY,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                NULL,
                G_TYPE_NONE,
                1,
                GSIGNOND_TYPE_IDENTITY_CHANGE_TYPE);

}

GVariant * 
gsignond_identity_get_info (GSignondIdentity *identity, const GSignondSecurityContext *ctx, GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    GVariant *vinfo = NULL;

    if (!identity->priv->info) {
        WARN ("assertion (identity->priv->info) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity not found.");
        return NULL;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, NULL);

    /* prepare identity info, excluding password and username if secret */
    vinfo = gsignond_identity_info_to_variant (identity->priv->info);
    if (!vinfo) {
        WARN ("identity info to variant convertion failed.");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity internal eror.");
        return NULL;
    }

    return vinfo;
}

static void
_on_dialog_refreshed (GError *error, gpointer user_data)
{
    GSignondIdentityCbData *cb_data = (GSignondIdentityCbData *)user_data;

    g_signal_handlers_disconnect_by_func(cb_data->session, _on_refresh_dialog, user_data);
    if (error) {
        WARN ("Error : %s", error->message);
        g_error_free (error);
    }
}

static void
_on_refresh_dialog (GSignondAuthSession *session, GSignondSignonuiData *ui_data, gpointer userdata)
{
    GSignondIdentityCbData *cb_data = (GSignondIdentityCbData *) userdata;

    if (!gsignond_daemon_refresh_dialog (GSIGNOND_DAEMON (cb_data->identity->priv->owner), 
            G_OBJECT (cb_data->session), ui_data, _on_dialog_refreshed, userdata)) {
        WARN ("Dialog Refresh Failed");
    }
}

static void
_on_refresh_requested_by_ui (GSignondSignonuiData *ui_data, gpointer user_data)
{
    GSignondIdentityCbData *cb_data = (GSignondIdentityCbData *) user_data;

    /* check for dialog refresh requests */
    g_signal_connect (cb_data->session, "process-refreshed", G_CALLBACK (_on_refresh_dialog), cb_data);
    gsignond_auth_session_refresh (cb_data->session, ui_data);
}

static void
_on_user_action_completed (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondIdentityCbData *cb_data = (GSignondIdentityCbData *) user_data;
    GSignondIdentityPrivate *priv = GSIGNOND_IDENTITY_PRIV (cb_data->identity);
    GSignondSignonuiError ui_error = SIGNONUI_ERROR_NONE;

    g_return_if_fail (cb_data && GSIGNOND_IS_AUTH_SESSION (cb_data->session));

    g_signal_handlers_disconnect_by_func(cb_data->session, _on_refresh_dialog, user_data);
    g_signal_handlers_disconnect_by_func(cb_data->session, _on_process_canceled, user_data);

    if (error) {
        WARN ("UI-Error: %s on identity %d",
              error->message,
              gsignond_identity_info_get_id (priv->info));
        if (cb_data->session) {
            GSignondSignonuiData *reply = gsignond_dictionary_new();
            gsignond_signonui_data_set_query_error (reply, SIGNONUI_ERROR_GENERAL);
            gsignond_auth_session_user_action_finished (cb_data->session, reply);
            gsignond_dictionary_unref(reply);
        }
        g_error_free (error);
        g_slice_free (GSignondIdentityCbData, cb_data);
        return;
    }

    if (gsignond_signonui_data_get_query_error (reply, &ui_error)
        && ui_error != SIGNONUI_ERROR_NONE) {
        WARN ("signonui error %d for identity %d",
              ui_error, gsignond_identity_info_get_id (priv->info));
    }

    if (!gsignond_identity_info_get_validated (priv->info) &&
        ui_error == SIGNONUI_ERROR_NONE) {
        gsignond_identity_info_set_username (priv->info,
                                             gsignond_signonui_data_get_username (reply));
    }

    /* storing secret allowed? */
    if (gsignond_identity_info_get_store_secret (priv->info)) {
        gboolean remember = TRUE;

        /* and there was no opt-out? */
        if (!gsignond_signonui_data_get_remember_password (reply, &remember))
            DBG ("identity %d - don't remember password",
                 gsignond_identity_info_get_id (priv->info));
        if (remember && ui_error == SIGNONUI_ERROR_NONE) {
                gsignond_identity_info_set_secret (priv->info,
                                                   gsignond_signonui_data_get_password (reply));
        } else if (!remember ||
                   (ui_error == SIGNONUI_ERROR_CANCELED ||
                    ui_error == SIGNONUI_ERROR_FORGOT_PASSWORD)) {
            /* reset to empty in case user canceled or forgot,
             * or doesn't want password to be remebered anymore */
            gsignond_identity_info_set_secret (priv->info, "");
        }
    }
    /* TODO: auto-set to validated on successful process() cycle */
    /* store if not a new identity (new is stored later) */
    if (!gsignond_identity_info_get_is_identity_new (priv->info)) {
        if (!gsignond_daemon_store_identity (priv->owner, cb_data->identity))
            WARN ("failed to update identity %d",
                  gsignond_identity_info_get_id (priv->info));
    }

    if (cb_data->session) {
        gsignond_auth_session_user_action_finished (cb_data->session, reply);
    }

    g_slice_free (GSignondIdentityCbData, cb_data);
}

static void
_on_process_canceled (GSignondAuthSession *session, GSignondIdentityCbData *cb_data)
{
    g_signal_handlers_disconnect_by_func(session, G_CALLBACK(_on_process_canceled), cb_data);

    if (!cb_data) {
        WARN ("assert (cb_data)");
        return;
    }
    if (!gsignond_daemon_cancel_dialog (
            cb_data->identity->priv->owner, G_OBJECT(session), NULL, NULL)) {
        WARN ("Fail to cancel dialog");
    }
    g_slice_free (GSignondIdentityCbData, cb_data);
}

static void
_on_user_action_required (GSignondAuthSession *session, GSignondSignonuiData *ui_data, gpointer userdata)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (userdata);
    GSignondIdentityCbData *cb_data = g_slice_new (GSignondIdentityCbData);

    cb_data->identity = identity;
    cb_data->session = session;

    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT(session), 
            ui_data, _on_user_action_completed, _on_refresh_requested_by_ui, cb_data);

    g_signal_connect (session, "process-canceled", G_CALLBACK(_on_process_canceled), cb_data);
}

static void
_on_store_token (GSignondAuthSession *session, GSignondDictionary *token_data, gpointer userdata)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (userdata);
    guint32 identity_id = GSIGNOND_IDENTITY_INFO_NEW_IDENTITY;

    g_return_if_fail (identity && session && GSIGNOND_IS_AUTH_SESSION (session));

    identity_id = gsignond_identity_info_get_id (identity->priv->info);

    if (identity_id != GSIGNOND_IDENTITY_INFO_NEW_IDENTITY) {
        gsignond_daemon_store_identity_data (identity->priv->owner, identity_id, 
            gsignond_auth_session_get_method (session), token_data);
    }
}

static gboolean
_compare_session_by_pointer (gpointer key, gpointer value, gpointer dead_object)
{
    return value == dead_object;
}

static void
_on_session_dead (gpointer data, GObject *session)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (data);

    DBG ("identity %p session %p disposed", identity, session);
    
    g_hash_table_foreach_remove (identity->priv->auth_sessions,
            _compare_session_by_pointer, session);   
}

GSignondAuthSession *
gsignond_identity_get_auth_session (GSignondIdentity *identity,
                                    const gchar *method,
                                    const GSignondSecurityContext *ctx,
                                    GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondAuthSession *session = NULL;
    GHashTable *supported_methods = NULL;
    gboolean method_available = FALSE;
    guint32 identity_id ;
    GSignondDictionary *token_data = NULL;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, NULL);

    if (!method) {
        WARN ("assertion (method) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_METHOD_NOT_KNOWN,
                      "authentication method not provided");
        return NULL;
    }

    DBG ("get auth session for method '%s'", method);
    session = g_hash_table_lookup (identity->priv->auth_sessions, method);

    if (session && GSIGNOND_IS_AUTH_SESSION (session)) {
        DBG("using cashed auth session '%p' for method '%s'", session, method);
        return GSIGNOND_AUTH_SESSION(g_object_ref (session));
    }

    if (!gsignond_plugin_proxy_factory_get_plugin_mechanisms (gsignond_get_plugin_proxy_factory (),
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

    if ( (identity_id = gsignond_identity_info_get_id (identity->priv->info)) !=
            GSIGNOND_IDENTITY_INFO_NEW_IDENTITY) {
        token_data = gsignond_daemon_load_identity_data (identity->priv->owner, identity_id, method);
    } 
    if (!token_data) token_data = gsignond_dictionary_new();

    session = gsignond_auth_session_new (identity->priv->info, method, token_data);

    if (token_data) gsignond_dictionary_unref (token_data);

    if (!session) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    /* Handle 'ui' signanls on session */
    g_signal_connect (session, "process-user-action-required", G_CALLBACK (_on_user_action_required), identity);
    g_signal_connect (session, "process-store", G_CALLBACK (_on_store_token), identity);

    g_hash_table_insert (identity->priv->auth_sessions, g_strdup (method), session);
    g_object_weak_ref (G_OBJECT (session), _on_session_dead, identity);

    DBG ("session %p creation for method '%s' complete", session, method);

    return session;
}

static void
_on_credentials_updated (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (user_data);
    guint32 id = 0;
    GError *err = NULL;

    if (error) {
        WARN ("failed to verify user : %s", error->message);
        g_error_free (error);

        err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, "Operation canceled");
    }
    else
    {
        GSignondSignonuiError err_id = SIGNONUI_ERROR_NONE;
        gboolean res = gsignond_signonui_data_get_query_error (reply, &err_id);

        if (!res) {
            DBG ("No error code set by UI daemon, treating as ERROR_NONE");
        }

        if (err_id != SIGNONUI_ERROR_NONE) {
            switch (err_id) {
                case SIGNONUI_ERROR_CANCELED:
                    err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED,
                        "Operation canceled");
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
                id = gsignond_daemon_store_identity (identity->priv->owner, identity);
                if (!id) err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_STORE_FAILED, "Failed to store secret");
            }
        }
    }

    g_signal_emit (identity, signals[SIG_CREDENTIALS_UPDATED], 0 , id, err);

    if (err) g_error_free (err);
}

gboolean
gsignond_identity_request_credentials_update (GSignondIdentity *identity,
                                              const gchar *message,
                                              const GSignondSecurityContext *ctx,
                                              GError **error)
{
    GSignondSignonuiData *ui_data = NULL;

    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    if (!identity->priv->info) {
        WARN ("assertion (identity->priv->info) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_ERR, "Identity not found.");
        return FALSE;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    if (!gsignond_identity_info_get_store_secret (identity->priv->info)) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_CREDENTIALS_NOT_AVAILABLE, "Password can not be stored.");
        return FALSE;
    }

    ui_data = gsignond_dictionary_new ();

    gsignond_signonui_data_set_query_password (ui_data, TRUE);
    gsignond_signonui_data_set_username (ui_data, gsignond_identity_info_get_username (identity->priv->info));
    gsignond_signonui_data_set_caption (ui_data, gsignond_identity_info_get_caption (identity->priv->info));
    gsignond_signonui_data_set_message (ui_data, message);
  
    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT(identity),
        ui_data, _on_credentials_updated, NULL, identity);

    gsignond_dictionary_unref (ui_data);

    return TRUE;
}

static void
_on_user_verified (GSignondSignonuiData *reply, GError *error, gpointer user_data)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (user_data);
    gboolean res = FALSE;
    GError *err = NULL;

    if (error) {
        WARN ("failed to verify user : %s", error->message);
        g_error_free (error);

        err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED, "Operation canceled");
    }
    else
    {
        GSignondSignonuiError err_id = SIGNONUI_ERROR_NONE;
        gboolean res = gsignond_signonui_data_get_query_error (reply, &err_id);
        if (!res) {
            DBG ("No error code set by UI daemon, treating as ERROR_NONE");
        }
        if (err_id != SIGNONUI_ERROR_NONE) {
            switch (err_id) {
                case SIGNONUI_ERROR_CANCELED:
                    err = gsignond_get_gerror_for_id (GSIGNOND_ERROR_IDENTITY_OPERATION_CANCELED,
                        "Operation canceled");
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
    }

    g_signal_emit (identity, signals[SIG_USER_VERIFIED], 0, res, err);

    if (err) g_error_free (err);
}

gboolean 
gsignond_identity_verify_user (GSignondIdentity *identity,
                               GVariant *params,
                               const GSignondSecurityContext *ctx,
                               GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
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

    ui_data = gsignond_dictionary_new_from_variant (params);
    gsignond_signonui_data_set_query_password (ui_data, TRUE);
    gsignond_signonui_data_set_username (ui_data, gsignond_identity_info_get_username (identity->priv->info));
    gsignond_signonui_data_set_caption (ui_data, gsignond_identity_info_get_caption (identity->priv->info));

    gsignond_daemon_show_dialog (GSIGNOND_DAEMON (identity->priv->owner), G_OBJECT (identity),
        ui_data, _on_user_verified, NULL, identity);

    gsignond_dictionary_unref (ui_data);

    return TRUE;
}

gboolean
gsignond_identity_verify_secret (GSignondIdentity *identity,
                                 const gchar *secret,
                                 const GSignondSecurityContext *ctx,
                                 GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Not supported");

    return FALSE;
}

gboolean
gsignond_identity_sign_out (GSignondIdentity *identity,
                            const GSignondSecurityContext *ctx,
                            GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    gboolean success = FALSE;
    guint32 identity_id = GSIGNOND_IDENTITY_INFO_NEW_IDENTITY;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, FALSE);

    identity_id = gsignond_identity_info_get_id (identity->priv->info);

    if (identity_id == GSIGNOND_IDENTITY_INFO_NEW_IDENTITY) {
        /* TODO; clear the cached secret for unstored identity */
        success = TRUE;
    }
    else {
        success = gsignond_daemon_clear_identity_data (identity->priv->owner, identity_id);
    }
    if(!success) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Failed to clear data");
        return FALSE;
    }

    g_signal_emit (identity, signals[SIG_INFO_UPDATED], 0, GSIGNOND_IDENTITY_SIGNED_OUT, NULL);

    return success;
}

typedef struct _{
    GSignondDaemon *daemon;
    guint32 identity_id;
}_StoreCachedTokenCbInfo;
static void
_store_cached_token_data (const gchar *method, GSignondAuthSession *session, _StoreCachedTokenCbInfo *data)
{
    if (!data || !method || !session) return ;

    GSignondDictionary *token_data = gsignond_auth_session_get_token_data (session);

    if (token_data)
        gsignond_daemon_store_identity_data (data->daemon, data->identity_id, method, token_data);
}

guint32
gsignond_identity_store (GSignondIdentity *identity, 
                         const GVariant *info,
                         const GSignondSecurityContext *ctx,
                         GError **error)
{
    GSignondIdentityPrivate *priv = NULL;
    GSignondIdentityInfo *identity_info = NULL;
    gboolean was_new_identity = FALSE;
    GSignondSecurityContextList *contexts = NULL;
    GSignondIdentityInfoPropFlags flags;
    guint32 id;

    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }

    priv = identity->priv;

    VALIDATE_IDENTITY_WRITE_ACCESS (identity, ctx, 0);

    was_new_identity = gsignond_identity_info_get_is_identity_new (priv->info);

    identity_info = gsignond_identity_info_new_from_variant ((GVariant *)info);

    contexts = gsignond_identity_info_get_access_control_list (identity_info);
    if (contexts) {
        VALIDATE_IDENTITY_WRITE_ACL (identity, ctx, 0);
        gsignond_security_context_list_free (contexts);
    }
   
    flags = gsignond_identity_info_get_edit_flags (identity_info);

    if (flags & IDENTITY_INFO_PROP_USERNAME)
        gsignond_identity_info_set_username (priv->info,
            gsignond_identity_info_get_username(identity_info));
    if (flags & IDENTITY_INFO_PROP_USERNAME_IS_SECRET)
        gsignond_identity_info_set_username_secret (priv->info,
            gsignond_identity_info_get_is_username_secret (identity_info));
    if (flags & IDENTITY_INFO_PROP_SECRET)
        gsignond_identity_info_set_secret (priv->info,
            gsignond_identity_info_get_secret (identity_info));
    if (flags & IDENTITY_INFO_PROP_STORE_SECRET)
        gsignond_identity_info_set_store_secret (priv->info,
            gsignond_identity_info_get_store_secret (identity_info));
    if (flags & IDENTITY_INFO_PROP_CAPTION)
        gsignond_identity_info_set_caption (priv->info,
            gsignond_identity_info_get_caption (identity_info));
    if (flags & IDENTITY_INFO_PROP_TYPE)
        gsignond_identity_info_set_identity_type (priv->info,
            gsignond_identity_info_get_identity_type (identity_info));
    if (flags & IDENTITY_INFO_PROP_METHODS) {
        GHashTable *methods = 
            gsignond_identity_info_get_methods (identity_info);
        gsignond_identity_info_set_methods (priv->info, methods);
        g_hash_table_unref (methods);
    }
    if (flags & IDENTITY_INFO_PROP_REALMS) {
        GSequence *realms = 
            gsignond_identity_info_get_realms (identity_info);
        gsignond_identity_info_set_realms (priv->info, realms);
        g_sequence_free (realms);
    }

    /* FIXME : either username/secret changed reset the identity
     * valdated state to FALSE ???
     */          

    gsignond_identity_info_unref (identity_info);

    /* Ask daemon to store identity info to db */
    id = gsignond_daemon_store_identity (priv->owner, identity);
    if (!id) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_STORE_FAILED,
                                                        "Failed to store identity");
        /*FIXME: Roll-back the local changes */
    }
    else {
        if (was_new_identity) {
            _set_id (identity, id);
            _StoreCachedTokenCbInfo data = { priv->owner, id };
            /* store any cached token data if available at auth sessions */
            g_hash_table_foreach (priv->auth_sessions, (GHFunc)_store_cached_token_data, (gpointer)&data);
        }

        g_signal_emit (identity, signals[SIG_INFO_UPDATED], 0, GSIGNOND_IDENTITY_DATA_UPDATED);
    }
 
    return id;
}

gboolean
gsignond_identity_remove (GSignondIdentity *identity, 
                          const GSignondSecurityContext *ctx,
                          GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    gboolean is_removed = FALSE;
    
    VALIDATE_IDENTITY_WRITE_ACCESS (identity, ctx, FALSE);

    is_removed = gsignond_identity_clear (identity);

    if (!is_removed && error)
        *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_REMOVE_FAILED, "failed to remove identity");

    return is_removed;
}


gboolean
gsignond_identity_clear (GSignondIdentity *identity)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        return FALSE;
    }
    gboolean is_removed = FALSE;

    if (gsignond_identity_info_get_is_identity_new (identity->priv->info))
        is_removed = TRUE;
    else
        is_removed = gsignond_daemon_remove_identity (
                       identity->priv->owner, 
                       gsignond_identity_info_get_id (identity->priv->info));

    if (is_removed)
        g_signal_emit (identity, signals[SIG_INFO_UPDATED], 0, GSIGNOND_IDENTITY_REMOVED);
    else
        WARN ("request to remove identity %u failed",
              gsignond_identity_info_get_id (identity->priv->info));

    return is_removed;
}


guint32
gsignond_identity_add_reference (GSignondIdentity *identity,
                                 const gchar *reference,
                                 const GSignondSecurityContext *ctx,
                                 GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }
    guint32 res = 0;
    guint32 identity_id = 0;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, 0);

    identity_id = gsignond_identity_info_get_id (identity->priv->info);
    if (!identity_id) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_STORE_FAILED, "Cannot add reference to unsaved identity");
        return 0;
    }
    res = gsignond_daemon_add_identity_reference (identity->priv->owner, identity_id, ctx, reference);

    if (res == 0) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
    }

    return res;
}

guint32
gsignond_identity_remove_reference (GSignondIdentity *identity,
                                    const gchar *reference,
                                    const GSignondSecurityContext *ctx,
                                    GError **error)
{
    if (!(identity && GSIGNOND_IS_IDENTITY (identity))) {
        WARN ("assertion (identity && GSIGNOND_IS_IDENTITY(identity)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return 0;
    }

    gboolean res = 0;
    guint32 identity_id = 0;

    VALIDATE_IDENTITY_READ_ACCESS (identity, ctx, 0);

    identity_id = gsignond_identity_info_get_id (identity->priv->info);
    if (!identity_id) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_REFERENCE_NOT_FOUND, "reference not '%s' found", reference);
        return 0;
    }

    res = gsignond_daemon_remove_identity_reference (identity->priv->owner, identity_id, ctx, reference);
    if (res == FALSE) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_REFERENCE_NOT_FOUND,
                                                        "reference '%s' not found", reference);
    }
    return identity_id;
}

GSignondAccessControlManager *
gsignond_identity_get_acm (GSignondIdentity *identity)
{
    g_return_val_if_fail (identity && GSIGNOND_IS_IDENTITY(identity), NULL);

    return gsignond_daemon_get_access_control_manager (identity->priv->owner);
}

guint
gsignond_identity_get_auth_session_timeout (GSignondIdentity *identity)
{
    g_return_val_if_fail (identity && GSIGNOND_IS_IDENTITY(identity), 0);

    return gsignond_daemon_get_auth_session_timeout (identity->priv->owner);
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
 * gsignond_identity_new:
 * @owner: Owner of this object, instance of #GSignondAuthServiceIface
 * @info (transfer full): Identity info, instance of #GSignondIdentityInfo
 * 
 * Creates new instance of #GSignondIdentity
 *
 * Returns[transfer full]: new instance of #GSignondIdentity
 */
GSignondIdentity * gsignond_identity_new (GSignondDaemon *owner,
                                          GSignondIdentityInfo *info)
{
    GSignondIdentity *identity =
        GSIGNOND_IDENTITY(g_object_new (GSIGNOND_TYPE_IDENTITY,
                                        "info", info,
                                        NULL));

    identity->priv->owner = g_object_ref (owner);

    return identity;
}
