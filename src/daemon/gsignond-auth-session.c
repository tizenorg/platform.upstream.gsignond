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

#include "gsignond-auth-session-iface.h"
#include "gsignond/gsignond-error.h"
#include "gsignond/gsignond-log.h"
#include "gsignond/gsignond-session-data.h"
#include "gsignond/gsignond-identity-info.h"
#include "dbus/gsignond-dbus.h"
#include "dbus/gsignond-dbus-auth-session-adapter.h"
#include "gsignond-auth-session.h"
#include "plugins/gsignond-plugin-proxy-factory.h"
#include "gsignond-daemon.h"

enum
{
	PROP_0,
	PROP_METHOD,
    PROP_APP_CONTEXT,
	N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondAuthSessionPrivate
{
    gchar *method;
    GSignondDbusAuthSessionAdapter *session_adapter;
    GSignondPluginProxy *proxy;
    GSequence *plugin_mechanisms;
    GSignondIdentityInfo *identity_info;
};

static void gsignond_auth_session_iface_init (gpointer g_iface);

G_DEFINE_TYPE_WITH_CODE (GSignondAuthSession, gsignond_auth_session,
                        GSIGNOND_TYPE_DISPOSABLE,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_AUTH_SESSION_IFACE,
                                               gsignond_auth_session_iface_init));

#define GSIGNOND_AUTH_SESSION_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_AUTH_SESSION, \
                                 GSignondAuthSessionPrivate)

#define VALIDATE_READ_ACCESS(info, ctx, ret) \
{ \
    GSignondAccessControlManager *acm = gsignond_get_access_control_manager(); \
    GSignondSecurityContextList *acl = gsignond_identity_info_get_access_control_list (info); \
    gboolean valid = gsignond_access_control_manager_peer_is_allowed_to_use_identity (acm, ctx, acl); \
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

static gchar **
_query_available_mechanisms (GSignondAuthSessionIface *iface,
                             const gchar **wanted_mechanisms,
                             const GSignondSecurityContext *ctx,
                             GError **error)
{
    if (!iface || !GSIGNOND_IS_AUTH_SESSION (iface)) {
        WARN ("assertion G_LIKELY ((iface && GSIGNOND_IS_AUTH_SESSION (iface)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return NULL;
    }
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, NULL);

    gchar **mechanisms, **iter;
    const gchar **src_iter;

    if (!self->priv->plugin_mechanisms) {
        g_object_get (self->priv->proxy, 
                      "mechanisms", &mechanisms,
                      NULL);
        self->priv->plugin_mechanisms = g_sequence_new (g_free);
        for (iter = mechanisms; *iter != NULL; iter++)
            g_sequence_insert_sorted (self->priv->plugin_mechanisms,
                                      *iter,
                                      _sort_cmp,
                                      NULL);
        g_free (mechanisms);
    }
    mechanisms = (gchar **)
        g_malloc0 ((g_sequence_get_length (self->priv->plugin_mechanisms) + 1) *
                   sizeof(gchar *));
    iter = mechanisms;
    for (src_iter = wanted_mechanisms; *src_iter != NULL; src_iter++) {
        GSequenceIter *pos = g_sequence_lookup (self->priv->plugin_mechanisms,
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

static gboolean
_process (GSignondAuthSessionIface *iface, 
          GSignondSessionData *session_data,
          const gchar *mechanism,
          const GSignondSecurityContext *ctx,
          GError **error)
{
    if (G_LIKELY ((iface && GSIGNOND_IS_AUTH_SESSION (iface)) == 0)) {
        WARN ("assertion G_LIKELY ((iface && GSIGNOND_IS_AUTH_SESSION (iface)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, FALSE);

    if (session_data && 
        !gsignond_session_data_get_username (session_data) 
        && self->priv->identity_info) {
        const gchar *username = gsignond_identity_info_get_username (self->priv->identity_info);

        if (username) {
            gsignond_session_data_set_username (session_data, username);
        }
    }

    gsignond_plugin_proxy_process(self->priv->proxy, iface, session_data,
                                  mechanism);

    return TRUE;
}

static gboolean
_cancel (GSignondAuthSessionIface *iface,
         const GSignondSecurityContext *ctx,
         GError **error)
{
    if (G_LIKELY ((iface && GSIGNOND_IS_AUTH_SESSION (iface)) == 0)) {
        WARN ("assertion G_LIKELY ((iface && GSIGNOND_IS_AUTH_SESSION (iface)) == 0) failed");
        if (error) *error = gsignond_get_gerror_for_id (GSIGNOND_ERROR_UNKNOWN, "Unknown error");
        return FALSE;
    }
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    VALIDATE_READ_ACCESS (self->priv->identity_info, ctx, FALSE);

    gsignond_plugin_proxy_cancel(self->priv->proxy, iface);

    return TRUE;
}

void 
_user_action_finished (GSignondAuthSessionIface *iface, 
                       GSignondSessionData *session_data)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    gsignond_plugin_proxy_user_action_finished(self->priv->proxy, session_data);
}

void 
_refresh (GSignondAuthSessionIface *iface, 
                              GSignondSessionData *session_data)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    gsignond_plugin_proxy_refresh(self->priv->proxy, session_data);
}

GSignondAccessControlManager *
_get_acm (GSignondAuthSessionIface *iface)
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
        case PROP_APP_CONTEXT:
            g_object_get_property (G_OBJECT (self->priv->session_adapter), "app-context", value);
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
        case PROP_APP_CONTEXT:
            g_object_set_property (G_OBJECT (self->priv->session_adapter), "app-context", value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_dispose (GObject *object)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (object);

    if (self->priv->session_adapter) {
        g_object_unref (self->priv->session_adapter);
        self->priv->session_adapter = NULL;
    }

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

    if (self->priv->plugin_mechanisms) {
        g_sequence_free (self->priv->plugin_mechanisms);
        self->priv->plugin_mechanisms = NULL;
    }

    G_OBJECT_CLASS (gsignond_auth_session_parent_class)->finalize (object);
}

static void
gsignond_auth_session_init (GSignondAuthSession *self)
{
    self->priv = GSIGNOND_AUTH_SESSION_PRIV (self);

    self->priv->session_adapter =
        gsignond_dbus_auth_session_adapter_new (
                                             GSIGNOND_AUTH_SESSION_IFACE(self));
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
                              | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);

    properties[PROP_APP_CONTEXT] =
        g_param_spec_string ("app-context",
                             "application security context",
                             "Application security context",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                              | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_auth_session_iface_init (gpointer g_iface)
{
    GSignondAuthSessionIfaceInterface *auth_session_iface =
        (GSignondAuthSessionIfaceInterface *) g_iface;

    auth_session_iface->process = _process;
    auth_session_iface->query_available_mechanisms = _query_available_mechanisms;
    auth_session_iface->cancel = _cancel;
    auth_session_iface->user_action_finished = _user_action_finished;
    auth_session_iface->refresh = _refresh;
    auth_session_iface->get_acm = _get_acm;
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

/**
 * gsignond_auth_session_get_object_path:
 * @session: instance of #GSignondAuthSession
 *
 * Retrieves dbus object path used by #session object.
 *
 * Returns: (transfer none) dbus object path if success, NULL otherwise
 */
const gchar *
gsignond_auth_session_get_object_path (GSignondAuthSession *session)
{
    g_return_val_if_fail (session && GSIGNOND_IS_AUTH_SESSION (session), NULL);
    
    return gsignond_dbus_auth_session_adapter_get_object_path (
        session->priv->session_adapter);
}

gboolean gsignond_auth_session_set_id(GSignondAuthSession *session, gint id)
{
    return gsignond_plugin_proxy_factory_add_plugin(
        gsignond_get_plugin_proxy_factory(), id, session->priv->proxy);
}

/**
 * gsignond_auth_session_new:
 * @info: instance of #GSignondIdentityInfo
 * @app_context: application security
 * @method: authentication method
 *
 * Creates instance of #GSignondAuthSession.
 *
 * Returns: (transfer full) newly created object 
 */
GSignondAuthSession * 
gsignond_auth_session_new (GSignondIdentityInfo *info, const gchar *app_context, const gchar *method, gint timeout)
{
    GSignondPluginProxy* proxy;
    guint id = 0;

    if (info) id = gsignond_identity_info_get_id (info);
    
    if (id == 0) {
        proxy = gsignond_plugin_proxy_new(gsignond_get_config(), method);
        if (!proxy) return NULL;
    } else {
        proxy = gsignond_plugin_proxy_factory_get_plugin(
            gsignond_get_plugin_proxy_factory(), id, method);
        if (!proxy) return NULL;
        g_object_ref(proxy);
    }

    GSignondAuthSession *auth_session =
        g_object_new (GSIGNOND_TYPE_AUTH_SESSION,
                      "method", method,
                      "app-context", app_context,
                      "timeout", timeout, 
                      NULL);
    auth_session->priv->proxy = proxy;
    auth_session->priv->identity_info = g_hash_table_ref ((GHashTable *)info);

    return auth_session;
}
