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
                              const gchar *app_context)
{
    (void) self;
    (void) app_context;
    return 0;
}

static const gchar *
_dummy_get_identity (GSignondAuthServiceIface *self, 
                     guint32 id,
                     const gchar *app_context, 
                     GVariant **identity_data)
{
    (void) self;
    (void) id;
    (void) app_context;
    (void) identity_data;
    return FALSE;
}

static gchar **
_dummy_query_methods (GSignondAuthServiceIface *self)
{
    (void) self;
    return NULL;
}

static gchar **
_dummy_query_mechanisms (GSignondAuthServiceIface *self, 
                         const gchar *method)
{
    (void) self;
    (void) method;
    return NULL;
}

static GVariant *
_dummy_query_identities (GSignondAuthServiceIface *self, 
                         const GVariant *filter)
{
    (void) self;
    (void) filter;
    return NULL;
}

static gboolean
_dummy_clear (GSignondAuthServiceIface *self)
{
    (void) self;

    return FALSE;
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
}

/**
 * gsignond_auth_service_iface_register_new_identity:
 * @self: instance of #GSignondAuthServiceIfacea
 * @app_context: application security context
 *
 *
 * Returns: (transfer none) object path of newly created identity.
 */
const gchar *
gsignond_auth_service_iface_register_new_identity (
                                                 GSignondAuthServiceIface *self,
                                                 const gchar *app_context)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        register_new_identity (self, app_context);
}

/**
 * gsignond_auth_service_iface_get_identity:
 * @self: instance of #GSignondAuthServiceIface
 * @id: id of the identity to lookup
 * @app_context: application security context
 * @object_path:  return location for object path of the identity
 * @identity_data: return location for identity data
 *
 * Retrives object path and identity info stored for the given @id.
 *
 * Returns: (transfer none): object path of the identity on success, @NULL otherwise.
 */
const gchar *
gsignond_auth_service_iface_get_identity (GSignondAuthServiceIface *self,
                                          guint32 id,
                                          const gchar *app_context,
                                          GVariant **identity_data)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE(self)->
        get_identity (self, id, app_context, identity_data);
}

/**
 * gsignond_auth_service_iface_query_methods:
 * @self: instance of #GSignondAuthServiceIface
 *
 * Retrieves the available authentication methods. Caller should free the return value
 * with #g_strfreev() when done with it.
 *
 * Returns: (transfer full): list of methods
 */
gchar **
gsignond_auth_service_iface_query_methods (GSignondAuthServiceIface *self)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->query_methods (self);
}

/**
 * gsignond_auth_service_iface_query_mechanisms:
 * @self: instance of #GSignondAuthServiceIface
 * @method: method to query
 *
 * Retrieves the available mechanisms for authentication method @method. Caller should free the return value
 * with #g_strfreev() when done with it.
 *
 * Returns: (transfer full): list of mechanisms
 */
gchar **
gsignond_auth_service_iface_query_mechanisms (GSignondAuthServiceIface *self,
                                              const gchar *method)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_mechanisms (self, method);
}

/**
 * gsignond_auth_service_iface_query_identities:
 * @self: instance of #GSignondAuthServiceIface
 * @filter: filter should be applited
 *
 * Retrieves the identities that satisifies the filter @filter.
 *
 * Returns: (transfer full): list of identities
 */
GVariant *
gsignond_auth_service_iface_query_identities (GSignondAuthServiceIface *self,
                                              const GVariant *filter)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_identities (self, filter);
}

/**
 * gsignond_auth_service_iface_clear:
 * @self: instance of #GSignondAuthServiceIface
 *  
 * Clears the cache.
 *
 * Returns: TRUE on successful, FALSE otherwise
 */
gboolean
gsignond_auth_service_iface_clear (GSignondAuthServiceIface *self)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->clear(self);
}

