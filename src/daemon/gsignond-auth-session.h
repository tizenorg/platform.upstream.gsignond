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

#include "gsignond-identity-iface.h"
#include "gsignond-disposable.h"

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

typedef struct _GSignondAuthSession GSignondAuthSession;
typedef struct _GSignondAuthSessionClass GSignondAuthSessionClass;
typedef struct _GSignondAuthSessionPrivate GSignondAuthSessionPrivate;

struct _GSignondAuthSession
{
    GSignondDisposable parent;

    /* private */
    GSignondAuthSessionPrivate *priv;
};

struct _GSignondAuthSessionClass
{
    GSignondDisposableClass parent_class;
};

GType gsignond_auth_session_get_type (void);

const gchar *
gsignond_auth_session_get_method (GSignondAuthSession *session);

const gchar *
gsignond_auth_session_get_object_path (GSignondAuthSession *session);

gboolean gsignond_auth_session_set_id(GSignondAuthSession *session, gint id);

GSignondAuthSession * 
gsignond_auth_session_new (gint id,
                           const gchar *method);

G_END_DECLS

#endif  /* _GSGINOND_AUTH_SESSION_H_ */

