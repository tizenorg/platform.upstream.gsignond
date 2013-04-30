/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of gsignond
 *
 * Copyright (C) 2012 Intel Corporation.
 *
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

#ifndef __GSIGNOND_DAEMON_H_
#define __GSIGNOND_DAEMON_H_

#include <glib.h>
#include <glib-object.h>

#include <gsignond/gsignond-access-control-manager.h>
#include "gsignond-identity.h"
#include "gsignond-signonui-proxy.h"
#include "plugins/gsignond-plugin-proxy-factory.h"

G_BEGIN_DECLS

#define GSIGNOND_TYPE_DAEMON            (gsignond_daemon_get_type())
#define GSIGNOND_DAEMON(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSIGNOND_TYPE_DAEMON, GSignondDaemon))
#define GSIGNOND_DAEMON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSIGNOND_TYPE_DAEMON, GSignondDaemonClass))
#define GSIGNOND_IS_DAEMON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSIGNOND_TYPE_DAEMON))
#define GSIGNOND_IS_DAEMON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GSIGNOND_TYPE_DAEMON))
#define GSIGNOND_DAEMON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSIGNOND_TYPE_DAEMON, GSignondDaemonClass))

typedef struct _GSignondDaemon GSignondDaemon;
typedef struct _GSignondDaemonClass GSignondDaemonClass;
typedef struct _GSignondDaemonPrivate GSignondDaemonPrivate;

struct _GSignondDaemon
{
    GObject parent;

    /* priv */
    GSignondDaemonPrivate *priv;
};

struct _GSignondDaemonClass
{
    GObjectClass parent_class;
};

GType gsignond_daemon_get_type (void) G_GNUC_CONST;

GSignondDaemon * gsignond_daemon_new ();

GSignondIdentity *
gsignond_daemon_register_new_identity (GSignondDaemon *daemon,
                                       const GSignondSecurityContext *ctx,
                                       GError **error) ;

GSignondIdentity *
gsignond_daemon_get_identity (GSignondDaemon *daemon,
                              const guint32 id,
                              const GSignondSecurityContext *ctx,
                              GError **error);

const gchar ** 
gsignond_daemon_query_methods (GSignondDaemon *daemon, GError **error);

const gchar ** 
gsignond_daemon_query_mechanisms (GSignondDaemon *daemon,
                                  const gchar *method,
                                  GError **error);

GList *
gsignond_daemon_query_identities (GSignondDaemon *daemon,
                                  GVariant *filter,
                                  const GSignondSecurityContext *ctx,
                                  GError **error);

gboolean 
gsignond_daemon_clear (GSignondDaemon *daemon,
                       const GSignondSecurityContext *ctx,
                       GError **error);

guint32
gsignond_daemon_store_identity (GSignondDaemon *daemon, GSignondIdentity *identity);

gboolean
gsignond_daemon_remove_identity (GSignondDaemon *daemon, guint32 id);

guint32
gsignond_daemon_add_identity_reference (GSignondDaemon *daemon, guint32 identity_id, const GSignondSecurityContext *owner, const gchar *ref);

gboolean
gsignond_daemon_remove_identity_reference (GSignondDaemon *daemon, guint32 identity_id, const GSignondSecurityContext *owner, const gchar *ref);

guint
gsignond_daemon_get_timeout (GSignondDaemon *self) G_GNUC_CONST;

guint
gsignond_daemon_get_identity_timeout (GSignondDaemon *self) G_GNUC_CONST;

guint
gsignond_daemon_get_auth_session_timeout (GSignondDaemon *self) G_GNUC_CONST;

GSignondAccessControlManager *
gsignond_daemon_get_access_control_manager (GSignondDaemon *self) G_GNUC_CONST;

GSignondPluginProxyFactory *
gsignond_daemon_get_plugin_proxy_factory (GSignondDaemon *self) G_GNUC_CONST;

GSignondConfig *
gsignond_daemon_get_config (GSignondDaemon *self) G_GNUC_CONST;

gboolean
gsignond_daemon_show_dialog (GSignondDaemon *self,
                             GObject *caller,
                             GSignondSignonuiData *ui_data,
                             GSignondSignonuiProxyQueryDialogCb handler,
                             GSignondSignonuiProxyRefreshCb refresh_handler,
                             gpointer userdata);

gboolean
gsignond_daemon_refresh_dialog (GSignondDaemon *self,
                                GObject *caller,
                                GSignondSignonuiData *ui_data,
                                GSignondSignonuiProxyRefreshDialogCb handler,
                                gpointer userdata);

gboolean
gsignond_daemon_cancel_dialog (GSignondDaemon *self,
                               GObject *caller,
                               GSignondSignonuiProxyCancelRequestCb handler,
                               gpointer userdata);

GSignondAccessControlManager *
gsignond_get_access_control_manager ();

GSignondPluginProxyFactory *
gsignond_get_plugin_proxy_factory ();

GSignondConfig *
gsignond_get_config ();

G_END_DECLS

#endif /* __GSIGNOND_DAEMON_H_ */
