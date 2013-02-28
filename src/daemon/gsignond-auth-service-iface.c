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

#include "gsignond-auth-service-iface.h"

G_DEFINE_INTERFACE (GSignondAuthServiceIface, gsignond_auth_service_iface, G_TYPE_OBJECT)

static const gchar * 
_dummy_register_new_identity (GSignondAuthServiceIface *self,
                              const GSignondSecurityContext *ctx,
                              GError **error)
{
    (void) self;
    (void) ctx;
    (void) error;
    return NULL;
}

static const gchar *
_dummy_get_identity (GSignondAuthServiceIface *self, 
                     guint32 id,
                     const GSignondSecurityContext *ctx, 
                     GVariant **identity_data,
                     GError **error)
{
    (void) self;
    (void) id;
    (void) ctx;
    (void) identity_data;
    (void) error;
    return NULL;
}

static const gchar **
_dummy_query_methods (GSignondAuthServiceIface *self,
                      GError **error)
{
    (void) self;
    (void) error;
    return NULL;
}

static const gchar **
_dummy_query_mechanisms (GSignondAuthServiceIface *self, 
                         const gchar *method,
                         GError **error)
{
    (void) self;
    (void) method;
    (void) error;
    return NULL;
}

static GVariant *
_dummy_query_identities (GSignondAuthServiceIface *self, 
                         const GVariant *filter,
                         GError **error)
{
    (void) self;
    (void) filter;
    (void) error;
    return NULL;
}

static gboolean
_dummy_clear (GSignondAuthServiceIface *self,
              GError **error)
{
    (void) self;
    (void) error;
    return FALSE;
}

static GSignondAccessControlManager *
_dummy_get_acm (GSignondAuthServiceIface *self)
{
    (void) self;

    return NULL;
}

static void
gsignond_auth_service_iface_default_init (
                                        GSignondAuthServiceIfaceInterface *self)
{
    self->register_new_identity = _dummy_register_new_identity;
    self->get_identity = _dummy_get_identity;
    self->query_methods = _dummy_query_methods;
    self->query_mechanisms = _dummy_query_mechanisms;
    self->query_identities = _dummy_query_identities;
    self->clear = _dummy_clear;
    self->get_acm = _dummy_get_acm;
}

/**
 * gsignond_auth_service_iface_register_new_identity:
 * @self: instance of #GSignondAuthServiceIfacea
 * @ctx: security context
 * @error: return location for error
 *
 *
 * Returns: (transfer none) object path of newly created identity.
 */
const gchar *
gsignond_auth_service_iface_register_new_identity (
                                                 GSignondAuthServiceIface *self,
                                                 const GSignondSecurityContext *ctx,
                                                 GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        register_new_identity (self, ctx, error);
}

/**
 * gsignond_auth_service_iface_get_identity:
 * @self: instance of #GSignondAuthServiceIface
 * @id: id of the identity to lookup
 * @ctx: security context
 * @object_path:  return location for object path of the identity
 * @identity_data: return location for identity data
 * @error: return location for error
 *
 * Retrives object path and identity info stored for the given @id.
 *
 * Returns: (transfer none): object path of the identity on success, @NULL otherwise.
 */
const gchar *
gsignond_auth_service_iface_get_identity (GSignondAuthServiceIface *self,
                                          guint32 id,
                                          const GSignondSecurityContext *ctx,
                                          GVariant **identity_data,
                                          GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE(self)->
        get_identity (self, id, ctx, identity_data, error);
}

/**
 * gsignond_auth_service_iface_query_methods:
 * @self: instance of #GSignondAuthServiceIface
 * @error: return location for error
 *
 * Retrieves the available authentication methods. 
 *
 * Returns: (transfer none): list of methods
 */
const gchar **
gsignond_auth_service_iface_query_methods (GSignondAuthServiceIface *self,
                                           GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->query_methods (self, error);
}

/**
 * gsignond_auth_service_iface_query_mechanisms:
 * @self: instance of #GSignondAuthServiceIface
 * @method: method to query
 * @error: return location for error
 *
 * Retrieves the available mechanisms for authentication method @method.
 *
 * Returns: (transfer none): list of mechanisms
 */
const gchar **
gsignond_auth_service_iface_query_mechanisms (GSignondAuthServiceIface *self,
                                              const gchar *method,
                                              GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_mechanisms (self, method, error);
}

/**
 * gsignond_auth_service_iface_query_identities:
 * @self: instance of #GSignondAuthServiceIface
 * @filter: filter should be applited
 * @error: return location for error
 *
 * Retrieves the identities that satisifies the filter @filter.
 *
 * Returns: (transfer full): list of identities
 */
GVariant *
gsignond_auth_service_iface_query_identities (GSignondAuthServiceIface *self,
                                              const GVariant *filter,
                                              GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_identities (self, filter, error);
}

/**
 * gsignond_auth_service_iface_clear:
 * @self: instance of #GSignondAuthServiceIface
 * @error: return location for error
 *  
 * Clears the cache.
 *
 * Returns: TRUE on successful, FALSE otherwise
 */
gboolean
gsignond_auth_service_iface_clear (GSignondAuthServiceIface *self,
                                   GError **error)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->clear(self, error);
}

GSignondAccessControlManager *
gsignond_auth_service_iface_get_acm (GSignondAuthServiceIface *self)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->get_acm (self);
}
