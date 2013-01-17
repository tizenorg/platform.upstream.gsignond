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

#include "gsignond-auth-session-iface.h"

G_DEFINE_INTERFACE (GSignondAuthSessionIface, gsignond_auth_session_iface, G_TYPE_OBJECT)

static gchar **
_dummy_query_available_mechanisms (GSignondAuthSessionIface *self,
                                   const gchar **wanted_mechanisms)
{
    (void) self;
    (void) wanted_mechanisms;
    return NULL;
}

static GVariant *
_dummy_process (GSignondAuthSessionIface *self, const GVariant *session_data,
                const gchar *mechanism)
{
    (void) self;
    (void) session_data;
    (void) mechanism;

    return NULL;
}

static void
_dummy_cancel (GSignondAuthSessionIface *self)
{
    (void) self;
}

static void
_dummy_set_id (GSignondAuthSessionIface *self, guint32 id)
{
    (void) self;
    (void) id;
}

static void
gsignond_auth_session_iface_default_init (
                                        GSignondAuthSessionIfaceInterface *self)
{
    self->query_available_mechanisms = _dummy_query_available_mechanisms;
    self->process = _dummy_process;
    self->cancel = _dummy_cancel;
    self->set_id = _dummy_set_id;
}


gchar **
gsignond_auth_session_iface_query_available_mechanisms (
                                                GSignondAuthSessionIface *self,
                                                const gchar **wanted_mechanisms)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        query_available_mechanisms (self, wanted_mechanisms);
}

GVariant *
gsignond_auth_session_iface_process (GSignondAuthSessionIface *self,
                                     const GVariant *session_data,
                                     const gchar *mechanism)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        process (self, session_data, mechanism);
}

void
gsignond_auth_session_iface_cancel (GSignondAuthSessionIface *self)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->cancel (self);
}

void
gsignond_auth_session_iface_set_id (GSignondAuthSessionIface *self, guint32 id)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->set_id (self, id);
}

