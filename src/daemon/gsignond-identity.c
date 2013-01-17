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

#include "daemon/gsignond-identity-iface.h"
#include "daemon/dbus/gsignond-dbus.h"
#include "daemon/dbus/gsignond-dbus-identity-adapter.h"
#include "gsignond-identity.h"

enum 
{
    PROP_0,
    PROP_ID,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

struct _GSignondIdentityPrivate
{
    guint32 id;
    GSignondDaemon *parent;
    GSignondDbusIdentityAdapter *identity_adapter;
};

static void gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GSignondIdentity, gsignond_identity, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GSIGNOND_TYPE_IDENTITY_IFACE, 
                                               gsignond_identity_iface_init));


#define GSIGNON_IDENTITY_PRIV(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSIGNOND_TYPE_IDENTITY, GSignondIdentityPrivate)

static guint32 gsignond_identity_request_credentials_update (
                                                    GSignondIdentityIface *self,
                                                    const gchar *message);
static GVariant * gsignond_identity_get_info (GSignondIdentityIface *self);
static gboolean gsignond_identity_verify_user (GSignondIdentityIface *self,
                                               const GVariant *params);
static gboolean gsignond_identity_verify_secret (GSignondIdentityIface *self,
                                                 const gchar *secret);
static void gsignond_identity_remove (GSignondIdentityIface *self);
static gboolean gsignond_identity_sing_out (GSignondIdentityIface *self);
static guint32 gsignond_identity_store (GSignondIdentityIface *self,
                                        const GVariant *info);
static gint32 gsignond_identity_add_reference (GSignondIdentityIface *self,
                                               const gchar *reference);
static gint32 gsignond_identity_remove_reference (GSignondIdentityIface *self,
                                                  const gchar *reference);


static GObject*
gsignond_identity_constructor (GType type,
                               guint n_construct_params,
                               GObjectConstructParam *construct_params)
{
    return G_OBJECT_CLASS (gsignond_identity_parent_class)->constructor (
                  type, n_construct_params, construct_params);
        
}

static void
gsignond_identity_get_property (GObject *object, 
                                guint property_id,
                                GValue *value, GParamSpec *pspec)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY (object);

    switch (property_id)
    {
        case PROP_ID: {
            guint32 id = gsignond_identity_get_id (self);
            g_value_set_uint (value, id);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_identity_set_property (GObject *object, 
                                guint property_id,
                                const GValue *value, GParamSpec *pspec)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY (object);

    switch (property_id)
    {
        case PROP_ID: {
            guint32 id = g_value_get_uint (value);
            gsignond_identity_set_id (self, id);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gsignond_identity_dispose (GObject *object)
{
    GSignondIdentity *self = GSIGNOND_IDENTITY(object);

    if (self->priv->identity_adapter) {
        g_object_unref (self->priv->identity_adapter);
        self->priv->identity_adapter = NULL;
    }

    if (self->priv->parent) {
        g_object_unref (self->priv->parent);
        self->priv->parent = NULL;
    }
   
    G_OBJECT_CLASS (gsignond_identity_parent_class)->dispose (object);
}

static void
gsignond_identity_finalize (GObject *object)
{
    G_OBJECT_CLASS (gsignond_identity_parent_class)->finalize (object);
}

static void
gsignond_identity_init (GSignondIdentity *self)
{
    GError *err = NULL;
    self->priv = GSIGNON_IDENTITY_PRIV(self);

    self->priv->identity_adapter =
        gsignond_dbus_identity_adapter_new (GSIGNOND_IDENTITY_IFACE (self));

}

static void
gsignond_identity_class_init (GSignondIdentityClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (GSignondIdentityPrivate));

    object_class->constructor = gsignond_identity_constructor;
    object_class->get_property = gsignond_identity_get_property;
    object_class->set_property = gsignond_identity_set_property;
    object_class->dispose = gsignond_identity_dispose;
    object_class->finalize = gsignond_identity_finalize;

    properties[PROP_ID] =
        g_param_spec_uint ("id", 
                           "unique id", 
                           "Unique identity id", 
                           0, 
                           G_MAXINT, 
                           0, 
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gsignond_identity_iface_init (gpointer g_iface, gpointer iface_data)
{
    GSignondIdentityIfaceInterface *identity_iface =
        (GSignondIdentityIfaceInterface *) g_iface;

    (void)iface_data;

    identity_iface->request_credentials_update =
        gsignond_identity_request_credentials_update;
    identity_iface->get_info = gsignond_identity_get_info;
    identity_iface->verify_user = gsignond_identity_verify_user;
    identity_iface->verify_secret = gsignond_identity_verify_secret;
    identity_iface->remove = gsignond_identity_remove;
    identity_iface->sign_out = gsignond_identity_sing_out;
    identity_iface->store = gsignond_identity_store;
    identity_iface->add_reference = gsignond_identity_add_reference;
    identity_iface->remove_reference = gsignond_identity_remove_reference;
}

static guint32
gsignond_identity_request_credentials_update (GSignondIdentityIface *self,
                                              const gchar *message) 
{
    (void) self;
    (void) message;

    return 0;
}

static GVariant * 
gsignond_identity_get_info (GSignondIdentityIface *self)
{
    (void) self;

    return NULL;
}

static gboolean 
gsignond_identity_verify_user (GSignondIdentityIface *self,
                               const GVariant *params)
{
    (void) self;
    (void) params;

    return FALSE;
}

static gboolean
gsignond_identity_verify_secret (GSignondIdentityIface *self,
                                 const gchar *secret)
{
    (void) self;
    (void) secret;

    return FALSE;
}

static void
gsignond_identity_remove (GSignondIdentityIface *self)
{
    (void) self;
}

static gboolean 
gsignond_identity_sing_out (GSignondIdentityIface *self)
{
    (void) self;

    return FALSE;
}

static guint32
gsignond_identity_store (GSignondIdentityIface *self, const GVariant *info)
{
    (void) self;
    (void) info;

    return 0;
}

static gint32
gsignond_identity_add_reference (GSignondIdentityIface *self,
                                 const gchar *reference)
{
    (void) self;
    (void) reference;

    return 0;
}

static gint32
gsignond_identity_remove_reference (GSignondIdentityIface *self,
                                    const gchar *reference)
{
    (void) self;
    (void) reference;

    return 0;
}

guint32 gsignond_identity_get_id (GSignondIdentity *self)
{
    return self->priv->id;
}

gboolean gsignond_identity_set_id (GSignondIdentity *self, guint32 id)
{
    self->priv->id = id;

    return TRUE;
}

GSignondIdentity * gsignond_identity_new (guint32 id, GSignondDaemon *parent)
{
    GSignondIdentity *identity =
        GSIGNOND_IDENTITY(g_object_new (GSIGNOND_TYPE_IDENTITY, "id", id, NULL));

    identity->priv->parent = g_object_ref (parent);
}

