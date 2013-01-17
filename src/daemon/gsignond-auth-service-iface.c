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
                              const GVariant *app_context)
{
    (void) self;
    (void) app_context;
    return 0;
}

static gboolean
_dummy_get_identity (GSignondAuthServiceIface *self, guint32 id,
                     const GVariant *app_context, gchar **object_path,
                     GVariant **identity_data)
{
    (void) self;
    (void) id;
    (void) app_context;
    (void) object_path;
    (void) identity_data;
    return FALSE;
}

static const gchar *
_dummy_get_auth_session_object_path (GSignondAuthServiceIface *self,
                                     guint32 id, const gchar *type,
                                     const GVariant *app_context)
{
    (void) self;
    (void) id;
    (void) type;
    (void) app_context;
    return 0;
}

static gchar **
_dummy_query_methods (GSignondAuthServiceIface *self)
{
    (void) self;
    return NULL;
}

static gchar **
_dummy_query_mechanisms (GSignondAuthServiceIface *self, const gchar *method)
{
    (void) self;
    (void) method;
    return NULL;
}

static GVariant *
_dummy_query_identities (GSignondAuthServiceIface *self, const GVariant *filter)
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
    self->get_auth_session_object_path = _dummy_get_auth_session_object_path;
    self->query_methods = _dummy_query_methods;
    self->query_mechanisms = _dummy_query_mechanisms;
    self->query_identities = _dummy_query_identities;
    self->clear = _dummy_clear;
}

const gchar *
gsignond_auth_service_iface_register_new_identity (
                                                 GSignondAuthServiceIface *self,
                                                 const GVariant *app_context)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        register_new_identity (self, app_context);
}

gboolean
gsignond_auth_service_iface_get_identity (GSignondAuthServiceIface *self,
                                          guint32 id,
                                          const GVariant *app_context,
                                          gchar **object_path,
                                          GVariant **identity_data)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE(self)->
        get_identity (self, id, app_context, object_path, identity_data);
}

const gchar *
gsignond_auth_service_iface_get_auth_session_object_path (
                                                 GSignondAuthServiceIface *self,
                                                 guint32 id,
                                                 const gchar *type,
                                                 const GVariant *app_context)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        get_auth_session_object_path (self, id, type, app_context);
}

gchar **
gsignond_auth_service_iface_query_methods (GSignondAuthServiceIface *self)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->query_methods (self);
}

gchar **
gsignond_auth_service_iface_query_mechanisms (GSignondAuthServiceIface *self,
                                              const gchar *method)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_mechanisms (self, method);
}

GVariant *
gsignond_auth_service_iface_query_identities (GSignondAuthServiceIface *self,
                                              const GVariant *filter)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->
        query_identities (self, filter);
}

gboolean
gsignond_auth_service_iface_clear (GSignondAuthServiceIface *self)
{
    return GSIGNOND_AUTH_SERVICE_GET_INTERFACE (self)->clear(self);
}

