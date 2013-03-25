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

G_DEFINE_INTERFACE (GSignondAuthSessionIface, gsignond_auth_session_iface, 0)

enum {
    SIG_PROCESS_RESULT,
    SIG_PROCESS_ERROR,
    SIG_PROCESS_STORE,
    SIG_PROCESS_USER_ACTION_REQUIRED,
    SIG_PROCESS_REFRESHED,
    SIG_PROCESS_STATE_CHANGED,
    
    SIG_MAX
};

static guint signals[SIG_MAX] = { 0 };

static void
gsignond_auth_session_iface_default_init (
                                        GSignondAuthSessionIfaceInterface *self)
{
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
            GSIGNOND_TYPE_SESSION_DATA);

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

    signals[SIG_PROCESS_STORE] =  g_signal_new ("process-store",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_SESSION_DATA);
    
    signals[SIG_PROCESS_USER_ACTION_REQUIRED] =  g_signal_new (
            "process-user-action-required",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_SIGNONUI_DATA);

    signals[SIG_PROCESS_REFRESHED] =  g_signal_new (
            "process-refreshed",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            GSIGNOND_TYPE_SIGNONUI_DATA);

    signals[SIG_PROCESS_STATE_CHANGED] =  g_signal_new (
            "state-changed",
            G_TYPE_FROM_INTERFACE (self),
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            2,
            G_TYPE_INT, G_TYPE_STRING);
}

gchar **
gsignond_auth_session_iface_query_available_mechanisms (
                                                GSignondAuthSessionIface *self,
                                                const gchar **wanted_mechanisms,
                                                const GSignondSecurityContext *ctx,
                                                GError **error)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        query_available_mechanisms (self, wanted_mechanisms, ctx, error);
}

gboolean
gsignond_auth_session_iface_process (GSignondAuthSessionIface *self,
                                     GSignondSessionData *session_data,
                                     const gchar *mechanism,
                                     const GSignondSecurityContext *ctx,
                                     GError **error)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        process (self, session_data, mechanism, ctx, error);
}

gboolean
gsignond_auth_session_iface_cancel (GSignondAuthSessionIface *self,
                                    const GSignondSecurityContext *ctx,
                                    GError **error)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->cancel (self, ctx, error);
}

GSignondAccessControlManager *
gsignond_auth_session_iface_get_acm (GSignondAuthSessionIface *self)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->get_acm (self);
}

void 
gsignond_auth_session_iface_user_action_finished (GSignondAuthSessionIface *self, 
                                                  GSignondSignonuiData *ui_data)
{
    return GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        user_action_finished (self, ui_data);
}

void 
gsignond_auth_session_iface_refresh (GSignondAuthSessionIface *self, 
                                     GSignondSignonuiData *ui_data)
{
    GSIGNOND_AUTH_SESSION_GET_INTERFACE (self)->
        refresh (self, ui_data);
}

void
gsignond_auth_session_iface_notify_process_result (GSignondAuthSessionIface *iface,
                                                   GSignondSessionData *result)
{
    g_signal_emit (iface, signals[SIG_PROCESS_RESULT], 0, result);
}

void
gsignond_auth_session_iface_notify_process_error (GSignondAuthSessionIface *iface,
                                                  const GError *error)
{
    g_signal_emit (iface, signals[SIG_PROCESS_ERROR], 0, error);
}

void 
gsignond_auth_session_iface_notify_store (GSignondAuthSessionIface *self, 
                                          GSignondSessionData *session_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_STORE], 0, session_data);
}

void 
gsignond_auth_session_iface_notify_user_action_required (GSignondAuthSessionIface *self, 
                                                         GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_USER_ACTION_REQUIRED], 0, ui_data);
}

void 
gsignond_auth_session_iface_notify_refreshed (GSignondAuthSessionIface *self, 
                                            GSignondSignonuiData *ui_data)
{
    g_signal_emit (self, signals[SIG_PROCESS_REFRESHED], 0, ui_data);
}

void 
gsignond_auth_session_iface_notify_state_changed (GSignondAuthSessionIface *self,
                                                  gint state,
                                                  const gchar *message)
{
    g_signal_emit (self, signals[SIG_PROCESS_STATE_CHANGED], 0, state,
        message);
}
