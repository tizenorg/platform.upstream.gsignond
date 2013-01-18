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

#include "gsignond/gsignond-log.h"

#include "gsignond-identity-iface.h"
#include "dbus/gsignond-dbus.h"
#include "dbus/gsignond-dbus-identity-adapter.h"
#include "gsignond-identity.h"
#include "gsignond-auth-session.h"

enum 
{
    PROP_0,
    PROP_INFO,
    PROP_APP_CONTEXT,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondIdentityPrivate
{
    GSignondIdentityInfo *info;
    gchar *app_context;
    GSignondAuthServiceIface *owner;
    GSignondDbusIdentityAdapter *identity_adapter;
    GHashTable *auth_sessions; /* "object_path":auth_session_object_path */
};

static void
gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondIdentity, gsignond_identity, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_IDENTITY_IFACE, 
                                               gsignond_identity_iface_init));


#define GSIGNOND_IDENTITY_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentityPrivate)

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
            g_value_set_string (value, self->priv->app_context);
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
                GSIGNOND_IDENTITY_INFO (g_value_get_boxed (value));
            break;
        case PROP_APP_CONTEXT:
            self->priv->app_context = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
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
        g_object_unref (self->priv->owner);
        self->priv->owner = NULL;
    }

    if (self->priv->info) {
        gsignond_identity_info_free (self->priv->info);
        self->priv->info = NULL;
    }
   
    G_OBJECT_CLASS (gsignond_identity_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY (object);

    if (self->priv->app_context) {
        g_free (self->priv->app_context);
        self->priv->app_context = NULL;
    }
}

static void
gsignond_identity_init (GSignondIdentity *self)
{
    GError *err = NULL;
    self->priv = GSIGNOND_IDENTITY_PRIV(self);

    self->priv->identity_adapter =
        gsignond_dbus_identity_adapter_new (GSIGNOND_IDENTITY_IFACE (self));
    self->priv->auth_sessions = 
        g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);

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

    properties[PROP_APP_CONTEXT] =
       g_param_spec_string ("app-context",
                            "application security context",
                            "Application security context of the identity object creater",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class, N_PROPERTIES, properties);

    g_signal_new ("remove",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_BOOLEAN,
                  1,
                  GSIGNOND_TYPE_IDENTITY_INFO);
    g_signal_new ("store",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_UINT,
                  1,
                  GSIGNOND_TYPE_IDENTITY_INFO);
    g_signal_new ("add-reference",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_INT,
                  2,
                  GSIGNOND_TYPE_IDENTITY_INFO,
                  G_TYPE_STRING);
    g_signal_new ("remove-reference",
                  GSIGNOND_TYPE_IDENTITY,
                  G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_INT,
                  2,
                  GSIGNOND_TYPE_IDENTITY_INFO,
                  G_TYPE_STRING);
}

static guint32
_request_credentials_update (GSignondIdentityIface *self, const gchar *message) 
{
    guint32 id = 0;

    g_signal_emit_by_name (self,
                           "request-credentials-update",
                           message, &id);
    return id;
}

static GVariant * 
_get_info (GSignondIdentityIface *self)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (self);
    GVariant *info = NULL;

    info = gsignond_dictionary_to_variant (identity->priv->info);

    return info;
}

static void
_on_session_close (gpointer data, GObject *session)
{
    GSignondIdentity *identity = GSIGNOND_IDENTITY (data);

    g_object_weak_unref (session, _on_session_close, data);

    g_hash_table_remove (identity->priv->auth_sessions, 
                         (gpointer)gsignond_auth_session_get_object_path (
                            GSIGNOND_AUTH_SESSION (session)));
}

static const gchar *
_get_auth_session (GSignondIdentityIface *self, const gchar *method)
{
    GSignondAuthSession *session = NULL;
    const gchar *object_path = NULL;

    g_value_return_if_fail (self, NULL);
    g_value_return_if_fail (method, NULL);

    /*
     * FIXME: check if given method is supported
     */
    
    session = gsignond_auth_session_new (self, method);

    if (!session)
        return NULL;

    object_path = gsignond_auth_session_get_object_path (session);

    g_hash_table_insert (GSIGNOND_IDENTITY(self)->priv->auth_sessions, 
                         (gpointer)object_path, 
                         (gpointer)session);

    return object_path;
}

static gboolean 
_verify_user (GSignondIdentityIface *self, const GVariant *params)
{
    gboolean success = FALSE;

    g_signal_emit_by_name (self,
                           "verify-user",
                           params, &success);
    return success;
}

static gboolean
_verify_secret (GSignondIdentityIface *self, const gchar *secret)
{
    gboolean success = FALSE;

    g_signal_emit_by_name (self,
                           "verify-secret",
                           secret, &success);
    return success;
}

static gboolean 
_sign_out (GSignondIdentityIface *self)
{
    (void) self;

    return FALSE;
}

static guint32
_store (GSignondIdentityIface *iface, const GVariant *info)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(iface);
    GSignondIdentityInfo *identity_info = NULL;
    guint32 id;
    gboolean was_new_identity = FALSE;

    identity_info = gsignond_dictionary_new_from_variant ((GVariant *)info);
    /* dont trust 'identity id' passed via 'info' */
    id = gsignond_identity_info_get_id (self->priv->info);
    gsignond_identity_info_set_id (identity_info, id);

    was_new_identity = gsignond_identity_info_get_is_identity_new (identity_info);

    /* update local cache */
    if (self->priv->info) gsignond_identity_info_free (self->priv->info);
    self->priv->info = identity_info;

    g_signal_emit_by_name (self,
                           "store",
                           identity_info, 
                           &id);

    if (was_new_identity) 
        gsignond_identity_set_id (GSIGNOND_IDENTITY(self), id);
    
    return id;
}

static void
_remove (GSignondIdentityIface *iface)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(iface);

    g_signal_emit_by_name (self,
                           "remove",
                           self->priv->info,
                           NULL);
}

static gint32
_add_reference (GSignondIdentityIface *iface, const gchar *reference)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(iface);
    gint32 res;

    g_signal_emit_by_name (self,
                           "add-reference",
                           self->priv->info,
                           reference,
                           &res);
    return res;
}

static gint32
_remove_reference (GSignondIdentityIface *iface, const gchar *reference)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(iface);
    gint32 res;

    g_signal_emit_by_name (self,
                           "remove-reference",
                           self->priv->info,
                           reference,
                           &res);
    return res;
}

static void
gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondIdentityIfaceInterface *identity_iface =
        (GSignondIdentityIfaceInterface *) g_iface;

    (void)iface_data;

    identity_iface->request_credentials_update = _request_credentials_update;
    identity_iface->get_info = _get_info;
    identity_iface->verify_user = _verify_user;
    identity_iface->verify_secret = _verify_secret;
    identity_iface->remove = _remove;
    identity_iface->sign_out = _sign_out;
    identity_iface->store = _store;
    identity_iface->add_reference = _add_reference;
    identity_iface->remove_reference = _remove_reference;
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
    return gsignond_identity_info_get_id (identity->priv->info);
}

/**
 * gsignond_identity_set_id:
 * @identity: instance of #GSignondIdentity
 * @id: unique identifier
 * 
 * Sets the #identity id to #id.
 *
 * Returns[transfer null]: Dbus object path used by this identity.
 */
gboolean 
gsignond_identity_set_id (GSignondIdentity *identity, guint32 id)
{
    gsignond_identity_info_set_id (identity->priv->info, id);
    g_object_notify_by_pspec (G_OBJECT(identity), properties[PROP_INFO]);

    return TRUE;
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
GSignondIdentity * gsignond_identity_new ( GSignondAuthServiceIface *owner,
                                          GSignondIdentityInfo *info,
                                          const gchar *app_context)
{
    GSignondIdentity *identity =
        GSIGNOND_IDENTITY(g_object_new (GSIGNOND_TYPE_IDENTITY,
                                        "info", info,
                                        "app-context", app_context,
                                        NULL));

    identity->priv->owner = g_object_ref (owner);
}

