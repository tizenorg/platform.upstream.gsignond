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
#include "dbus/gsignond-dbus.h"
#include "dbus/gsignond-dbus-auth-session-adapter.h"
#include "gsignond-auth-session.h"

enum
{
	PROP_0,
	PROP_METHOD,
	N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondAuthSessionPrivate
{
    gchar *method;
    GSignondIdentityIface *owner;
    GSignondDbusAuthSessionAdapter *session_adapter;
};

static void gsignond_auth_session_iface_init (gpointer g_iface,
                                              gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondAuthSession, gsignond_auth_session,
                        G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_AUTH_SESSION,
                                               gsignond_auth_session_iface_init));

#define GSIGNOND_AUTH_SESSION_PRIV(obj) \
    G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_AUTH_SESSION, \
                                 GSignondAuthSessionPrivate)

static gchar **
_query_available_mechanisms (GSignondAuthSessionIface *iface,
                             const gchar **wanted_mechanisms)
{
    return NULL;
}

static gboolean
_process (GSignondAuthSessionIface *iface, GSignondSessionData *session_data,
          const gchar *mechanism)
{
    GSignondAuthSession *self = GSIGNOND_AUTH_SESSION (iface);

    (void) self;
    (void) session_data;
    (void) mechanism;
    /*
     * TODO: Implement process() when results ready
     * call gsignond_auth_session_iface_notify_process_result (iface, results);
     *
     */

     return TRUE;
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

    if (self->priv->session_adapter) {
        g_object_unref (self->priv->session_adapter);
        self->priv->session_adapter = NULL;
    }

    if (self->priv->owner) {
        g_object_unref (self->priv->owner);
        self->priv->owner = NULL;
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
                             G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_auth_session_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondAuthSessionIfaceInterface *auth_session_iface =
        (GSignondAuthSessionIfaceInterface *) g_iface;

    (void) iface_data;

    auth_session_iface->process = _process;
    auth_session_iface->query_available_mechanisms = _query_available_mechanisms;
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

/**
 * gsignond_auth_session_new:
 * @owner: instance of #GSignondIdentityIface
 * @method: authentication method
 *
 * Creates instance of #GSignondAuthSession.
 *
 * Returns: (transfer full) newly created object 
 */
GSignondAuthSession * 
gsignond_auth_session_new (GSignondIdentityIface *owner, const gchar *method)
{
    GSignondAuthSession *auth_session =
        g_object_new (GSIGNOND_TYPE_AUTH_SESSION, "method", method, NULL);

    return auth_session;
}

