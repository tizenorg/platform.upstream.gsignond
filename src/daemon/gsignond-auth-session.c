/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2013 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "gsignond-auth-session.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-session-data.h"
#include "gsignond/gsignond-identity-info.h"
#include "plugins/gsignond-plugin-proxy-factory.h"
#include "gsignond-daemon.h"

enum
{
    PROP_0,
    PROP_METHOD,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

enum {
    SIG_PROCESS_STORE,
    SIG_PROCESS_USER_ACTION_REQUIRED,
    SIG_PROCESS_REFRESHED,
 
    SIG_MAX
};

static guint signals[SIG_MAX] = { 0 };

typedef struct {
    GSignondAuthSession *self;
    ProcessReadyCb ready_cb;
    StateChangeCb state_change_cb;
    gpointer userdata;
} _ProcessData;

struct _GSignondAuthSessionPrivate
{
    gchar *method;
    GSignondPluginProxy *proxy;
    GSequence *available_mechanisms;
    GSignondIdentityInfo *identity_info;
};

G_DEFINE_TYPE (GSignondAuthSession, gsignond_auth_session, G_TYPE_OBJECT)

#define GSIGNOND_AUTH_SESSION_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_AUTH_SESSION, \
                                 GSignondAuthSessionPrivate)

#define VALIDATE_READ_ACCESS(info, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_get_access_control_manager(); \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (info); \
    GSignondSecurityContext *owner = gsignond_identity_info_get_owner (info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, owner, acl); \
    gsignond_security_context_free (owner); \
    gsignond_security_context_list_free (acl); \
    if (!valid) { \
        WARN ("security check failed"); \
        if (error) { \
            *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_PERMISSION_DENIED, "Can not access identity"); \
        } \
        return ret; \
    } \
}

static gint
_sort_cmp (gconstpointer str1, gconstpointer str2, gpointer user_data)
{
    (void) user_data;

    return g_strcmp0 ((const gchar *) str1, (const gchar *) str2);
}

static void
_create_mechanism_cache (GSignondAuthSession *self)
{
    GSignondAuthSessionPrivate *priv = self->priv;

    if (priv->available_mechanisms)
        return;

    gchar **mechanisms, **iter;
    GSequence *allowed_mechanisms = NULL;
    GSequenceIter *wcard = NULL;

    self->priv->available_mechanisms = g_sequence_new (g_free);
    if (!gsignond_identity_info_get_is_identity_new (priv->identity_info)) {
        allowed_mechanisms = gsignond_identity_info_get_mechanisms (
                                                           priv->identity_info,
                                                           priv->method);
        if (!allowed_mechanisms)
            return;
        wcard = g_sequence_lookup (allowed_mechanisms,
                                   (gpointer) "*",
                                   _sort_cmp, NULL);
    }

    g_object_get (self->priv->proxy, 
                  "mechanisms", &mechanisms,
                  NULL);
    if (!mechanisms) {
        if (allowed_mechanisms)
            g_sequence_free (allowed_mechanisms);
        return;
    }
    if (wcard || !allowed_mechanisms) {
        DBG ("add all mechanisms to allowed");
        for (iter = mechanisms; *iter != NULL; iter++) {
            g_sequence_insert_sorted (priv->available_mechanisms,
                                      (gpointer) *iter,
                                      _sort_cmp,
                                      NULL);
            DBG ("  allow '%s'", *iter);
        }
    } else {
        DBG ("allow intersection of plugin and ACL mechanisms");
        for (iter = mechanisms; *iter != NULL; iter++) {
            GSequenceIter *pos = g_sequence_lookup (allowed_mechanisms,
                                                    (gpointer) *iter,
                                                    _sort_cmp,
                                                    NULL);
            DBG ("  allow: '%s'", *iter);
            if (pos)
                g_sequence_insert_sorted (priv->available_mechanisms,
                                          (gpointer) *iter,
                                          _sort_cmp,
                                          NULL);
        }
    }
    if (allowed_mechanisms)
        g_sequence_free (allowed_mechanisms);
    g_free (mechanisms);
}

gchar **
gsignond_auth_session_query_available_mechanisms (GSignondAuthSession *self,
                                                  const gchar **wanted_mechanisms,
                                                  const GSignondSecurityContext *ctx,
                                                  GError **error)
{
    if (!self || !GSIGNOND_IS_AUTH_SESSION (self)) {
        WARN ("assertion (iself && GSIGNOND_IS_AUTH_SESSION (self)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }

    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, NULL);

    gchar **mechanisms, **iter;
    const gchar **src_iter;

    _create_mechanism_cache (self);
    mechanisms = (gchar **)
        g_malloc0 ((g_sequence_get_length (self->priv->available_mechanisms) +
                   1) * sizeof(gchar *));
    iter = mechanisms;
    for (src_iter = wanted_mechanisms; *src_iter != NULL; src_iter++) {
        GSequenceIter *pos = g_sequence_lookup (
                                              self->priv->available_mechanisms,
                                              (gpointer) *src_iter,
                                              _sort_cmp,
                                              NULL);
        if (pos) {
            *iter = g_sequence_get (pos);
            iter++;
        }
    }
    *iter = NULL;

    return mechanisms;
}

gboolean
gsignond_auth_session_process (GSignondAuthSession *self,
                               GSignondSessionData *session_data,
                               const gchar *mechanism,
                               const GSignondSecurityContext *ctx,
                               ProcessReadyCb ready_cb,
                               StateChangeCb state_change_cb,
                               gpointer userdata,
                               GError **error)
{
    if (!self || !GSIGNOND_IS_AUTH_SESSION (self)) {
        WARN ("assertion (seöf && GSIGNOND_IS_AUTH_SESSION (self))failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }

    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, FALSE);

    _create_mechanism_cache (self);
    if (!g_sequence_lookup (self->priv->available_mechanisms,
                            (gpointer) mechanism,
                            _sort_cmp,
                            NULL)) {
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_MECHANISM_NOT_AVAILABLE, "Mechanism is not available");
        return FALSE;
    }

    if (session_data && 
        !gsignond_session_data_get_username (session_data) 
        && self->priv->identity_info) {
        const gchar *username = gsignond_identity_info_get_username (self->priv->identity_info);

        if (username) {
            gsignond_session_data_set_username (session_data, username);
        }
    }

    _ProcessData * data = g_slice_new0 (_ProcessData);
    data->self = self;
    data->ready_cb = ready_cb;
    data->state_change_cb = state_change_cb;
    data->userdata = userdata;
    gsignond_plugin_proxy_process(self->priv->proxy, self, session_data,
                                  mechanism, data);

    return TRUE;
}

gboolean
gsignond_auth_session_cancel (GSignondAuthSession *self,
                              const GSignondSecurityContext *ctx,
                              GError **error)
{
    if (!self || !GSIGNOND_IS_AUTH_SESSION (self)) {
        WARN ("assertion (self && GSIGNOND_IS_AUTH_SESSION (self)) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, FALSE);

    gsignond_plugin_proxy_cancel(self->priv->proxy, self);

    return TRUE;
}

void
gsignond_auth_session_abort_process (GSignondAuthSession *self)
{
    g_return_if_fail (self && GSIGNOND_IS_AUTH_SESSION (self));

    gsignond_plugin_proxy_cancel (self->priv->proxy, self);
}

void 
gsignond_auth_session_user_action_finished (GSignondAuthSession *self,
                                            GSignondSignonuiData *ui_data)
{
    gsignond_plugin_proxy_user_action_finished(self->priv->proxy, ui_data);
}

void
gsignond_auth_session_refresh (GSignondAuthSession *self, 
                               GSignondSignonuiData *ui_data)
{
    gsignond_plugin_proxy_refresh(self->priv->proxy, ui_data);
}

GSignondAccessControlManager *
gsignond_auth_session_get_acm (GSignondAuthSession *session)
{
    return gsignond_get_access_control_manager ();
}

static void
_get_property (GObject *object, guint property_id, GValue *value,
               GParamSpec *pspec)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (object);

    switch (property_id)
    {
        case PROP_METHOD:
            g_value_set_string (value, self->priv->method);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_set_property (GObject *object, guint property_id, const GValue *value,
               GParamSpec *pspec)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (object);

    switch (property_id)
    {
        case PROP_METHOD:
            self->priv->method = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (object);

    if (self->priv->proxy) {
        g_object_unref (self->priv->proxy);
        self->priv->proxy = NULL;
    }

    if (self->priv->identity_info) {
        g_hash_table_unref ((GHashTable *)self->priv->identity_info);
        self->priv->identity_info = NULL;
    }

    G_OBJECT_CLASS (gsignond_auth_session_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (object);

    if (self->priv->method) {
        g_free (self->priv->method);
        self->priv->method = NULL;
    }

    if (self->priv->available_mechanisms) {
        g_sequence_free (self->priv->available_mechanisms);
        self->priv->available_mechanisms = NULL;
    }

    G_OBJECT_CLASS (gsignond_auth_session_parent_class)->finalize (object);
}

static void
gsignond_auth_session_init (GSignondAuthSession *self)
{
    self->priv = GSIGNOND_AUTH_SESSION_PRIV (self);

    self->priv->method = NULL;
    self->priv->proxy = NULL;
    self->priv->identity_info = NULL;
    self->priv->available_mechanisms = NULL;
}

static void
gsignond_auth_session_class_init (GSignondAuthSessionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (GSignondAuthSessionPrivate));

    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    object_class->dispose = _dispose;
    object_class->finalize = _finalize;

    properties[PROP_METHOD] =
        g_param_spec_string ("method",
                             "authentication method",
                             "Authentication method used",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                             | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);

    signals[SIG_PROCESS_STORE] =  g_signal_new ("process-store",
            GSIGNOND_TYPE_AUTH_SESSION,
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_DICTIONARY);

    signals[SIG_PROCESS_USER_ACTION_REQUIRED] =  g_signal_new ("process-user-action-required",
            GSIGNOND_TYPE_AUTH_SESSION,
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_SIGNONUI_DATA);

    signals[SIG_PROCESS_REFRESHED] =  g_signal_new ("process-refreshed",
            GSIGNOND_TYPE_AUTH_SESSION,
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_SIGNONUI_DATA);
}

/**
 * gsignond_auth_session_get_method:
 * @session: instance of #GSignondAuthSession
 *
 * Retrieves authentication method used by #session.
 *
 * Returns: (transfer none) authenticaiton method if success, NULL otherwise
 */
const gchar *
gsignond_auth_session_get_method (GSignondAuthSession *session)
{
    g_return_val_if_fail (session && GSIGNOND_IS_AUTH_SESSION (session), NULL);

    return session->priv->method;
}

void
gsignond_auth_session_notify_process_result (GSignondAuthSession *iface,
                                             GSignondSessionData *result,
                                             gpointer userdata)
{
    if (!userdata) {
        WARN("assert (userdata)");
        return ;
    }
    _ProcessData *data = (_ProcessData *)userdata;

    if (data->ready_cb) data->ready_cb (result, NULL, data->userdata);

    g_slice_free (_ProcessData, data);
}

void
gsignond_auth_session_notify_process_error (GSignondAuthSession *iface,
                                            const GError *error,
                                            gpointer userdata)
{
    if (!userdata) {
        WARN("assert (userdata)");
        return ;
    }
    _ProcessData *data = (_ProcessData *)userdata;

    if (data->ready_cb) data->ready_cb (NULL, error, data->userdata);

    g_slice_free (_ProcessData, data);
}

void 
gsignond_auth_session_notify_state_changed (GSignondAuthSession *self,
                                            gint state,
                                            const gchar *message,
                                            gpointer userdata)
{
    if (!userdata) {
        WARN("assert (userdata)");
        return ;
    }
    _ProcessData *data = (_ProcessData *)userdata;

    if (data->state_change_cb) data->state_change_cb (state, message, data->userdata);
}

void 
gsignond_auth_session_notify_store (GSignondAuthSession *self, 
                                    GSignondDictionary *token_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_STORE], 0, token_data);
}

void 
gsignond_auth_session_notify_user_action_required (GSignondAuthSession *self, 
                                                   GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_USER_ACTION_REQUIRED], 0, ui_data);
}

void 
gsignond_auth_session_notify_refreshed (GSignondAuthSession *self, 
                                        GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_REFRESHED], 0, ui_data);
}


/**
 * gsignond_auth_session_new:
 * @info: instance of #GSignondIdentityInfo
 * @method: authentication method
 *
 * Creates instance of #GSignondAuthSession.
 *
 * Returns: (transfer full) newly created object 
 */
GSignondAuthSession * 
gsignond_auth_session_new (GSignondIdentityInfo *info, const gchar *method)
{
    GSignondPluginProxy* proxy = NULL;

    g_return_val_if_fail (method, NULL);

    proxy = gsignond_plugin_proxy_factory_get_plugin(
    gsignond_get_plugin_proxy_factory(), method);
    if (!proxy) return NULL;

    GSignondAuthSession *auth_session =
        g_object_new (GSIGNOND_TYPE_AUTH_SESSION,
                      "method", method, NULL);
    auth_session->priv->proxy = proxy;
    auth_session->priv->identity_info = g_hash_table_ref ((GHashTable *)info);

    return auth_session;
}
