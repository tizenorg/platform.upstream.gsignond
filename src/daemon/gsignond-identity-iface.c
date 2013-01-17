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

#include "gsignond-identity-iface.h"

enum 
{
    SIGNAL_UNREGISTERED,
    SIGNAL_INFO_UPDATED
};

G_DEFINE_INTERFACE (GSignondIdentityIface, gsignond_identity_iface, G_TYPE_OBJECT)

static guint32
_dummy_request_credentials_update (GSignondIdentityIface *self,
                                   const gchar *message)
{
    (void) self;
    (void) message;

    return 0;
}

static GVariant *
_dummy_get_info (GSignondIdentityIface *self)
{
    (void) self;

    return 0;
}

static gboolean
_dummy_verify_user (GSignondIdentityIface *self, const GVariant *params)
{
    (void) self;
    (void) params;

    return FALSE;
}

static gboolean
_dummy_verify_secret (GSignondIdentityIface *self, const gchar *secret)
{
    (void) self;
    (void) secret;

    return FALSE;
}

static void
_dummy_remove (GSignondIdentityIface *self)
{
    (void) self;
}

static gboolean
_dummy_sign_out (GSignondIdentityIface *self)
{
    (void) self;

    return FALSE;
}

static guint32
_dummy_store (GSignondIdentityIface *self, const GVariant *info)
{
    (void) self;
    (void) info;

    return 0;
}

static gint32
_dummy_add_reference (GSignondIdentityIface *self, const gchar *reference)
{
    (void) self;
    (void) reference;

    return 0;
}

static gint32
_dummy_remove_reference (GSignondIdentityIface *self, const gchar *reference)
{
    (void) self;
    (void) reference;

    return 0;
}

static void
gsignond_identity_iface_default_init (GSignondIdentityIfaceInterface *self)
{
    self->request_credentials_update = _dummy_request_credentials_update;
    self->get_info = _dummy_get_info;
    self->verify_user = _dummy_verify_user;
    self->verify_secret = _dummy_verify_secret;
    self->remove = _dummy_remove;
    self->sign_out = _dummy_sign_out;
    self->store = _dummy_store;
    self->add_reference = _dummy_add_reference;
    self->remove_reference = _dummy_remove_reference;
}

guint32
gsignond_identity_iface_request_credentials_update (GSignondIdentityIface *self,
                                                    const gchar *message)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        request_credentials_update (self, message);
}

GVariant *
gsignond_identity_iface_get_info (GSignondIdentityIface *self)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->get_info (self);
}

gboolean
gsignond_identity_iface_verify_user (GSignondIdentityIface *self,
                                     const GVariant *params)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        verify_user (self, params);
}

gboolean
gsignond_identity_iface_verify_secret (GSignondIdentityIface *self,
                                       const gchar *secret)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        verify_secret (self, secret);
}

void
gsignond_identity_iface_remove (GSignondIdentityIface *self)
{
    GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->remove (self);
}

gboolean
gsignond_identity_iface_sign_out (GSignondIdentityIface *self)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->sign_out (self);
}

guint32
gsignond_identity_iface_store (GSignondIdentityIface *self,
                               const GVariant *info)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->store (self, info);
}

gint32
gsignond_identity_iface_add_reference (GSignondIdentityIface *self,
                                       const gchar *reference)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        add_reference (self, reference);
}

gint32
gsignond_identity_iface_remove_reference (GSignondIdentityIface *self,
                                          const gchar *reference)
{
    return GSIGNOND_IDENTITY_IFACE_GET_INTERFACE (self)->
        add_reference (self, reference);
}
