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

#ifndef _GSIGNOND_AUTH_SESSION_H_
#define _GSIGNOND_AUTH_SESSION_H_

#include <glib-object.h>

#include "gsignond-types.h"
#include <gsignond/gsignond-dictionary.h>
#include <gsignond/gsignond-identity-info.h>
#include <gsignond/gsignond-signonui-data.h>
#include <gsignond/gsignond-session-data.h>
#include <gsignond/gsignond-access-control-manager.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SESSION \
    (gsignond_auth_session_get_type())
#define GSIGNOND_AUTH_SESSION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_AUTH_SESSION, \
                                GSignondAuthSession))
#define GSIGNOND_AUTH_SESSION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_AUTH_SESSION, \
                             GSignondAuthSessionClass))
#define GSIGNOND_IS_AUTH_SESSION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_AUTH_SESSION))
#define GSIGNOND_IS_AUTH_SESSION_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_AUTH_SESSION))
#define GSIGNOND_AUTH_SESSION_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_AUTH_SESSION, \
                               GSignondAuthSessionClass))

typedef struct _GSignondAuthSessionPrivate GSignondAuthSessionPrivate;
typedef void (*ProcessReadyCb) (GSignondSessionData *results, const GError *error, gpointer user_data);
typedef void (*StateChangeCb) (gint state, const gchar *message, gpointer userdata);

struct _GSignondAuthSession
{
    GObject parent;

    /* private */
    GSignondAuthSessionPrivate *priv;
};

struct _GSignondAuthSessionClass
{
    GObjectClass parent_class;
};

GType gsignond_auth_session_get_type (void);

gchar **
gsignond_auth_session_query_available_mechanisms (GSignondAuthSession *self,
                                                  const gchar **wanted_mechanisms,
                                                  const GSignondSecurityContext *ctx,
                                                  GError **error);

gboolean
gsignond_auth_session_process (GSignondAuthSession *self,
                               GSignondSessionData *session_data,
                               const gchar *mechanism,
                               const GSignondSecurityContext *ctx,
                               ProcessReadyCb ready_cb,
                               StateChangeCb state_change_cb,
                               gpointer userdata,
                               GError **error);
gboolean
gsignond_auth_session_cancel (GSignondAuthSession *self,
                              const GSignondSecurityContext *ctx,
                              GError **error);

void
gsignond_auth_session_abort_process (GSignondAuthSession *self);

void 
gsignond_auth_session_user_action_finished (GSignondAuthSession *self,
                                            GSignondSignonuiData *ui_data);

void
gsignond_auth_session_refresh (GSignondAuthSession *self, 
                               GSignondSignonuiData *ui_data);

const gchar *
gsignond_auth_session_get_method (GSignondAuthSession *session);

GSignondDictionary *
gsignond_auth_session_get_token_data (GSignondAuthSession *session);

GSignondAccessControlManager *
gsignond_auth_session_get_acm (GSignondAuthSession *session);

gboolean 
gsignond_auth_session_set_id(GSignondAuthSession *session, gint id);

void
gsignond_auth_session_notify_process_result (GSignondAuthSession *iface,
                                             GSignondSessionData *result,
                                             gpointer userdata);

void
gsignond_auth_session_notify_process_error (GSignondAuthSession *iface,
                                            const GError *error,
                                            gpointer userdata);
void 
gsignond_auth_session_notify_state_changed (GSignondAuthSession *self, 
                                            gint state,
                                            const gchar *message,
                                            gpointer userdata);

void 
gsignond_auth_session_notify_store (GSignondAuthSession *self, 
                                    GSignondDictionary *token_data);

void 
gsignond_auth_session_notify_user_action_required (GSignondAuthSession *self, 
                                                   GSignondSignonuiData *ui_data);

void 
gsignond_auth_session_notify_refreshed (GSignondAuthSession *self, 
                                        GSignondSignonuiData *ui_data);

GSignondAuthSession * 
gsignond_auth_session_new (GSignondIdentityInfo *info,
                           const gchar *method,
                           GSignondDictionary *token_data);

G_END_DECLS

#endif  /* _GSIGNOND_AUTH_SESSION_H_ */

