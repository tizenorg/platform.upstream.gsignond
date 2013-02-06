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

enum {
    SIG_PROCESS_RESULT,
    SIG_PROCESS_ERROR,
    SIG_MAX
};

guint signals[SIG_MAX];

static gchar **
_dummy_query_available_mechanisms (GSignondAuthSessionIface *self,
                                   const gchar **wanted_mechanisms)
{
    (void) self;
    (void) wanted_mechanisms;
    return NULL;
}

static gboolean 
_dummy_process (GSignondAuthSessionIface *self, const GVariant *session_data,
                const gchar *mechanism)
{
    (void) self;
    (void) session_data;
    (void) mechanism;

    return FALSE;
}

static void
_dummy_cancel (GSignondAuthSessionIface *self)
{
    (void) self;
}

static void
gsignond_auth_session_iface_default_init (
                                        GSignondAuthSessionIfaceInterface *self)
{
    self->query_available_mechanisms = _dummy_query_available_mechanisms;
    self->process = _dummy_process;
    self->cancel = _dummy_cancel;

    /**
     * GSignondAuthSessionIfaceInterface::process:
     * @object: A #GSignondAuthServiceIface.
     * @results:
     *
     * Signal emitted when a processing is done.
     *
     */
    signals[SIG_PROCESS_RESULT] =  g_signal_new ("process-result",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            G_TYPE_VARIANT);

    signals[SIG_PROCESS_ERROR] = g_signal_new ("process-error",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            G_TYPE_ERROR);
}

gchar **
gsignond_auth_session_iface_query_available_mechanisms (
                                                GSignondAuthSessionIface *self,
                                                const gchar **wanted_mechanisms)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        query_available_mechanisms (self, wanted_mechanisms);
}

gboolean
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
gsignond_auth_session_iface_notify_process_result (
                                                GSignondAuthSessionIface *iface,
                                                const GVariant *result)
{
    g_signal_emit (iface, signals[SIG_PROCESS_RESULT], 0, result);
}

void
gsignond_auth_session_iface_notify_process_error (
                                                GSignondAuthSessionIface *iface,
                                                const GError *error)
{
    g_signal_emit (iface, signals[SIG_PROCESS_ERROR], 0, error);
}
