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

guint gsignond_daemon_identity_timeout (GSignondDaemon *self);

guint gsignond_daemon_auth_session_timeout (GSignondDaemon *self);

GSignondAccessControlManager *
gsignond_daemon_get_access_control_manager (GSignondDaemon *self);

G_END_DECLS

#endif /* __GSIGNOND_DAEMON_H_ */
