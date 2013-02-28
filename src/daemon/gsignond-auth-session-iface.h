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

#ifndef __GSIGNOND_AUTH_SESSION_IFACE_H_
#define __GSIGNOND_AUTH_SESSION_IFACE_H_

#include <glib.h>
#include <glib-object.h>
#include <gsignond/gsignond-session-data.h>

G_BEGIN_DECLS

#define GSIGNOND_TYPE_AUTH_SESSION_IFACE          (gsignond_auth_session_iface_get_type ())
#define GSIGNOND_AUTH_SESSION_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSIGNOND_TYPE_AUTH_SESSION_IFACE, GSignondAuthSessionIface))
#define GSIGNOND_IS_AUTH_SESSION_IFACE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSIGNOND_TYPE_AUTH_SESSION_IFACE))
#define GSIGNOND_AUTH_SESSION_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GSIGNOND_TYPE_AUTH_SESSION_IFACE, GSignondAuthSessionIfaceInterface))

typedef struct _GSignondAuthSessionIface GSignondAuthSessionIface; /* dummy */
typedef struct _GSignondAuthSessionIfaceInterface GSignondAuthSessionIfaceInterface;

struct _GSignondAuthSessionIfaceInterface {
    GTypeInterface parent;

    /**
     * query_available_mechanisms:
     * @session: instance of #GSignondAuthSessionIface
     * @desired_mechanisms: desired authentication mechanisms
     * @error: return location for error
     *
     * Checks for support of desired authentication mechanisms #desired_mechanisms for this
     * authentication session, The result will be interseciton of desired authenticaiton mechansims 
     * and available authenticaiton mechansims.
     *
     * Returns: (transfer full) list of supported authentication mechansims out of #desired_mechanisms
     * Caller should use g_strfreev() when done with return value.
     */
    gchar **   (*query_available_mechanisms) (GSignondAuthSessionIface *session,
                                              const gchar **desired_mechanisms,
                                              GError **error);

    /**
     * process:
     * @session: instance of #GSignondAuthSessionIface
     * @session_data: authentication session data to use
     * @mechansims: authentication mechanism to use
     * @error: return location for error
     *
     * Initiates authentication process on #session, On successful authentication #gsignond_auth_session_iface_notify_process_result will be called.
     * In case failure occured in authentication process, the error is informed via #gsignond_auth_session_iface_notify_process_error.

     * Returns: @TRUE if authentication process was started successfully, @FALSE otherwise
     */
    gboolean   (*process) (GSignondAuthSessionIface *session,
                           GSignondSessionData *session_data,
                           const gchar *mechanism,
                           GError **error);

    /**
     * cancel:
     * @session: instance of #GSignondAuthSessionIface
     * @error: return location for error
     *
     */
    gboolean   (*cancel) (GSignondAuthSessionIface *session,
                          GError **error);

    void (*user_action_finished) (GSignondAuthSessionIface *session, 
                                  GSignondSessionData *session_data);

    void (*refresh) (GSignondAuthSessionIface *session, 
                     GSignondSessionData *session_data);

};

GType gsignond_auth_session_iface_get_type (void);

gchar ** 
gsignond_auth_session_iface_query_available_mechanisms (GSignondAuthSessionIface *self,
                                                        const gchar **wanted_mechanisms,
                                                        GError **error);
gboolean 
gsignond_auth_session_iface_process (GSignondAuthSessionIface *self,
                                     GSignondSessionData *session_data,
                                     const gchar *mechanism,
                                     GError **error);
gboolean
gsignond_auth_session_iface_cancel (GSignondAuthSessionIface *self,
                                    GError **error);
void 
gsignond_auth_session_iface_user_action_finished (GSignondAuthSessionIface *self, 
                                                  GSignondSessionData *session_data);
void 
gsignond_auth_session_iface_refresh (GSignondAuthSessionIface *self, 
                                     GSignondSessionData *session_data);


/* handlers */
/**
  * process_reply:
  * @session: instance of #GSignondAuthSessionIface
  * @results: authentication process results
  *
  * Function to be called with #results on authentication process success.
  */
void
gsignond_auth_session_iface_notify_process_result (GSignondAuthSessionIface *iface,
                                                   GSignondSessionData *result);

/**
  * process_error:
  * @session: instance of #GSignondAuthSessionIface
  * @error: error of type #GError, occured during authentication process
  *
  * Function to be called with #error on authentication process failure.
  */
void
gsignond_auth_session_iface_notify_process_error (GSignondAuthSessionIface *iface,
                                                  const GError *error);

void 
gsignond_auth_session_iface_notify_store (GSignondAuthSessionIface *self, 
                                          GSignondSessionData *session_data);
void 
gsignond_auth_session_iface_notify_user_action_required (GSignondAuthSessionIface *self, 
                                                         GSignondSessionData *session_data);
void 
gsignond_auth_session_iface_notify_refreshed (GSignondAuthSessionIface *self, 
                                              GSignondSessionData *session_data);
void 
gsignond_auth_session_iface_notify_state_changed (GSignondAuthSessionIface *self, 
                                                  gint state,
                                                  const gchar *message);

G_END_DECLS

#endif /* __GSIGNOND_AUTH_SESSION_IFACE_H_ */
